#include "hash_table.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stdbool.h>
#define INITIAL_CAP 29

//HASHTABLE IMPLEMENTATION WITH QUADRATIC PROBING COLLISION RESOLUTION

struct HashTable* newHash() {
	//initialize hashtable, dynamically allocate memory for array of
	//key value pairs using a prime number for quadratic probing collision
	//resolution
	struct HashTable *hashtable = malloc(sizeof(struct HashTable));
	hashtable -> pairList = malloc(INITIAL_CAP * sizeof(struct KVPair*));
	hashtable -> length = 0;
	hashtable -> capacity = INITIAL_CAP;
	allocatePairs(hashtable -> pairList, INITIAL_CAP);

	return hashtable;
}

uint32_t hornerHash(struct HashTable *hashtable, char* key) {
	//hash function
	int len = strlen(key);
	int n = (8 > len) ? len : 8;
	int i = 0;
	uint32_t x = 0;
	while (i < n) {
		int ord = key[i];
		int exp = n - i - 1;
		x += ord * (pow(31, exp));
		i++;
	}

	return x % (hashtable -> capacity);
}

void add(struct HashTable* hashtable, char* key, long int value) {
	//load factor for resizing; used for quadratic probing collision resolution
	double loadFactor =  ((double)(hashtable -> length) / (hashtable -> capacity));

	//load factor check
	if (loadFactor > 0.5) {
		resize(hashtable);
		loadFactor = ((double)(hashtable -> length) / (hashtable -> capacity));
	}

	//hash key for index into key-value array
	uint32_t index = hornerHash(hashtable, key);

	//if spot empty, add
	//else if key is the same, update value
	//else resolve collision
	if (((hashtable -> pairList)[index] -> key == NULL) && ((hashtable -> pairList)[index] -> value == 0)) {
		(hashtable -> pairList)[index] -> key = key;
		(hashtable -> pairList)[index] -> value = value;
	} else if (strcmp((hashtable -> pairList)[index] -> key, key) == 0) {
		(hashtable -> pairList)[index] -> value = value;
		(hashtable -> length)--;
	} else {
		collisionResolution(hashtable, key, value);
	}
	
	//increase length of array
	(hashtable -> length)++;
}

long int get(struct HashTable* hashtable, char* key) {
	uint32_t index = hornerHash(hashtable, key);

	//if first index's key matches, return value
	//else check indices given by quadratic probing
	if (((hashtable -> pairList)[index] -> key != NULL) && (strcmp((hashtable -> pairList)[index] -> key, key) == 0)) {
		return (hashtable -> pairList)[index] -> value; 
	} else {
		int i = 1;
		//100 is arbitrary number chosen
		while (i < 100) {
			int newIndex = index + (i * i);

			if (newIndex > ((hashtable -> capacity) - 1)) {
				newIndex = newIndex % (hashtable -> capacity);
			}

			if (((hashtable -> pairList)[newIndex] -> key != NULL) && (strcmp((hashtable -> pairList)[newIndex] -> key, key) == 0)) {
				return (hashtable -> pairList)[newIndex] -> value;
			}

			i++;
		}

		//will return -1 if key not in hashtable
		return -1;
	}
}

void collisionResolution(struct HashTable* hashtable, char* key, long int value) {
	//quadratic probing collision resolution
	//
	//boolean for while loop condition
	bool spotFound = false;
	uint32_t index = hornerHash(hashtable, key);
	int i = 1;
	while (spotFound != true) {
		int newIndex = index + (i * i);

		if (newIndex > ((hashtable -> capacity)) - 1) {
			newIndex = newIndex % (hashtable -> capacity);
		}

		if ((hashtable -> pairList)[newIndex] -> key != NULL) {
			if (strcmp((hashtable -> pairList)[newIndex] -> key, key) == 0) {
				(hashtable -> pairList)[newIndex] -> value = value;
				(hashtable -> length)--;
				spotFound = true;
			}
		} else if (((hashtable -> pairList)[newIndex] -> key == NULL) && ((hashtable -> pairList)[newIndex] -> value == 0)) {
			(hashtable -> pairList)[newIndex] -> key = key;
			(hashtable -> pairList)[newIndex] -> value = value;
			spotFound = true;
		}

		i++;
	}
}

void resize(struct HashTable* hashtable) {
	//old capacity
	int oldCap = hashtable -> capacity;

	//make temp copy of original hashmap to be able to rehash in new table
	//allocate memory for temporary array of key-value pairs
	struct KVPair **temp = malloc(oldCap * sizeof(struct KVPair*));
	allocatePairs(temp, oldCap);
	for (int j = 0; j < (oldCap); j++) {
		temp[j] -> key = ((hashtable -> pairList)[j]) -> key;
		temp[j] -> value = ((hashtable -> pairList)[j]) -> value;
	}

	//create new capacity that is prime number for quadratic probing
	int newCap = primeNumberGenerator(oldCap);

	//allocate memory for hashmap's new array of key-value pairs
	struct KVPair **newList = malloc(newCap * sizeof(struct KVPair*));
	allocatePairs(newList, newCap);

	//free old array
	freePairs(hashtable -> pairList, oldCap);
	free(hashtable -> pairList);

	//reset pointers to new list, new capacity, and reset size
	hashtable -> pairList = newList;
	hashtable -> capacity = newCap;
	hashtable -> length = 0;

	//add pairs into new table
	for (int k = 0; k < oldCap; k++) {
		if (temp[k] -> key != NULL) {
			add(hashtable, temp[k] -> key, temp[k] -> value);
		}
	}

	//free temporary duplicate array
	freePairs(temp, oldCap);
	free(temp);
}

int primeNumberGenerator(int oldSize) {
	//method to create a prime number generator from a given interger
	//used for quadratic probing
	
	int newSize = 2 * oldSize;
	bool prime = false;

	while (prime == false) {
		int innerLoopCheck = 0;
		if ((newSize % 2 == 0) || (newSize % 3 == 0)) {
			prime = false;
			newSize++;
			continue;
		}
		
		for (int i = 5; (i * i) <= newSize; i = i + 6) {
			if (newSize % i == 0 || newSize % (i + 2) == 0) {
				innerLoopCheck = 1;
			}
		}

		if (innerLoopCheck == 1) {
			prime = false;
			newSize++;
			continue;
		} else {
			prime = true;
		}

	}

	return newSize;
}

void allocatePairs(struct KVPair **pairList, int cap) {
	//memory allocation method for key-value array
	
	for (int i = 0; i < cap; i++) {
		pairList[i] = malloc(sizeof(struct KVPair));
		(pairList[i]) -> key = NULL;
		(pairList[i]) -> value = 0;
	}
}

void freePairs(struct KVPair **pairList, int cap) {
	//memory free method for key-value array
	
	for (int i = 0; i < cap; i++) {
		free(pairList[i]);
	}
}
