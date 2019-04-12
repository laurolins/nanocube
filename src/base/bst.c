#ifdef bst_UNIT_TEST
#include "platform.c"
#endif




//
// balanced search tree
//
// based on the paper:
//     Balanced Search Trees Made Simple
//     Arne Andersson
//
// bst_ stands for balanced search tree
//

// @todo
// - multiple values for the same key (there needs to be an extra counter in case some of the key's values are null)

typedef struct {
	u32 used:1;
	u32 level:5;  // 2^(2^32) nodes max
	u32 count:26; // max of 67M entries of the same
	union {
		struct {
			u32 left; // node handle
			u32 right;
			u32 value;
			u64 key;
		};
		u32 next_free;
	};
} bst_Node;

typedef struct {
	u32 node_count; // same key doesn't increase the node count
	u32 count;      // total number of items inserted and not removed
	                // accounts for the same keys which go in the same node
	u32 root;
	u32 free_node;  // node handle of first free node
	u32 free_value; // value handle of first free value
	u32 max_value_length; // data_max_length
	u32 left;
	u32 right;
	u32 size;
	u32 pad_;
} bst_Tree;

//
// the bst_ValueTrailer goes as a trailer
// after the value
//
typedef union {
	u32 next; // next value for the same key
	// this is a LIFO (last in first out thing,
	// except that nulls are the last to go out)
	u32 next_free;
} bst_ValueTrailer;

StaticAssertAlignment(bst_Tree,8);
StaticAssertAlignment(bst_Node,8);
StaticAssertAlignment(bst_ValueTrailer,4);

static s32
bst_full(bst_Tree* self)
{
	if (self->free_node && self->free_value) return 0;
	else if (self->free_node) {
		return self->left + self->max_value_length > self->right;
	} else if (self->free_value) {
		return self->left + sizeof(bst_Node) > self->right;
	} else {
		return self->left + sizeof(bst_Node) + self->max_value_length > self->right;
	}
}

static bst_Tree*
bst_new(u32 max_value_length, void *buffer, u32 length)
{
	//
	// storage max value needs to fit a next pointer for repeated keys
	//
	max_value_length = RAlign(sizeof(bst_ValueTrailer) + max_value_length,8);
	Assert(max_value_length >= 4); // needs to fit at least an handle
	Assert(length % 8 == 0);
	Assert(length > sizeof(bst_Tree) + sizeof(bst_Node));
	bst_Tree *tree = buffer;
	u32 bottom_handle = sizeof(bst_Tree);
	*tree = (bst_Tree) {
		.node_count = 0,
		.count = 0,
		.root = bottom_handle,
		.free_node = 0,
		.free_value = 0,
		.max_value_length = max_value_length,
		.left = bottom_handle + sizeof(bst_Node),
		.right = length,
		.size = length,
		.pad_ = 0
	};
	bst_Node *bottom = OffsetedPointer(tree, bottom_handle);
	*bottom = (bst_Node) {
		.used = 1,
		.level = 0,
		.count = 0,
		.left = bottom_handle,
		.right = bottom_handle,
		.value = 0,
		.key = 0
	};
	return tree;
}

static u32
bst_bottom_handle(bst_Tree *self)
{
	return sizeof(bst_Tree);
}

static bst_Node*
bst_bottom(bst_Tree *self)
{
	return OffsetedPointer(self, sizeof(bst_Tree));
}

static bst_Node*
bst_root(bst_Tree *self)
{
	return OffsetedPointer(self, self->root);
}

static bst_Node*
bst_nodes(bst_Tree *self)
{
	return OffsetedPointer(self, sizeof(bst_Tree) + sizeof(bst_Node));
}

static bst_Node*
bst_node(bst_Tree *self, u32 node_handle)
{
	return OffsetedPointer(self, node_handle);
}

static bst_Node*
bst_left(bst_Tree *self, bst_Node *node)
{
	return OffsetedPointer(self, node->left);
}

static bst_Node*
bst_right(bst_Tree *self, bst_Node *node)
{
	return OffsetedPointer(self, node->right);
}

static u32
bst_node_handle(bst_Tree *self, bst_Node *node)
{
	Assert((u64)node > (u64)self);
	return PointerDifference(node, self);
}

static void*
bst_value(bst_Tree *self, u32 value_handle)
{
	return RightOffsetedPointer(self, self->size, value_handle);
}

static bst_ValueTrailer*
bst_value_trailer(bst_Tree *self, u32 value_handle)
{
	char *value = bst_value(self, value_handle);
	return (bst_ValueTrailer*) (value + self->max_value_length - sizeof(bst_ValueTrailer));
}

static bst_Node*
bst_new_node_(bst_Tree *self)
{
	bst_Node *new_node = 0;
	if (self->free_node) {
		new_node = OffsetedPointer(self, self->free_node);
		self->free_node = new_node->next_free;
	} else {
		if (self->left + sizeof(bst_Node) > self->right) return 0;
		new_node = OffsetedPointer(self, self->left);
		self->left += sizeof(bst_Node);
	}
	++self->node_count;
	return new_node;
}

static void
bst_free_value_(bst_Tree *self, u32 value_handle)
{
	if (value_handle) {
		void *value = bst_value(self, value_handle);
		platform.memory_set(value, 0, self->max_value_length);
		bst_ValueTrailer *value_trailer = bst_value_trailer(self, value_handle);
		value_trailer->next_free = self->free_value;
		self->free_value = value_handle;
	}
}

static void
bst_free_node_(bst_Tree *self, bst_Node *node)
{
	Assert(node->used);
	Assert(node->count == 1);

	// get data pointer
	bst_free_value_(self, node->value);

	// release node now
	platform.memory_set(node, 0, sizeof(bst_Node));
	node->next_free = self->free_node;
	self->free_node = bst_node_handle(self, node);
	--self->node_count;
	--self->count;
}


static s32
bst_is_bottom(bst_Tree *self, bst_Node *t)
{
	return t == bst_bottom(self);
}

static bst_Node*
bst_skew_(bst_Tree *self, bst_Node *t)
{
	bst_Node *left = bst_left(self,t); // never null by design (sentinel)
	if (left->level == t->level) {
		bst_Node *temp = t;
		t = left;
		temp->left= t->right;
		t->right = bst_node_handle(self, temp);
	}
	return t;
}

static bst_Node*
bst_split_(bst_Tree *self, bst_Node *t)
{
	bst_Node *right = bst_right(self,t); // never null by design (sentinel)
	if (t->level == bst_right(self,right)->level) {
		bst_Node *temp = t;
		t = right;
		temp->right = t->left;
		t->left = bst_node_handle(self, temp);
		++t->level;
	}
	return t;
}

static bst_Node*
bst_insert_(bst_Tree *self, bst_Node *t, u64 key, u32 value_handle)
{
	// root node
	if (bst_is_bottom(self,t)) {
		t = bst_new_node_(self);
		*t = (bst_Node) {
			.used = 1,
			.level = 1,
			.count = 1,
			.left = bst_bottom_handle(self),
			.right = bst_bottom_handle(self),
			.value = value_handle,
			.key = key
		};
		++self->count; // new h,v pair (not sharing a hash)
		return t; // a new node was inserted
	} else {
		if (key != t->key) {
			if (key < t->key) {
				t->left = bst_node_handle(self, bst_insert_(self, bst_left(self, t), key, value_handle));
			} else if (key > t->key) {
				t->right = bst_node_handle(self, bst_insert_(self, bst_right(self, t), key, value_handle));
			}
			t = bst_skew_(self, t);
			t = bst_split_(self, t);
		} else {
			// same key: append to the value list and count
			if (value_handle) {
				bst_ValueTrailer *value_trailer = bst_value_trailer(self, value_handle);
				value_trailer->next = t->value;
				t->value = value_handle; // insert as the head of the linked list
			}
			++t->count;
			++self->count; // another value for an existing key
		}
		return t;
	}
}

static bst_Node*
bst_get_(bst_Tree *self, bst_Node *t, u64 key)
{
	if (bst_is_bottom(self,t)) return 0;
	if (key < t->key) {
		return bst_get_(self, bst_left(self,t), key);
	} else if (key > t->key) {
		return bst_get_(self, bst_right(self,t), key);
	} else {
		return t;
	}
}

static bst_Node*
bst_get(bst_Tree *self, u64 key)
{
	return bst_get_(self, bst_root(self), key);
}


// find the first key in the tree that is >= lb
static bst_Node*
bst_lower_bound_(bst_Tree *self, bst_Node *t, u64 lb)
{
	if (bst_is_bottom(self,t)) return 0;
	if (t->key < lb) {
		return bst_lower_bound_(self, bst_right(self,t), lb);
	} else if (t->key > lb) {
		bst_Node *left_candidate = bst_lower_bound_(self, bst_left(self,t), lb);
		Assert(t->key > lb);
		u64 t_diff = t->key - lb;
		if (left_candidate) {
			Assert(left_candidate->key > lb);
			u64 left_diff = left_candidate->key - lb;
			return (left_diff < t_diff) ? left_candidate : t;
		} else {
			return t;
		}
	} else {
		return t; // same exact value as lb
	}
}

// find the first key in the tree that is >= lb
static bst_Node*
bst_lower_bound(bst_Tree *self, u64 lb)
{
	return bst_lower_bound_(self, bst_root(self), lb);
}

//
// assuming keys are all different for now
//
static s32
bst_insert(bst_Tree *self, u64 key, void *value_buffer, u32 value_length)
{
	Assert((sizeof(bst_ValueTrailer)+value_length) <= self->max_value_length);
	u32 llen = (self->free_node ? (u32) 0 : (u32) sizeof(bst_Node));
	u32 rlen = self->max_value_length;

	// doesn't fit?
	if (self->left + llen + rlen > self->right) { return 0; }

	u32 value_handle = 0;
	if (value_buffer) {
		self->right -= rlen;
		value_handle = self->size - self->right;
		void *dst = bst_value(self, value_handle);
		bst_ValueTrailer *value_trailer = bst_value_trailer(self, value_handle);
		value_trailer->next = 0;
		platform.copy_memory(dst, value_buffer, value_length);
	}

	u32 count_before = self->count;

	self->root = bst_node_handle(self, bst_insert_(self, bst_root(self), key, value_handle));

	return self->count > count_before;
}

typedef struct {
	bst_Tree *tree;
	bst_Node *last;
	bst_Node *deleted;
	u64 key;
} bst_DeleteContext;

static bst_Node*
bst_delete_(bst_DeleteContext *ctx, bst_Node *t)
{
	bst_Tree *tree = ctx->tree;
	bst_Node *bottom = bst_bottom(tree);

	// node key not found (same number of nodes before and after)
	if (bst_is_bottom(tree, t)) { return t; }

	ctx->last = t;
	if (ctx->key < t->key) {
		t->left = bst_node_handle(tree,bst_delete_(ctx, bst_left(tree, t)));
	} else {
		ctx->deleted = t;
		t->right = bst_node_handle(tree,bst_delete_(ctx, bst_right(tree, t)));
	}

	if (t == ctx->last && bottom != ctx->deleted && ctx->key == ctx->deleted->key) {
		if (ctx->deleted->count > 1) {
			if (ctx->deleted->value) {
				// drop the head value
				bst_ValueTrailer *value_trailer = bst_value_trailer(tree,ctx->deleted->value);
				u32 next_value_handle = value_trailer->next;

				// free slot for someone else's data
				bst_free_value_(tree,ctx->deleted->value);

				ctx->deleted->value = next_value_handle;

			}
			--ctx->deleted->count;
			--tree->count;
		} else {
			// copy the key and value from t
			Swap(ctx->deleted->key,t->key);
			Swap(ctx->deleted->value,t->value);
			// swap count field (note that ctx->delete->count is 1)
			ctx->deleted->count = t->count;
			t->count = 1;

			t = bst_node(tree, t->right);

			bst_free_node_(tree, ctx->last);
		}

		ctx->deleted = bottom;

	} else if ( bst_left(tree,t)->level < (t->level-1) || bst_right(tree,t)->level < (t->level-1) ) {

		--t->level;
		if (bst_right(tree,t)->level > t->level)
			bst_right(tree,t)->level = t->level;

		t = bst_skew_(tree,t);

		bst_Node *right=0, *right_right=0;

		right = bst_right(tree,t);
		right = bst_skew_(tree,bst_right(tree,t));
		t->right = bst_node_handle(tree,right);

		right_right = bst_right(tree,right);
		right_right = bst_skew_(tree,right_right);
		right->right = bst_node_handle(tree,right_right);

		t = bst_split_(tree,t);

		right = bst_right(tree,t);
		right = bst_split_(tree,right);
		t->right = bst_node_handle(tree,right);
	}

	return t;

}

static s32
bst_delete(bst_Tree *self, u64 key)
{
	bst_DeleteContext ctx = {
		.tree = self,
		.key = key,
		.last = bst_bottom(self),
		.deleted = bst_bottom(self)
	};

	u32 previous_count = self->node_count;

	self->root = bst_node_handle(self,bst_delete_(&ctx,bst_node(self,self->root)));

	return self->node_count < previous_count;
}






//
// depth first traversal
//

typedef struct {
	u32 node_handle;
	u32 index;
} bst_DFS_Item;

typedef struct {
	bst_Tree *tree;
	u32 stack_size;
	u32 stack_capacity;
} bst_DFS;

static bst_DFS_Item*
bst_dfs_items_(bst_DFS *self)
{
	return OffsetedPointer(self, sizeof(bst_DFS));
}

static void
bst_dfs_push_(bst_DFS *self, u32 node_handle)
{
	Assert(self->stack_size < self->stack_capacity);
	bst_DFS_Item *items = bst_dfs_items_(self);
	items[self->stack_size] = (bst_DFS_Item) { .node_handle = node_handle, .index = 0 };
	++self->stack_size;
}

static void
bst_dfs_pop_(bst_DFS *self)
{
	Assert(self->stack_size > 0);
	--self->stack_size;
}

static bst_DFS_Item*
bst_dfs_top_(bst_DFS *self)
{
	Assert(self->stack_size > 0);
	return bst_dfs_items_(self) + self->stack_size - 1;
}

static bst_DFS*
bst_new_dfs(bst_Tree *tree, void *buffer, u32 buffer_length)
{
	// Assert((u64) buffer % 8 == 0);
	Assert(buffer_length % 8 == 0);
	Assert(buffer_length >= sizeof(bst_DFS) + sizeof(bst_DFS_Item));

	u32 capacity = (buffer_length - sizeof(bst_DFS)) / sizeof(bst_DFS_Item);
	bst_DFS *dfs = buffer;
	*dfs = (bst_DFS) {
		.tree = tree,
		.stack_size = 0,
		.stack_capacity = capacity
	};

	if (tree->node_count) {
		bst_dfs_push_(dfs, tree->root);
	}

	return dfs;
}

static bst_Node*
bst_dfs_next(bst_DFS *self)
{
	u32 bottom_handle = bst_bottom_handle(self->tree);
	while (self->stack_size) {
		bst_DFS_Item *item = bst_dfs_top_(self);
		bst_Node *node = bst_node(self->tree,item->node_handle);
		if (item->index == 0) {
			// try traversing left
			item->index = 1;
			if (node->left != bottom_handle) {
				bst_dfs_push_(self, node->left);
			}
		} else { // if (item->index == 1) {
			Assert(item->index == 1);
			// return this node and schedule the right side
			bst_dfs_pop_(self);
			if (node->right != bottom_handle) {
				bst_dfs_push_(self, node->right);
			}
			return node;
		}
	}
	return 0; // no more nodes
}

static void
bst_dfs_reset(bst_DFS *self)
{
	self->stack_size = 0;
	if (self->tree->node_count) {
		bst_dfs_push_(self, self->tree->root);
	}
}








static void
bst_dot(bst_Tree *self)
{
	fprintf(stderr,"digraph {\n center=true; fontname=Helvetica; ordering=out; node[fontsize=8]; edge[fontsize=8]; \n ");
	bst_Node *bottom = bst_bottom(self);
	bst_Node *it  = bottom + 1;
	bst_Node *end = OffsetedPointer(self, self->left);

	s32 min_level = 1;
	s32 max_level = 1;

	u32  dfs_buffer_length = Kilobytes(1);
	char dfs_buffer[dfs_buffer_length];
	char *dfs_begin = &dfs_buffer[0];
	char *dfs_end   = dfs_begin + dfs_buffer_length;
	dfs_begin = (void*) RAlign((u64)dfs_begin,8);
	dfs_end   = (void*) LAlign((u64)dfs_end,8);
	bst_DFS *dfs = bst_new_dfs(self, dfs_begin, dfs_end - dfs_begin);


	bst_dfs_reset(dfs);
	for (;;) {
		bst_Node* it = bst_dfs_next(dfs);
		if (!it) break;
		Assert(it->used);
		fprintf(stderr,"    n%d [ label=\"l:%d k:%d", bst_node_handle(self,it), it->level, (u32) it->key);
		u32 value_handle = it->value;
		while (value_handle) {
			char *value_buffer =  bst_value(self, value_handle);
			bst_ValueTrailer *value_trailer = bst_value_trailer(self,value_handle);
			fprintf(stderr,"\\n'%s'", value_buffer);
			value_handle = value_trailer->next;
		}
		fprintf(stderr,"\" ]\n");
		max_level = Max(max_level, it->level);
	}

	bst_dfs_reset(dfs);
	for (;;) {
		bst_Node* it = bst_dfs_next(dfs);
		if (!it) break;
		Assert(it->used);
		bst_Node *left  = bst_left(self, it);
		bst_Node *right = bst_right(self, it);
		if (left != bottom) {
			fprintf(stderr,"    n%d -> n%d [ label=\"L\" ]\n", bst_node_handle(self,it), bst_node_handle(self,left));
		}
		if (right != bottom) {
			fprintf(stderr,"    n%d -> n%d [ label=\"R\"; weight=%d ]\n", bst_node_handle(self,it), bst_node_handle(self,right), (it->level == right->level ? 10 : 1));
		}
	}

// 	it  = bst_bottom(self); ++it;
// 	while (it != end) {
// 		if (it->used) {
// 			bst_Node *left  = bst_left(self, it);
// 			bst_Node *right = bst_right(self, it);
// 			if (left != bottom) {
// 				fprintf(stderr,"    n%d -> n%d [ label=\"L\" ]\n", bst_node_handle(self,it), bst_node_handle(self,left));
// 			}
// 			if (right != bottom) {
// 				fprintf(stderr,"    n%d -> n%d [ label=\"R\" ]\n", bst_node_handle(self,it), bst_node_handle(self,right));
// 			}
// 		}
// 		++it;
// 	}

#if 1
	//
	// @todo maybe if we build all this in dfs order, the same rank
	// stuff will with dot. right now it is getting to a segfault
	//
	for (s32 i=min_level;i<=max_level;++i) {

		s32 count = 0;
		{
			bst_dfs_reset(dfs);
			for (;;) {
				bst_Node* it = bst_dfs_next(dfs);
				if (!it) break;
				Assert(it->used);
				if (it->level == i) {
					++count;
				}
			}
		}
		if (count <= 1) continue;

		{
			fprintf(stderr, "     { rank=same; ");
			bst_dfs_reset(dfs);
			s32 first = 1;
			for (;;) {
				bst_Node* it = bst_dfs_next(dfs);
				if (!it) break;
				Assert(it->used);
				if (it->level == i) {
					if (!first) {
						fprintf(stderr,", ");
					}
					fprintf(stderr,"n%d",bst_node_handle(self,it));
					first = 0;
				}
			}
			fprintf(stderr, "     }\n");
		}

		{
			fprintf(stderr, "     ");
			bst_dfs_reset(dfs);
			s32 first = 1;
			for (;;) {
				bst_Node* it = bst_dfs_next(dfs);
				if (!it) break;
				Assert(it->used);
				if (it->level == i) {
					if (!first) {
						fprintf(stderr," -> ");
					}
					fprintf(stderr,"n%d",bst_node_handle(self,it));
					first = 0;
				}
			}
			fprintf(stderr, "  [ style=invis ] \n");
		}
	}
#endif

	fprintf(stderr,"}\n");
}

static u32
bst_size_for_capacity(u32 value_length, u32 capacity)
{
	u32 max_value_length = RAlign(value_length + sizeof(bst_ValueTrailer), 8);
	return sizeof(bst_Tree) + sizeof(bst_Node) * capacity + max_value_length * capacity;
}

static void
bst_log(bst_Tree *self)
{
	fprintf(stderr,"// [bst_Tree] { .node_count=%d .root=%d .free_node=%d .free_value=%d .max_value_length=%d .left=%d .right=%d .size=%d }\n",
		self->node_count,
		self->root,
		self->free_node,
		self->free_value,
		self->max_value_length,
		self->left,
		self->right,
		self->size);

	bst_Node *it  = bst_bottom(self);
	bst_Node *end = OffsetedPointer(self, self->left);
	while (it != end) {
		if (it->used) {
			fprintf(stderr,"//    [bst_Node] { .addr=%d .used=%d .level=%d .count=%d .left=%d .right=%d .value=%d .key=%"PRIu64" .value=",
				bst_node_handle(self,it), it->used, it->level, it->count, it->left, it->right, it->value, it->key);
			u32 value_handle = it->value;
			while (value_handle) {
				char *value_buffer = bst_value(self,value_handle);
				bst_ValueTrailer *value_trailer = bst_value_trailer(self,value_handle);
				fprintf(stderr,"'%s' ", value_buffer);
				value_handle = value_trailer->next;
			}
			fprintf(stderr," }\n");
		} else {
			fprintf(stderr,"//    [bst_Node] { .addr=%d .used=%d .next_free=%d }\n", bst_node_handle(self,it), it->used, it->next_free);
		}
		++it;
	}
	bst_dot(self);
}



#ifdef bst_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

#include "arena.c"
#include "cstr.c"
#include "print.c"

#include "../platform_dependent/nix_platform.c"

// every two names have the same key
#define MULT 2

static s32 order [] = {
	29,
	13,
	44,
	19,
	96,
	79,
	66,
	25,
	28,
	17,
	9,
	90,
	24,
	20,
	67,
	80,
	0,
	53,
	51,
	11,
	6,
	61,
	1,
	83,
	54,
	97,
	62,
	85,
	27,
	95,
	75,
	56,
	84,
	30,
	60,
	58,
	93,
	87,
	72,
	78,
	16,
	86,
	63,
	39,
	57,
	92,
	99,
	88,
	12,
	81,
	50,
	21,
	89,
	3,
	55,
	77,
	69,
	45,
	43,
	47,
	23,
	76,
	70,
	82,
	2,
	36,
	7,
	41,
	4,
	26,
	10,
	94,
	40,
	32,
	64,
	34,
	8,
	31,
	5,
	15,
	59,
	49,
	22,
	33,
	14,
	98,
	38,
	74,
	35,
	91,
	46,
	48,
	65,
	68,
	37,
	18,
	42,
	52,
	73,
	71
};

static char *names [] = {
	"aaron",
	"adam",
	"adrian",
	"alan",
	"albert",
	"alex",
	"alexander",
	"alfred",
	"allen",
	"alvin",
	"andre",
	"andrew",
	"angel",
	"anthony",
	"antonio",
	"arnold",
	"arthur",
	"barry",
	"ben",
	"benjamin",
	"bernard",
	"bill",
	"billy",
	"bobby",
	"brad",
	"bradley",
	"brandon",
	"brent",
	"brett",
	"brian",
	"bruce",
	"bryan",
	"calvin",
	"carl",
	"carlos",
	"cecil",
	"chad",
	"charles",
	"charlie",
	"chester",
	"chris",
	"christopher",
	"clarence",
	"claude",
	"clifford",
	"clyde",
	"corey",
	"cory",
	"craig",
	"curtis",
	"dale",
	"dan",
	"daniel",
	"danny",
	"darrell",
	"darryl",
	"david",
	"dean",
	"dennis",
	"derek",
	"derrick",
	"don",
	"donald",
	"douglas",
	"duane",
	"dustin",
	"earl",
	"eddie",
	"edgar",
	"edward",
	"edwin",
	"elmer",
	"eric",
	"erik",
	"ernest",
	"eugene",
	"floyd",
	"francis",
	"francisco",
	"frank",
	"franklin",
	"fred",
	"frederick",
	"gabriel",
	"gary",
	"gene",
	"george",
	"gerald",
	"gilbert",
	"glen",
	"glenn",
	"gordon",
	"greg",
	"gregory",
	"harold",
	"harry",
	"harvey",
	"hector",
	"henry",
	"herbert"
};


// ids for 0 to 49 appearing twice each
// in a shuffled order
static s32 delete[] = {
	25,
	34,
	37,
	19,
	30,
	6,
	18,
	47,
	34,
	21,
	18,
	15,
	36,
	49,
	12,
	2,
	11,
	3,
	40,
	30,
	9,
	28,
	27,
	3,
	16,
	45,
	25,
	49,
	37,
	11,
	4,
	13,
	26,
	35,
	5,
	36,
	16,
	41,
	1,
	32,
	22,
	1,
	29,
	20,
	29,
	42,
	15,
	9,
	4,
	38,
	27,
	19,
	24,
	23,
	17,
	44,
	12,
	43,
	48,
	48,
	47,
	2,
	8,
	14,
	35,
	17,
	44,
	20,
	22,
	39,
	7,
	43,
	13,
	33,
	10,
	7,
	28,
	0,
	23,
	26,
	46,
	10,
	5,
	32,
	45,
	24,
	14,
	31,
	46,
	6,
	42,
	41,
	21,
	40,
	38,
	39,
	31,
	0,
	8,
	33
};

#define names_length ArrayCount(names)
#define delete_length ArrayCount(delete)

int main(int argc, char *argv[])
{
	nix_init_platform(&platform);

	u32 buffer_length = Megabytes(1);
	void *buffer = malloc(buffer_length);

	bst_Tree *tree = bst_new(32,buffer,buffer_length);

	fprintf(stderr,"// names_length %d\n", (s32) names_length);
	fprintf(stderr,"// delete_length %d\n", (s32) delete_length);


#if 1
	// insert all points
	for (s32 i=0;i<names_length;++i) {
		s32 j = order[i];
		bst_insert(tree, j/MULT, names[j], cstr_len(names[j])+1); // include the 0 when inserting
		fprintf(stderr, "// ...... After inserting %d '%s' ......\n", j, names[j]);
		bst_log(tree);
	}

	for (s32 i=0;i<delete_length;++i) {
		bst_delete(tree, delete[i]);
		fprintf(stderr, "// ...... After deleting %d '%s' ......\n", i, names[delete[i]]);
		bst_log(tree);
	}
#else
	// insert all points
	bst_insert(tree, 0, names[0], cstr_len(names[0]));
	bst_log(tree);
#endif
}

#endif



