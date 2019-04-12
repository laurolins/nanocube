//
// create association tables that map
// a key with zero or more action strings
//
// let actions strings within a table be associated with handlers (function and context)
//
// given an active table that is in context let there be a trigger(key)
// function that results in an object to iterate through all the handlers
// associated to the appropriate actions
//
// why is this interesting? code gets more readable and easier to configure
// from text files containing the actions
//
//
//
//                            while building
// [ km_KeyMap ] [ tables ] [ actions        ]        [ table_entries ] [ keymapped ]
//
// each table points to two arrays
//
//    [ entries ] [ actions ]
//
// entries are sorted by key
// actions are sorted by name
// entries point to a chain of actions
//
// if we could first parse the keys and then the actions it
// would be a more natural ordering and would simplify the construction
// of the table
//
// that's it: assume a two pass procedure
//

#define km_HANDLER(name) void name(void *context, void *trigger)
typedef km_HANDLER(km_Handler);

//
// # main table of actions
// - main
// q quit
// h help
//
// # redirect to other tables on keys that are not present
// - naviation : secondary main
// z zoom_in play_sound
// x zoom_out
//

typedef u32 km_LeftOffset;
typedef u32 km_RightOffset;

typedef struct km_HandlerEntry km_HandlerEntry;
struct km_HandlerEntry {
	void            *context; // context
	km_Handler      *handler; // pointer to a function
	km_RightOffset  next;
	s32             padding;
};

// make it null terminated always
typedef struct {
	u32 length;
} km_String;

typedef struct {
	km_LeftOffset  table;
	km_RightOffset forward;
} km_Forward;

typedef struct {
	km_RightOffset name;
	km_RightOffset handler_chain; // left ptr to handler (0 means nothing)
} km_Action;

typedef struct {
	km_RightOffset name;         // we will sort the Mapped Keys
	km_RightOffset actions_offsets;
	u32            num_actions;
	u32            padding;
} km_Key;

typedef struct {
	km_RightOffset name;         // pointer to the right
	km_RightOffset keys;
	u32            num_keys;  // number of keys
	km_RightOffset actions;     // pointer to the actions array
	u32            num_actions; // name of the action
	km_RightOffset forward;      // offset
} km_Table;

//
// km_KeyMap_set_active_table(km,table_name);
// km_KeyMap_set_handler(km,action_name,handler,context)
// multiple actions per key?
//
// a table that registers an event overrides the previous table
// only if it can't find the key on that table it can go to other tables
// concept of the active table
//

// first pass is when tables and table entries are inserted
#define km_STATUS_FIRST_PASS 0
// second pass is when actions are inserted
#define km_STATUS_SECOND_PASS 1
// associate actions to keys
#define km_STATUS_THIRD_PASS 2
// ready to register handlers and get into action
#define km_STATUS_READY      3

typedef struct {
	u32            left;
	u32            right;
	u32            size;
	u32            num_tables;
	km_LeftOffset  active_table;
	km_RightOffset source;      // offset
	u32            status;
	u32            padding;
} km_KeyMap;

_Static_assert(sizeof(km_KeyMap) % 8 == 0, "km_KeyMap_invalid_size");
_Static_assert(sizeof(km_Table) % 8 == 0, "km_Table_invalid_size");
_Static_assert(sizeof(km_Key) % 8 == 0, "km_Key_invalid_size");

static km_KeyMap*
km_new_keymap_raw(u32 size)
{
	if (size == 0) {
		size = Kilobytes(16);
	}
	void *buffer = platform.allocate_memory_raw(size,0);
	km_KeyMap *keymap = buffer;
	*keymap = (km_KeyMap) {
		.left = sizeof(km_KeyMap),
		.right = size,
		.size = size,
		.num_tables = 0,
		.active_table = 0,
		.status = km_STATUS_FIRST_PASS,
		.source = 0
	};
	return keymap;
}

static km_String*
km_KeyMap_string(km_KeyMap *self, km_RightOffset offset)
{
	Assert(offset);
	return RightOffsetedPointer(self, self->size, offset);
}

static char*
km_String_value(km_String *self)
{
	return OffsetedPointer(self, sizeof(km_String));
}

static km_Table*
km_KeyValue_tables(km_KeyMap *self)
{
	return OffsetedPointer(self, sizeof(km_KeyMap));
}

static s32
km_KeyMap_store_source(km_KeyMap *self, char *text, u32 length)
{
	if (self->right < self->size) {
		fputs("source should be the first thing into a keymap\n",stderr);
		exit(-1);
	}
	u32 right_demand = RAlign(sizeof(km_String) + length + 1,8);
	if (self->left + right_demand > self->right) {
		return 0;
	}
	self->right -= right_demand;
	km_String *str = OffsetedPointer(self, self->right);
	str->length = length;
	char *dst = km_String_value(str);
	platform.memory_copy(dst, text, length);
	dst[length] = 0;
	self->source = self->size - self->right;
	return 1;
}

static char*
km_KeyMap_source_cstr(km_KeyMap *self)
{
	if (!self->source) {
		return 0;
	}
	km_String *str = km_KeyMap_string(self, self->source);
	return km_String_value(str);
}

static
PLATFORM_SORT_COMPARE(km_compare_keys)
{
	km_KeyMap *km = context;
	km_Key *aa = (km_Key*) a;
	km_Key *bb = (km_Key*) b;

	km_String* stra = km_KeyMap_string(km, aa->name);
	km_String* strb = km_KeyMap_string(km, bb->name);

	char *ap = km_String_value(stra);
	char *bp = km_String_value(strb);

	return cstr_compare_memory(ap, ap + stra->length, bp, bp + strb->length);
}

static
PLATFORM_SORT_COMPARE(km_compare_actions)
{
	km_KeyMap *km = context;
	km_Action *aa = (km_Action*) a;
	km_Action *bb = (km_Action*) b;

	km_String* stra = km_KeyMap_string(km, aa->name);
	km_String* strb = km_KeyMap_string(km, bb->name);

	char *ap = km_String_value(stra);
	char *bp = km_String_value(strb);

	return cstr_compare_memory(ap, ap + stra->length, bp, bp + strb->length);
}

static void
km_KeyMap_move_active_table_entries_from_left_to_right_and_sort_(km_KeyMap *self)
{
	if (!self->active_table) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	km_Table *table = OffsetedPointer(self, self->active_table);

	// entries base is here
	void *src = OffsetedPointer(table,sizeof(km_Table));
	u32   len = table->num_keys * sizeof(km_Key);
	void *dst = OffsetedPointer(self, self->right - len);
	platform.memory_move(dst, src, len);
	self->right -= len;
	table->keys = self->size - self->right;
	self->left = self->active_table + sizeof(km_Table);

	platform.sort(dst, table->num_keys, sizeof(km_Key), km_compare_keys, self);
}

static void
km_KeyMap_move_active_table_actions_from_left_to_right_and_sort_(km_KeyMap *self)
{
	if (!self->active_table) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	km_Table *table = OffsetedPointer(self, self->active_table);

	km_Action *actions = OffsetedPointer(self, sizeof(km_KeyMap) + self->num_tables * sizeof(km_Table));

	// entries base is here
	void *src = actions;
	u32   len = table->num_actions * sizeof(km_Action);
	void *dst = OffsetedPointer(self, self->right - len);
	platform.memory_move(dst, src, len);
	self->right -= len;
	table->actions = self->size - self->right;
	self->left = sizeof(km_KeyMap) + self->num_tables * sizeof(km_Table);

	platform.sort(dst, table->num_actions, sizeof(km_Action), km_compare_actions, self);
}

static void
km_KeyMap_finish_first_pass_(km_KeyMap *self)
{
	Assert(self->status == km_STATUS_FIRST_PASS);
	if (self->active_table) {
		km_KeyMap_move_active_table_entries_from_left_to_right_and_sort_(self);
	}
	self->active_table = 0;
	self->status = km_STATUS_SECOND_PASS;
}

static void
km_KeyMap_finish_third_pass_(km_KeyMap *self)
{
	Assert(self->status == km_STATUS_THIRD_PASS);
	// activate first table
	if (self->num_tables > 0) {
		self->active_table = sizeof(km_KeyMap);
	} else {
		self->active_table = 0;
	}
	self->status = km_STATUS_READY;
}

static void
km_KeyMap_finish_second_pass_(km_KeyMap *self)
{
	Assert(self->status == km_STATUS_SECOND_PASS);
	if (self->active_table) {
		km_KeyMap_move_active_table_actions_from_left_to_right_and_sort_(self);
	}
	self->active_table = 0;
	self->status = km_STATUS_THIRD_PASS;
}

//
// returns zero if no more tables are available
//
static s32
km_KeyMap_goto_next_table_second_pass_(km_KeyMap *self)
{
	Assert(self->status == km_STATUS_SECOND_PASS);
	km_Table *table_array = OffsetedPointer(self, sizeof(km_KeyMap));
	if (self->active_table == 0) {
		self->active_table  = sizeof(km_KeyMap);
	} else {

		//
		// transfer actions of table at active table to
		// the right and update the table actions
		//
		km_KeyMap_move_active_table_actions_from_left_to_right_and_sort_(self);

		self->active_table += sizeof(km_Table);
	}
	km_Table *table = OffsetedPointer(self, self->active_table);
	s64 index = table - table_array;
	if (index < self->num_tables) {
		return 1;
	} else if (index == self->num_tables) {
		return 0;
	} else {
		fputs("Invalid Path\n",stderr);
		exit(-1);
	}
}

//
// returns zero if no more tables are available
//
static s32
km_KeyMap_goto_next_table_third_pass_(km_KeyMap *self)
{
	Assert(self->status == km_STATUS_THIRD_PASS);
	km_Table *table_array = OffsetedPointer(self, sizeof(km_KeyMap));
	if (self->active_table == 0) {
		self->active_table  = sizeof(km_KeyMap);
	} else {
		self->active_table += sizeof(km_Table);
	}
	km_Table *table = OffsetedPointer(self, self->active_table);
	s64 index = table - table_array;
	if (index < self->num_tables) {
		return 1;
	} else if (index == self->num_tables) {
		return 0;
	} else {
		fputs("Invalid Path\n",stderr);
		exit(-1);
	}
}

static s32
km_KeyMap_push_table(km_KeyMap *self, void *name, u32 name_length)
{
	Assert(self->status == km_STATUS_FIRST_PASS);

	if (self->active_table) {
		km_KeyMap_move_active_table_entries_from_left_to_right_and_sort_(self);
	}

	u32 right_demand = RAlign(sizeof(km_String) + name_length + 1,8);
	u32 left_demand = sizeof(km_Table);
	if (self->left + left_demand + right_demand > self->right) {
		return 0;
	}

	// write the name of the table on the right side
	km_String *str = OffsetedPointer(self, self->right - right_demand);
	self->right   -= right_demand;
	str->length    = name_length;
	char *buffer   = OffsetedPointer(str, sizeof(km_String));
	platform.memory_copy(buffer, name, name_length);
	buffer[name_length] = 0;

	//
	km_Table  *table = OffsetedPointer(self, self->left);
	self->active_table = self->left;
	self->left += sizeof(km_Table);
	*table = (km_Table) {
		.name = self->size - self->right,
		.keys= 0,
		.num_keys = 0,
		.actions = 0,
		.num_actions= 0,
		.forward = 0
	};

	++self->num_tables;

	return 1;
}

static km_Table*
km_KeyMap_active_table(km_KeyMap *self)
{
	Assert(self->active_table);
	return OffsetedPointer(self, self->active_table);
}

static km_Key*
km_KeyMap_keys(km_KeyMap *self, km_Table *table)
{
	Assert(self->active_table);
	Assert(table->keys);
	return RightOffsetedPointer(self, self->size, table->keys);
}

static km_Action*
km_KeyMap_actions(km_KeyMap *self, km_Table *table)
{
	Assert(self->active_table);
	Assert(table->keys);
	return RightOffsetedPointer(self, self->size, table->actions);
}

static km_RightOffset
km_KeyMap_key_by_name(km_KeyMap *self, char *key, u32 len)
{
	km_Table *table = km_KeyMap_active_table(self);
	// @cleanup linear search for now replace with binary search
	km_Key *keys = km_KeyMap_keys(self, table);
	for (s32 i=0;i<table->num_keys;++i) {
		km_String *key_str = km_KeyMap_string(self,keys[i].name);
		char *key_raw = km_String_value(key_str);
		if (cstr_compare_memory(key, key + len, key_raw, key_raw + key_str->length) == 0) {
			return ((char*) self + self->size) - (char*) (keys + i);
		}
	}
	return 0;
}

static km_RightOffset
km_KeyMap_action_by_name(km_KeyMap *self, char *name, u32 len)
{
	km_Table *table = km_KeyMap_active_table(self);
	// @cleanup linear search for now replace with binary search
	km_Action *actions = km_KeyMap_actions(self, table);
	for (s32 i=0;i<table->num_actions;++i) {
		km_String *name_str = km_KeyMap_string(self,actions[i].name);
		char *name_raw = km_String_value(name_str);
		if (cstr_compare_memory(name, name + len, name_raw, name_raw + name_str->length) == 0) {
			return ((char*) self + self->size) - (char*) (actions + i);
		}
	}
	return 0;
}

static km_RightOffset
km_KeyMap_action_by_name_cstr(km_KeyMap *self, char *name)
{
	return km_KeyMap_action_by_name(self, name, cstr_length(name));
}


static km_HandlerEntry*
km_KeyMap_handler_by_action_name(km_KeyMap *self, char *name, u32 len)
{
	km_Table *table = km_KeyMap_active_table(self);

	km_RightOffset action_id = km_KeyMap_action_by_name(self, name, len);
	if (!action_id) { return 0; }

	km_Action *action = RightOffsetedPointer(self, self->size, action_id);

	if (!action->handler_chain) { return 0; }

	km_HandlerEntry *entry = RightOffsetedPointer(self, self->size, action->handler_chain);

	return entry;
}

static km_HandlerEntry*
km_KeyMap_handler_by_action_name_cstr(km_KeyMap *self, char *name)
{
	return km_KeyMap_handler_by_action_name(self, name, cstr_length(name));
}

static km_HandlerEntry*
km_KeyMap_next_handler(km_KeyMap *self, km_HandlerEntry *current)
{
	if (!current) return 0;
	if (!current->next) return 0;
	km_HandlerEntry *next = RightOffsetedPointer(self, self->size, current->next);
	return next;
}

static s32
km_KeyMap_register_handler(km_KeyMap *self, char *name_cstr, km_Handler *callback, void *context)
{
	char *name = name_cstr;
	u32  name_length = cstr_length(name);
	if (self->status != km_STATUS_READY) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	if (self->active_table == 0) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	if (self->left + sizeof(km_HandlerEntry) > self->right) {
		return 0;
	}

	km_RightOffset action_offset = km_KeyMap_action_by_name(self, name, name_length);

	if (action_offset) {

		// found the right action
		km_Action *action = RightOffsetedPointer(self, self->size, action_offset);

		// reserve right
		self->right -= sizeof(km_HandlerEntry);
		km_HandlerEntry *handler_entry = OffsetedPointer(self, self->right);
		*handler_entry = (km_HandlerEntry) {
			.context = context,
				.handler = callback,
				.next = 0,
				.padding = 0
		};

		km_RightOffset *slot = &action->handler_chain;
		while (*slot != 0) {
			km_HandlerEntry *h = RightOffsetedPointer(self, self->size, *slot);
			slot = &h->next;
		}
		*slot = self->size - self->right;
		return 1;
	} else {
		return -1;
	}
//
// 	// @cleanup binary search
// 	for (s32 i=0;i<table->num_actions;++i) {
// 		km_String *name_str = km_KeyMap_string(self, actions[i].name);
// 		char *name_raw = km_String_value(name_str);
// 		if (pt_compare_memory(name, name + name_length, name_raw, name_raw + name_str->length) == 0) {
// 			// found the right action
// 			km_Action *action = actions + i;
//
// 			// reserve right
// 			self->right -= sizeof(km_Handler);
// 			km_HandlerEntry *handler_entry = OffsetedPointer(self, self->right);
// 			*handler_entry = (km_HandlerEntry) {
// 				.context = context,
// 				.handler = callback,
// 				.next = 0,
// 				.padding = 0
// 			};
//
// 			km_RightOffset *slot = &action->handler_chain;
// 			while (*slot != 0) {
// 				km_HandlerEntry *h = RightOffsetedPointer(self, self->size, *slot);
// 				slot = &h->next;
// 			}
// 			*slot = self->size - self->right;
//
// 			break;
// 		}
// 	}
//
// 	// find action in the current active table
// 	return -1;
}

//
// push new key into active table
//
static s32
km_KeyMap_push_key(km_KeyMap *self, void *name, u32 name_length)
{
	if (self->active_table == 0) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	// keys go on the left after the active table while in the push
	// key stage
	u32 right_demand = RAlign(sizeof(km_String) + name_length + 1,8);
	u32 left_demand  = sizeof(km_Key);
	if (self->left + left_demand + right_demand > self->right) {
		return 0;
	}

	// write the name of the table on the right side
	km_String *str = OffsetedPointer(self, self->right - right_demand);
	self->right -= right_demand;
	str->length = name_length;
	char *buffer = OffsetedPointer(str, sizeof(km_String));
	platform.memory_copy(buffer, name, name_length);
	buffer[name_length] = 0;

	//
	km_Table      *table = OffsetedPointer(self, self->active_table);
	++table->num_keys;
	km_Key *entry = OffsetedPointer(self, self->left);
	self->left += sizeof(km_Key);
	*entry= (km_Key) {
		.name = self->size - self->right, // where the key string is located
		.padding = 0,
		.actions_offsets = 0,
		.num_actions = 0
	};
	return 1;
}

//
// push new action into the active table's active key
//
static s32
km_KeyMap_push_action(km_KeyMap *self, void *name, u32 name_length)
{
	if (self->active_table == 0) {
		fputs("No table is active\n", stderr);
		exit(-1);
	}

	// @tode search for action with the same name

	// keys go on the left after the active table while in the push
	// key stage
	u32 right_demand = RAlign(sizeof(km_String) + name_length + 1,8);
	u32 left_demand  = sizeof(km_Action);
	if (self->left + left_demand + right_demand > self->right) {
		return 0;
	}

	// write the name of the action on the right side
	km_String *str = OffsetedPointer(self, self->right - right_demand);
	self->right -= right_demand;
	str->length = name_length;
	char *buffer = OffsetedPointer(str, sizeof(km_String));
	platform.memory_copy(buffer, name, name_length);
	buffer[name_length] = 0;

	//
	km_Table      *table = OffsetedPointer(self, self->active_table);
	++table->num_actions;
	km_Action *action = OffsetedPointer(self, self->left);
	self->left += sizeof(km_Action);
	*action = (km_Action) {
		.name = self->size - self->right, // where the key string is located
		.handler_chain = 0 // list of
	};
	return 1;
}

//
// push new action into the active table's active key
//
static s32
km_KeyMap_tmp_push_action_offset_to_left_(km_KeyMap *self, km_RightOffset offset)
{
	Assert(self->status == km_STATUS_THIRD_PASS);
	if (self->left + offset > self->right) {
		return 0;
	}
	km_RightOffset *ptr = OffsetedPointer(self,self->left);
	*ptr = offset;
	self->left += sizeof(km_RightOffset);
	return 1;
}

static s32
km_KeyMap_consolidate_key_actions_using_tmp_offsets_(km_KeyMap *self, km_RightOffset key_offset)
{
	Assert(self->status == km_STATUS_THIRD_PASS);

	// number of offsets is the current left
	km_Table *tables = km_KeyValue_tables(self);
	u32 pop_left = (char*) (tables + self->num_tables) - (char*) self;
	Assert((self->left - pop_left) % sizeof(km_RightOffset) == 0);
	u32 n = (self->left - pop_left)/sizeof(km_RightOffset);
	km_RightOffset *offsets = OffsetedPointer(self, pop_left);
	self->left = pop_left;

	// check if it will fit
	u32 size = RAlign(n * sizeof(km_RightOffset),8);

	if (self->left + size > self->right) {
		return 0;
	}

	// push action offsets to the right
	void *src = offsets;
	u32   len = n * sizeof(km_RightOffset);
	void *dst = OffsetedPointer(self, self->right - len);
	platform.memory_move(dst, src, len);
	self->right -= len;

	//
	km_Key *key = RightOffsetedPointer(self, self->size, key_offset);
	Assert(key->actions_offsets == 0);
	Assert(key->num_actions == 0);
	key->num_actions = n;
	key->actions_offsets = self->size - self->right;

	return 1;
}



#define km_FMT "%.*s"
#define km_STR(name) (s32) name->length, km_String_value(name)

static void
km_KeyMap_print_to_stderr(km_KeyMap *self)
{
	fprintf(stderr,"km_KeyMap with %d tables\n", self->num_tables);
	km_Table *table_array = OffsetedPointer(self, sizeof(km_KeyMap));
	for (s32 i=0;i<self->num_tables;++i) {
		km_Table  *table = table_array + i;
		km_String *name = km_KeyMap_string(self, table->name);
		fprintf(stderr,"....[%02d] table "km_FMT" has %d keys and %d actions\n", i, km_STR(name), table->num_keys, table->num_actions);
		if (table->num_keys> 0) {
			km_Key *keys = km_KeyMap_keys(self, table);
			for (s32 j=0;j<table->num_keys;++j) {
				km_Key *key = keys + j;
				km_String *key_name = km_KeyMap_string(self, key->name);
				fprintf(stderr,"........[%02d] key "km_FMT"\n", j, km_STR(key_name));
				km_RightOffset *actions_offsets = RightOffsetedPointer(self, self->size, key->actions_offsets);
				for (s32 k=0;k<key->num_actions;++k) {
					km_Action *action = RightOffsetedPointer(self, self->size, actions_offsets[k]);
					km_String *action_name = km_KeyMap_string(self, action->name);
					fprintf(stderr,"............[%02d] key's action "km_FMT"\n", j, km_STR(action_name));
				}
			}
		}
		if (table->num_actions > 0) {
			km_Action *actions = km_KeyMap_actions(self, table);
			for (s32 j=0;j<table->num_actions;++j) {
				km_Action *action = actions + j;
				km_String *action_name = km_KeyMap_string(self, action->name);
				fprintf(stderr,"........[%02d] action "km_FMT"\n", j, km_STR(action_name));
			}
		}
	}
}

static s32
km_is_valid_identifier(char *st, u32 len)
{
	if (len == 0) {
		return 0;
	}
	char start_char = st[0];
	if (!char_is_letter(start_char) && start_char != '_') {
		return 0;
	}
	for (s32 i=1;i<len;++i) {
		if (!char_is_letter(start_char) && !char_is_digit(st[i]) && st[i] != '_') {
			return 0;
		}
	}
	return 1;
}

typedef struct {
	km_KeyMap       *keymap;
	km_Table        *table;
	km_Key          *key;
	km_RightOffset  *actions_offsets;
	s32             actions_offsets_index;
	km_Action       *action;
	km_HandlerEntry *entry;
} km_HandlerEnumeration;

static km_HandlerEnumeration
km_KeyMap_get_handler_enumeration(km_KeyMap *self, char *key_name_cstr)
{
	if (self->active_table == 0) {
		fputs("No active table\n",stderr);
		exit(-1);
	}

	km_HandlerEnumeration result = { 0 };
	result.keymap = self;
	result.table = km_KeyMap_active_table(self);
	// @cleanup binary search for key
	km_RightOffset key_offset = km_KeyMap_key_by_name(self, key_name_cstr, cstr_length(key_name_cstr));
	if (key_offset == 0) {
		return result;
	}
	result.key = RightOffsetedPointer(self, self->size, key_offset);
	if (result.key->num_actions == 0) {
		return result;
	}
	result.actions_offsets = RightOffsetedPointer(self, self->size, result.key->actions_offsets);
	result.action = RightOffsetedPointer(self, self->size, result.actions_offsets[0]);

	// everything is else is zero by the result = { 0 } initialization above

	return result;
}

static km_HandlerEntry*
km_HandlerEnumeration_next(km_HandlerEnumeration *self)
{
	for (;;) {
		if (!self->key || !self->action) return 0;
		if (self->entry == 0) {
			// go to first entry of current action
			if (self->action->handler_chain) {
				self->entry = RightOffsetedPointer(self->keymap, self->keymap->size, self->action->handler_chain);
				return self->entry;
			} else {
				// go to next action
				++self->actions_offsets_index;
				if (self->actions_offsets_index < self->key->num_actions) {
					self->action = RightOffsetedPointer(self->keymap, self->keymap->size, self->actions_offsets[self->actions_offsets_index]);
					continue;
				} else {
					self->action = 0;
					return 0;
				}
			}
		} else {
			// go to next entry
			if (self->entry->next) {
				self->entry = RightOffsetedPointer(self->keymap, self->keymap->size, self->entry->next);
				return self->entry;
			} else {
				// go to next action
				++self->actions_offsets_index;
				if (self->actions_offsets_index < self->key->num_actions) {
					self->action = RightOffsetedPointer(self->keymap, self->keymap->size, self->actions_offsets[self->actions_offsets_index]);
					continue;
				} else {
					self->action = 0;
					return 0;
				}
			}
		}
	}
}

//
// depends on base/stream.c
// depends on base/print.c
//
static km_KeyMap*
km_parse(void *buffer, u64 length, Print *error_log)
{
	km_KeyMap *km = km_new_keymap_raw(0);

	if (!km_KeyMap_store_source(km, buffer, length)) {
		// resize into something that fits
		fputs("Resizing not implemented yet\n",stderr);
		exit(-1);
	}

	static char *TABLE_SPECIFIER = "TABLE";
	// store source code as a string

	// read one line at a time
	strm_Stream stream;
	strm_Stream tokens;
	strm_Record record;
	strm_Record token;
	u64 line_no;

	//
	// @cleanup strm next should not mess with the content
	// in the buffer (see it as read only?)
	// this will make multiple passes on the same base content
	// (balance this idea with the idea of making cstr for every
	// token, which allow us to print without copying...
	// I wish there was a print format for strings with length
	//
	// https://stackoverflow.com/questions/3767284/using-printf-with-a-non-null-terminated-string
	//
	// phase 1 create tables and insert keys
	//
	// fputs((char*) buffer, stderr);
	//
	//

	//
	// First pass: insert keys into tables
	//
	strm_Stream_simple(&stream, '\n', 0, buffer, length);
	line_no = 0;
	s32 table_offset = -1;
	s32 key_offset = -1;
	for (;;) {
		++line_no;
		record = strm_Stream_next(&stream);
		if (strm_EOF == record.status) {
			break;
		} else if (strm_OVERFLOW == record.status) {
			fputs("Line overflow\n",stderr);
			exit(-1);
		}

		strm_Stream_simple(&tokens, ' ', 1, record.begin, record.length);

		// get first token

		token = strm_Stream_next(&tokens);
		if (strm_OVERFLOW == token.status) {
			fputs("Token overlfow\n",stderr);
			exit(-1);
		} else if (strm_OK == token.status) {
			if (token.length == 0 || token.begin[0] == '#') {
				continue;
			} else if (cstr_match_str(TABLE_SPECIFIER,token.begin,token.length)) {
				// create a table
				++table_offset;
				token = strm_Stream_next(&tokens);
				if (strm_OVERFLOW == token.status) {
					fputs("Token overlfow\n",stderr);
					exit(-1);
				} else if (strm_EOF == token.status) {
					// syntax error
					if (error_log) {
						print_format(error_log, "Missing table name on line %d\n", (s32)line_no);
					}
					return 0;
				} else {
					if (!km_is_valid_identifier(token.begin, token.length)) {
						if (error_log) {
							print_format(error_log, "Invalid table name on line %d\n", (s32)line_no);
						}
						return 0;
					} else {
						km_KeyMap_push_table(km, token.begin, (u32) token.length);
					}
				}
			} else {
				km_KeyMap_push_key(km, token.begin, (u32) token.length);
			}
		}
	}
	km_KeyMap_finish_first_pass_(km);

	//
	// Second pass: insert actions into tables
	//
	// second pass add actions to the keys already stored
	// action names go on the right and actions go on the left
	// until we get to a new table
	//
	strm_Stream_simple(&stream, '\n', 0, buffer, length);
	line_no = 0;
	for (;;) {

		++line_no;
		record = strm_Stream_next(&stream);
		if (strm_EOF == record.status) {
			break;
		} else if (strm_OVERFLOW == record.status) {
			fputs("Line overflow\n",stderr);
			exit(-1);
		}

		strm_Stream_simple(&tokens, ' ', 1, record.begin, record.length);

		// get first token

		token = strm_Stream_next(&tokens);
		switch(token.status) {
		case strm_OVERFLOW: {
			fputs("Token overlfow\n",stderr);
			exit(-1);
		} break;
		case strm_OK: {
			if (token.length == 0 || token.begin[0] == '#') {
				break;
			} else if (cstr_match_str(TABLE_SPECIFIER, token.begin, token.length)) {
				fputs("go to next table\n", stderr);
				km_KeyMap_goto_next_table_second_pass_(km);
				key_offset = 0;
			} else {
				// km_KeyMap_push_key(km, token.begin, (u16) token.length);
				// all the next tokens are actions to be stored if they
				// don't yet exist in the current table
				s32 done = 0;
				while (!done) {
					token = strm_Stream_next(&tokens);
					switch(token.status) {
					case strm_OVERFLOW: {
						fputs("Token overlfow\n",stderr);
						exit(-1);
					} break;
					case strm_OK: {
						if (token.begin[0] == '#') {
							done = 1;
						} else {
							if (!km_is_valid_identifier(token.begin, token.length)) {
								if (error_log) {
									print_format(error_log, "Invalid action name on line %d\n", (s32)line_no);
								}
								return 0;
							} else {
								s32 ok = km_KeyMap_push_action(km, token.begin, token.length);
							}
						}
					} break;
					case strm_EOF: { done = 1; } break;
					}
				}
			}
		}
		default: break;
		}
	}
	km_KeyMap_finish_second_pass_(km);


	//
	// Third pass: associate actions to keys in all tables
	//
	strm_Stream_simple(&stream, '\n', 0, buffer, length);
	line_no = 0;
	for (;;) {

		++line_no;
		record = strm_Stream_next(&stream);
		if (strm_EOF == record.status) {
			break;
		} else if (strm_OVERFLOW == record.status) {
			fputs("Line overflow\n",stderr);
			exit(-1);
		}

		strm_Stream_simple(&tokens, ' ', 1, record.begin, record.length);

		token = strm_Stream_next(&tokens);
		switch(token.status) {
		case strm_OVERFLOW: {
			fputs("Token overlfow\n",stderr);
			exit(-1);
		} break;
		case strm_OK: {
			if (token.length == 0 || token.begin[0] == '#') {
				break;
			} else if (cstr_match_str(TABLE_SPECIFIER, token.begin, token.length)) {
				fputs("go to next table\n", stderr);
				km_KeyMap_goto_next_table_third_pass_(km);
				key_offset = 0;
			} else {

				km_RightOffset key= km_KeyMap_key_by_name(km, token.begin, token.length);
				Assert(key && "Unexpected error. Key should have been found");

				// km_KeyMap_push_key(km, token.begin, (u16) token.length);
				// all the next tokens are actions to be stored if they
				// don't yet exist in the current table
				s32 done = 0;
				while (!done) {
					token = strm_Stream_next(&tokens);
					switch(token.status) {
					case strm_OVERFLOW: {
						fputs("Token overlfow\n",stderr);
						exit(-1);
					} break;
					case strm_OK: {
						if (token.begin[0] == '#') {
							done = 1;
						} else {
							if (!km_is_valid_identifier(token.begin, token.length)) {
								if (error_log) {
									print_format(error_log, "Invalid action name on line %d\n", (s32)line_no);
								}
								return 0;
							} else {
								km_RightOffset action = km_KeyMap_action_by_name(km, token.begin, token.length);
								if (action == 0) {
									fputs("Unexpected error. Action should have been found\n",stderr);
									exit(-1);
								}
								// s32 ok = km_KeyMap_push_action(km, token.begin, token.length);
						                // add action to key in a local buffer
								if (!km_KeyMap_tmp_push_action_offset_to_left_(km, action)) {
									fputs("Resizing not implemented yet\n",stderr);
									exit(-1);
								}
							}
						}
					} break;
					case strm_EOF: { done = 1; } break;
					}
				}
				if (!km_KeyMap_consolidate_key_actions_using_tmp_offsets_(km, key)) {
					fputs("Resizing not implemented yet\n",stderr);
					exit(-1);
				}
			}
		}
		default: break;
		}
	} // end third stage
	km_KeyMap_finish_third_pass_(km);


	return km;
}

