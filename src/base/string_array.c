
//------------------------------------------------------------------------------
//
// StringArray:
//     - double it if we need more
//     - pack it when we finish
//
// [ StringArray ] [ text ] ----> <---- [ pos ]
//
//------------------------------------------------------------------------------

typedef struct {
	u32  count;
	u32  left;
	u32  right;
	u32  length;
	char text[];
} StringArray;

//
// if init != 0, then copy content from 'init' string array
//
static StringArray*
string_array_init(void *buffer, u32 length, StringArray *init)
{
	Assert(length >= sizeof(StringArray));
	AssertMultiple(buffer,8);
	AssertMultiple(length,8);

	StringArray *result = buffer;
	if (init == 0) {
		result[0] = (StringArray) {
			.count = 0,
			.left = sizeof(StringArray),
			.right = length,
			.length = length
		};
	} else {
		// returns 0 if it
		u32 pos_array_length = init->length - init->right;
		u32 required_space = RAlign(init->left, 4) + pos_array_length;
		if (required_space > length) {
			return 0;
		}
		platform.copy_memory(buffer, init, init->left);
		StringArray *result = buffer;
		result->length = length;
		result->right  = length - pos_array_length;
		platform.memory_copy(OffsetedPointer(result,result->right),OffsetedPointer(init,init->right),pos_array_length);
	}

	return result;
}

static u32
string_array_available_storage(StringArray *self)
{
	if (self->left + sizeof(u32) >= self->right) return 0;
	else return self->right - self->left - sizeof(u32);
}

static u32
string_array_push(StringArray *self, void *string, u32 string_length)
{
	u32 storage_required = string_length + sizeof(u32);
	if (self->left + storage_required > self->right) {
		return 0;
	}

	self->right -= sizeof(u32);
	u32 *p = OffsetedPointer(self,self->right);
	p[0] = self->left;
	//
	// NOTE this is not the convention dst <- src, length
	//
	pt_copy_bytesn(string, OffsetedPointer(self,self->left), string_length);
	++self->count;
	self->left += string_length;

	return 1;
}

//
// returns new length
//
static u32
string_array_pack(StringArray *self)
{
	u32 new_right = RAlign(self->left,4);
	if (new_right < self->right) {
		u32 pos_array_length = self->length - self->right;
		pt_copy_bytesn(OffsetedPointer(self,new_right), OffsetedPointer(self,self->right), pos_array_length);
		self->right = new_right;
		self->length = self->right + pos_array_length;
	}
	return self->length;
}

