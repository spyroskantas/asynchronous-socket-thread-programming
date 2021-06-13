#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hashFunctions.h"

typedef struct{
	int hashn;
	int size;
	uint8_t *bytes;
}bloomFilter;


bloomFilter* Bloom_Create(int size, int hashn);
void Bloom_Set(bloomFilter* filter, uint8_t* arr);
void Bloom_Insert(bloomFilter* filter, char* key);
bool Bloom_Check(bloomFilter* filter, char* key);
void Bloom_Destroy(bloomFilter* filter);