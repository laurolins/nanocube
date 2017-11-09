/*
AA tree data structure.

Assumptions:

(A1) At most 2^58 elements.
(A2) User provides an 58-bit key (0 to 288,230,376,151,711,744) and a void* data element.
(A3) Repeated keys are not allowed. If the same key is inserted twice,
     info is returned pointing to the existing data element. A user can
     encode some mechanism to deal with duplicate keys in the void*
     data field.

Notes:

(N1) No deletion is implemented yet

*/

// #define aatree_UNIT_TEST

#ifdef aatree_UNIT_TEST
#include "platform.c"
#endif

typedef struct aatree_Node aatree_Node;
typedef struct aatree_Tree aatree_Tree;
typedef aatree_Node *aatree_Node_Ptr;

struct aatree_Node {
	u64    key: 58;
	u64    level: 6;
	struct aatree_Node *left;
	struct aatree_Node *right;
	void   *data;
};

struct aatree_Tree {
	struct {
		aatree_Node *begin;
		aatree_Node *end;
		aatree_Node *capacity;
	} data;
	aatree_Node *root;
};

internal void
aatree_Tree_init(aatree_Tree *self, aatree_Node *begin, aatree_Node *capacity)
{
	self->data.begin    = begin;
	self->data.end      = begin;
	self->data.capacity = capacity;
	self->root = 0;
}


#define aatree_INSERT_IN_PROGRESS         0
#define aatree_INSERT_OK                  1
#define aatree_INSERT_KEY_ALREADY_EXISTS -1
#define aatree_INSERT_FULL               -2

internal inline aatree_Node*
aatree_Tree_append(aatree_Tree *self, u64 key, void *data)
{
	if (self->data.end != self->data.capacity) {
		aatree_Node *new_node = self->data.end;
		++self->data.end;
		*new_node = (aatree_Node) { .key = key, .level = 1, .left = 0, .right = 0, .data = data };
		return new_node;
	} else {
		return 0;
	}
}

internal aatree_Node*
aatree_skew(aatree_Node *node)
{
	if (node->left && node->left->level == node->level) {
		/* rotate right */
		aatree_Node *aux = node;
		node = node->left;
		aux->left  = node->right;
		node->right  = aux;
	}
	return node;
}

internal aatree_Node*
aatree_split(aatree_Node *node)
{
	if (node->right && node->right->right && node->right->right->level == node->level) {
		/* rotate left */
		aatree_Node *aux = node;
		node = node->right;
		aux->right = node->left;
		node->left = aux;
		++node->level;
	}
	return node;
}

internal aatree_Node*
aatree_insert_node(aatree_Tree *tree, aatree_Node *node, u64 key, void *data, s32 *status, aatree_Node_Ptr *result)
{
	if (node) {
		if (key < node->key) {
			node->left = aatree_insert_node(tree, node->left, key, data, status, result);
		} else if (key > node->key) {
			node->right = aatree_insert_node(tree, node->right, key, data, status, result);
		} else {
			*status = aatree_INSERT_KEY_ALREADY_EXISTS;
			*result = node;
			return node;
		}
		node = aatree_skew(node);
		node = aatree_split(node);
		return node;
	} else {
		// new node
		node =  aatree_Tree_append(tree, key, data);
		*result = node;
		*status = node ? aatree_INSERT_OK : aatree_INSERT_FULL;
		return node;
	}
}

internal void
aatree_Tree_clear(aatree_Tree *self)
{
	self->data.end = self->data.begin;
	self->root = 0;
}

internal aatree_Node*
aatree_Tree_insert(aatree_Tree *self, u64 key, void *data, s32 *status)
{
	aatree_Node *result = 0;
	*status = aatree_INSERT_IN_PROGRESS;
	self->root = aatree_insert_node(self, self->root, key, data, status, &result);
	return result;
}

internal aatree_Node*
aatree_Tree_find(aatree_Tree *self, u64 key)
{
	aatree_Node *node = self->root;
	while (node) {
		if (key < node->key) {
			node = node->left;
		} else if (key > node->key) {
			node = node->right;
		} else {
			return node;
		}
	}
	return 0;
}

internal void
aatree_print_subtree(aatree_Node *node, Print *print, u64 depth)
{
	if (!node)
		return;
	if (node->right)
		aatree_print_subtree(node->right, print, depth + 1);

	Print_nchar(print, '.', depth * 12);
	Print_format(print, "| %llu\n", node->key);

	if (node->left)
		aatree_print_subtree(node->left, print, depth + 1);
}

internal void
aatree_Tree_print(aatree_Tree *self, Print *print)
{
	aatree_print_subtree(self->root, print, 0);
}

#ifdef aatree_UNIT_TEST

// Use bash script utest.sh

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	aatree_Node nodes[100];
	aatree_Tree tree;
	aatree_Tree_init(&tree, nodes, nodes + 100);

	// u64 keys[] = { 10, 5, 6, 12, 13, 1 };

	s32 status = 0;
	for (s32 i=1;i<=13;++i) {
		if (i != 6 && i != 7)
			aatree_Tree_insert(&tree, i, 0, &status);
	}
	aatree_Tree_insert(&tree, 7, 0, &status);
	aatree_Tree_insert(&tree, 6, 0, &status);

	static const u64 buffer_size = Megabytes(4);
	char *buffer = (char*) malloc(buffer_size);
	Print print;
	Print_init(&print, buffer, buffer + buffer_size);

	aatree_Tree_print(&tree, &print);
	write(fileno(stdout), print.begin, print.end - print.begin);
	// fsync(stderr);
}

#endif


