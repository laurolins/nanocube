/* this file should be included before nanocube_index.c */

/******************************************************************************/
/* hijack nx_PAYLOAD for custom payload */
#define nx_PAYLOAD
typedef struct {
	/* offset pointer to a record contained all the accumulated */
	al_Ptr_char data;
} nx_PayloadAggregate;

static void
nx_PayloadAggregate_init(nx_PayloadAggregate *self, void* payload_context);

static void
nx_PayloadAggregate_share(nx_PayloadAggregate* self, nx_PayloadAggregate* other, void *payload_context);

static void
nx_PayloadAggregate_insert(nx_PayloadAggregate* self, void *payload_unit, void *payload_context);
/******************************************************************************/
