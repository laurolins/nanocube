/* profiling infra-structure */

#ifdef PROFILE

/* forward declade profile table */
struct pf_ProfileTable;

typedef enum {
	pf_BEGIN_EVENT,
	pf_END_EVENT
} pf_EventType;

typedef struct {
	u64  clock;
	char *id;
	u16 thread_id;
	u16 core_index;
	u16 type;
	u16 unused;
} pf_Event;

typedef struct {
	u64 volatile slot_event_index;
	u64 event_capacity;
	pf_Event *events[2];
} pf_Table;

static void
pf_Table_init(pf_Table *self, void *buffer, u64 buffer_size)
{
	s64 n = buffer_size/(sizeof(pf_Event)*2);
	self->event_capacity = n;
	self->events[0] = (pf_Event*) buffer;
	self->events[1] = self->events[0] + n;
}

static u64
pf_Table_current_events_count(pf_Table *self)
{
	u32 event_index = self->slot_event_index & 0xFFFFFFFF;
	return event_index;
}

static u64
pf_Table_current_slot(pf_Table *self)
{
	u32 slot        = self->slot_event_index >> 32;
	return slot;
}

#define pf_EVENT_ID__(A, B, C, D) A "|" #B "|" #C "|" D
#define pf_EVENT_ID_(A,B,C,D) pf_EVENT_ID__(A, B, C, D)
#define pf_EVENT_ID(name) pf_EVENT_ID_(__FILE__, __LINE__, __COUNTER__, name)

/* implement this one */
//
// AtomicAddU64
// GetThreadID
// assuming global_profile_table is available
// __rdtsc()
//

// pf_Table global_profile_table_storage = { .slot_event_index =0 };
extern pf_Table *global_profile_table; // = &global_profile_table_storage;

/* TODO(llins): Implement GetThreadID() */

#define pf_RECORD_EVENT(event_type_init, id_init) \
u64 slot_event_index = pt_atomic_add_u64(&global_profile_table->slot_event_index, 1); \
u32 event_index = slot_event_index & 0xFFFFFFFF; \
u32 slot        = slot_event_index >> 32; \
Assert(event_index < global_profile_table->event_capacity);   \
pf_Event *event = global_profile_table->events[slot] + event_index; \
event->clock = pt_get_cpu_clock(); \
event->type = (u8)event_type_init; \
event->core_index = 0; \
event->thread_id = pt_get_thread_id(); \
event->id = id_init;

#define pf_BEGIN_BLOCK_(id) {pf_RECORD_EVENT(pf_BEGIN_EVENT,id)}
#define pf_END_BLOCK_(id) {pf_RECORD_EVENT(pf_END_EVENT,id)}

#define pf_BEGIN_BLOCK(name) {pf_RECORD_EVENT(pf_BEGIN_EVENT,pf_EVENT_ID(name))}
#define pf_END_BLOCK() {pf_RECORD_EVENT(pf_END_EVENT,pf_EVENT_ID("END_"))}

#define pf_BEGIN_FUNCTION() pf_BEGIN_BLOCK(__FUNCTION__)
#define pf_END_FUNCTION() pf_END_BLOCK()

#else

#define pf_BEGIN_BLOCK(name)
#define pf_END_BLOCK()

#define pf_BEGIN_FUNCTION()
#define pf_END_FUNCTION()

#endif


