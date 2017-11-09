#if 0

/* #define nx_PAYLOAD */

/* typedef u32 nx_PayloadUnit; */

/* typedef struct { */
/*   /\* offset pointer to a record contained all the accumulated *\/ */
/*   al_Ptr_char data; */
/* } nx_PayloadAggregate; */

/* declare the 3 services that are expected with respect to payloads */

typedef struct{
	u32 key;
	u32 value;
} nd_Pair;

typedef struct {
	nd_Pair first;
	u32     size;
} nd_QDigest;

typedef struct {
	u32 dataMax;
	u32 compressionFactor;
} nd_QDigest_Parameters;

internal void
nd_QDigest_compress(nd_QDigest *self, nd_QDigest_Paramters *params)
{
}

internal void
nd_QDigest_insert(nd_QDigest *self, nd_QDigest_Paramters *params, u32 value)
{
	u32 newValue  = *((u32*) payload_unit);
	nd_QDigest_Parameters *parameters = (nd_QDigest_Parameters*) payload_context;


	//insert new value
	u32 key = parameters->dataMax + newValue;
	//search for node
	nd_QDigest *qDigest = (nd_QDigest*)al_Ptr_char_get(&self->data);
	nd_Pair* pairs = &(qDigest->first);
	bool done = false;
	for(int i = 0 ; i < qDigest->size ; ++i){
		nd_Pair* currentPair = (pairs+i);
		if(currentPair->key == key){
			currentPair->value += 1;
			done = true;
			break;
		}
	}

	if(!done){
		pairs[qDigest->size] = (nd_Pair){.key=key,.value=1};
	}

	//
	nd_QDigest_compress();
}

internal void
nx_PayloadAggregate_init(nx_PayloadAggregate *self, void* payload_context){

}

internal void
nx_PayloadAggregate_share(nx_PayloadAggregate* self, nx_PayloadAggregate* other, void *payload_context)
{
	nv_Nanocube *nanocube = (nv_Nanocube*) payload_context;

	char *it_src = al_Ptr_char_get(&other->data);
	Assert(!al_Ptr_char_is_null(&self->data));
	char *it_dst = al_Ptr_char_get(&self->data); 

	al_Ptr_char_set(&self->data, it_dst);
	pt_copy_bytes(it_src, it_src + nanocube->payload_aggregate_size,
		      it_dst, it_dst + nanocube->payload_aggregate_size);
}



//
u32
nd_getSibiling(u32 id)
{
	if(id > 1){
		if(id % 2 == 0)
			return id+1;
		else
			return id-1;
	}
	else{
		return 0;
	}

}

u32
nd_getParent(u32 id)
{
	if(id > 1){
		return Math.floor(id/2);
	}
	else{
		return 0;
	}
}

u32
nd_getLeftChild(u32 id)
{
	if(id >= this.dataMax)
		return 0;
	else
		return 2*id;
}

u32 getRightChild(u32 id){
	if(id >= this.dataMax)
		return 0;
	else
		return 2*id+1;
}

nd_Pair getNodeRange(u32 id, u32 dataMax){
	u32 ancestors[100];
	u32 currentNode = id;
	u32 nextAvailable = 0;
	do{
		ancestors[nextAvailable++] = currentNode;
		currentNode = getParent(currentNode);
	}while(currentNode);

	//console.log("====>",ancestors);
	var range = (nd_Pair){.key=1,.value=dataMax};
	for(var i = nextAvailable-1 ; i > 0; --i){
		u32 currentIndex = ancestors[i];
		u32 childIndex   = ancestors[i-1];
		u32 newLength    = ((range[1]-range[0])/2);
		if(childIndex == getLeftChild(currentIndex)){
			range.value = range.key + newLength;
		}
		else{
			range.key = range.value - newLength;
		}
	}
	return range;
}

internal void
nx_PayloadAggregate_insert(nx_PayloadAggregate* self, void *payload_unit, void *payload_context){
	u32 newValue  = *((u32*) payload_unit);
	nd_QDigest_Parameters *parameters = (nd_QDigest_Parameters*) payload_context;


	//insert new value
	u32 key = parameters->dataMax + newValue;
	//search for node
	nd_QDigest *qDigest = (nd_QDigest*)al_Ptr_char_get(&self->data);
	nd_Pair* pairs = &(qDigest->first);
	bool done = false;
	for(int i = 0 ; i < qDigest->size ; ++i){
		nd_Pair* currentPair = (pairs+i);
		if(currentPair->key == key){
			currentPair->value += 1;
			done = true;
			break;
		}
	}

	if(!done){
		pairs[qDigest->size] = (nd_Pair){.key=key,.value=1};
	}

	//
	compress();
}

#endif

#ifdef nanocube_qdigest_UNIT_TEST

// TODO(llins): vector payload is more general, can be used for
// every payload that is indirect
#include "nanocube_vector_payload.h"

#include <stdio.h>
#include <stdlib.h>

int main()
{

	printf("Hello World\n");

}

#endif


