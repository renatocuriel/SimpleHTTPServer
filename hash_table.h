#include <stdint.h>

#ifndef HT
#define HT
struct KVPair {
	char *key;
	long int value;
};

struct HashTable {
	struct KVPair **pairList;
	int length;
	int capacity;
};

struct HashTable* newHash();

void add();

long int get();

void collisionResolution();

void resize();

int primeNumberGenerator();

void allocatePairs();

void freePairs();

uint32_t hornerHash();
#endif
