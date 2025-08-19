// definitions in new_common.h
// how many enties in buffer?
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "new_common.h"

#define RBTYPE	uint8_t


typedef struct RB32_s RB32_t;


RB32_t *RBinit(int maxval);
void RBfree(RB32_t *rb);
int RB_saveVal(RB32_t *rb, RBTYPE val);
void iterateRBtoBuff(RB32_t *rb, void(*callback)(RBTYPE val, void *buff, char* concatstr), void *buff, char* concatstr);

