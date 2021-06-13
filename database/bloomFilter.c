#include "bloomFilter.h"

bloomFilter* Bloom_Create(int size, int hashn){
	bloomFilter *filter = malloc(sizeof(bloomFilter));
	filter->size = size;
	filter->hashn = hashn;
	filter->bytes = calloc(size, 1);
	return filter;
}

void Bloom_Set(bloomFilter* filter, uint8_t* arr){
	int i;
	for(i=0; i<filter->size; i++)
		filter->bytes[i] = arr[i];
}

void Bloom_Insert(bloomFilter* filter, char* key){		//using hash_i()
	int h, k = filter->hashn;
	while(k--){						//h/8 = byte to insert, h%8 = position in byte
		h = hash_i(key, k-1) % (filter->size * 8);	//1 = 00000001 in bin, shift left n bits
		filter->bytes[h / 8] |= 1 << h % 8;		//where n = bloom filter bit# to set
	}
}

bool Bloom_Check(bloomFilter* filter, char* key){
	int h, k = filter->hashn;
	while(k--){
		h = hash_i(key, k-1) % (filter->size * 8);	//take the byte e.g. 01100010, we want to check if set on n=2
		if(!(filter->bytes[h / 8] & (1 << h % 8)))	//byte & 00000010 zeros every bit except from nth if only it is set
			return 0;
	}
	return 1;
}

void Bloom_Destroy(bloomFilter* filter){
	free(filter->bytes);
	free(filter);
}
