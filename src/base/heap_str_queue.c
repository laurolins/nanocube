// heap string queue

//
// next head should be at location pointer by right
//
typedef struct {
	u32 start;           // offset of the start
	u32 length;          // length
	u32 content_start;   // offset of the start
	u32 content_length;  // length
} hsq_Item;

typedef struct {
	u32 left;
	u32 right;
	u32 cap;
	u32 buffer_size;
} hsq_Queue;

_Static_assert(sizeof(hsq_Queue) % 8 == 0, "hsq_Queue_invalid_size");
_Static_assert(sizeof(hsq_Item) % 8 == 0, "hsq_Item_invalid_size");

#define hsq_Queue_empty(self) (self->left == self->right)
#define hsq_Queue_full(self) ((self->right + 1) % self->cap == self->left)
#define hsq_Queue_items(self) ((hsq_Item*) ((char*) self + sizeof(hsq_Queue)))
#define hsq_Queue_buffer(self) ((char*) self + sizeof(hsq_Queue) + self->cap * sizeof(hsq_Item))

static hsq_Queue*
hsq_new_queue(Arena *arena, u32 item_capacity, u32 size)
{
	Assert(size % 8 == 0);
	if (size == 0) { size = Megabytes(1); }
	Assert(size >= sizeof(hsq_Queue) + item_capacity * sizeof(hsq_Item) + Kilobytes(16));
	void *buffer = Arena_push(arena, size, 8, 0);
	hsq_Queue *queue = buffer;
	u32 buffer_size = size - sizeof(hsq_Queue) - item_capacity * sizeof(hsq_Item);
	*queue = (hsq_Queue) { .left=0, .right=0, .cap = item_capacity, .buffer_size = buffer_size };
	return queue;
}

static hsq_Item*
hsq_Queue_first(hsq_Queue *self)
{
	u32 index = 0;
	if (hsq_Queue_empty(self)) {
		return 0;
	} else {
		return hsq_Queue_items(self) + self->left;
	}
}

static hsq_Item*
hsq_Queue_last(hsq_Queue *self)
{
	u32 index = 0;
	if (hsq_Queue_empty(self)) {
		return 0;
	} else if (self->right > 0) {
		index = self->right - 1;
	} else {
		index = self->cap - 1;
	}
	return hsq_Queue_items(self) + index;
}

#define hsq_ENQUEUE_RESULT_OK 0
#define hsq_ENQUEUE_RESULT_ITEMS_FULL 1
#define hsq_ENQUEUE_RESULT_STRING_DOESNT_FIT 2

#define hsq_PADDED_LENGTH(len) RALIGN(len+1,8)

static hsq_Item*
hsq_Queue_enqueue_item_(hsq_Queue *self, hsq_Item item)
{
	Assert(!hsq_Queue_full(self));
	hsq_Item *result = hsq_Queue_items(self) + self->right;
	*result = item;
	self->right = (self->right + 1) % self->cap;
	return result;
}

static void
hsq_Queue_dequeue_item_(hsq_Queue *self)
{
	Assert(self->left != self->right);
	self->left = (self->left + 1) % self->cap;
	if (self->left == self->right) {
		self->left = self->right = 0;
	}
}

static void
hsq_Queue_dequeue_last_item_(hsq_Queue *self)
{
	Assert(self->left != self->right);
	self->right = (self->right > 0) ? (self->right-1) : (self->cap-1);
	if (self->left == self->right) {
		self->left = self->right = 0;
	}
}

static s32
hsq_Queue_enqueue(hsq_Queue *self, char *str, u32 length)
{
	if (hsq_Queue_full(self)) { return hsq_ENQUEUE_RESULT_ITEMS_FULL; }

	u32 padded_len = hsq_PADDED_LENGTH(length);

#define hsq_ITEM(A,B,C,D) \
	hsq_Queue_enqueue_item_(self,  (hsq_Item) { .start = (A), .length = (B), \
			.content_start = (C), .content_length = (D)})

	hsq_Item *new_item = 0;
	if (!hsq_Queue_empty(self)) {
		hsq_Item *first = hsq_Queue_first(self);
		hsq_Item *last  = hsq_Queue_last(self);

		u32 used_begin = first->start;
		u32 used_end   = (last->start + last->length) % self->buffer_size;

		if (used_begin < used_end) {
			if (used_end + padded_len <= self->buffer_size) {
				new_item = hsq_ITEM(used_end, padded_len, used_end, length);
			} else if (padded_len <= used_begin) {
				new_item = hsq_ITEM(used_end, padded_len + (self->buffer_size - used_end), 0, length);
			} else {
				return hsq_ENQUEUE_RESULT_STRING_DOESNT_FIT;
			}
		} else {
			if (used_end + padded_len <= used_begin) {
				new_item = hsq_ITEM(used_end, padded_len, used_end, length);
			} else {
				return hsq_ENQUEUE_RESULT_STRING_DOESNT_FIT;
			}
		}
	} else {
		// empty case
		Assert(self->left == self->right && self->right == 0);
		if (padded_len >= self->buffer_size) {
			return hsq_ENQUEUE_RESULT_STRING_DOESNT_FIT;
		} else {
			new_item = hsq_ITEM(0, padded_len, 0, length);
		}
	}
	Assert(new_item);
	char *buffer = hsq_Queue_buffer(self);
	platform.memory_copy(buffer + new_item->start, str, length);
	buffer[new_item->start + length] = 0;

#undef hsq_ITEM

	return hsq_ENQUEUE_RESULT_OK;

}

static void
hsq_Queue_dequeue(hsq_Queue *self)
{
	Assert(!hsq_Queue_empty(self));
	hsq_Queue_dequeue_item_(self);
}

static void
hsq_Queue_dequeue_last(hsq_Queue *self)
{
	Assert(!hsq_Queue_empty(self));
	hsq_Queue_dequeue_last_item_(self);
}



static char*
hsq_Queue_last_cstr(hsq_Queue *self)
{
	Assert(!hsq_Queue_empty(self));
	u32 index = (self->right > 0) ? (self->right-1) : (self->cap-1);
	hsq_Item *items = hsq_Queue_items(self);
	char *buffer = hsq_Queue_buffer(self);
	return buffer + items[index].content_start;
}

