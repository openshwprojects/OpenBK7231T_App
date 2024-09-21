#include "ringbuff32.h"
#include <stdlib.h>
#include <string.h>


// very simple circular buffer / ring buffer implementation
// - elements can be stored, but not removed
// - you can iterate over all elements, writing them to a buffer, concatenated by a given char
// type of values is defined in ringbuff.h
// e.g. #define RBTYPE	uint16_t

typedef struct RB32_s {
	RBTYPE *var;	// array of vars
	int next;
	int maxval;
	bool overflow;	
} RB32_t;


RB32_t *RBinit(int maxval){
	RB32_t *rb = (RB32_t*)malloc(sizeof(RB32_t));
	if (! rb) {
			return NULL;
	}
	rb->overflow=0; 
	rb->next = 0;
	rb->maxval = maxval;
	rb->var = (RBTYPE*)malloc(sizeof(RBTYPE)*maxval);
	if (! rb->var) {
			free(rb);
			return NULL;
	}
	memset(rb->var, 255, maxval);
	return rb;
}

void RBfree(RB32_t *rb){
	if (rb) {
		free(rb->var);
		free(rb);
	}
}

int RB_saveVal(RB32_t *rb, RBTYPE val){
	rb->var[rb->next++] = val;
	if (rb->next > rb->maxval-1) { 
		rb->overflow=1; 
		rb->next = 0;
	}
	return rb->next;
}

void iterateRBtoBuff(RB32_t *rb, void(*callback)(RBTYPE val, void *buff, char* concatstr), void *buff, char* concatstr){
	if (!rb) {
		return 0;
	}
	int c=rb->next;			// usually (RB is "fully used") we start at "next": this is the oldest value, which will be overwritten with the next value
	int values = rb->maxval; 	// we will have all values defined ..
	if (rb->overflow==0) { 
		c = 0;				// if we only started, the values start from var[0]  ...
		values = rb->next;		// ... and we have "next" values (from var[0] to var[next - 1] )
	 }
	for (int i=0; i < values;i++) {
		callback(rb->var[c],buff, ( i == values -1) ? "" : concatstr);
		c = (c + 1) % rb->maxval;	
	}
	
	
}

