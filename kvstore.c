#include "hash_table.h" //hash table header
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

void signalHandler(int signo);

void getRequest(struct HashTable* ht, FILE* db, char* dbname, char* key, pid_t PID);

//Global variables for signal handling
char* server_fifo = NULL;
int server_fifo_fd = 0;
int numKeys = 0;
char** keyArray = NULL;
struct HashTable* datatable = NULL;

int main (int argc, char* argv[]) {
	//Signal Handler
	signal(SIGQUIT, signalHandler);

	//arg count check
	if (argc != 3) {
		printf("Please enter a valid number of arguments\n");
		return 1;
	}

	//initialize buffers for request from client/s, for kv pair from database
	char request[BUFSIZ];
	char kv[BUFSIZ];
	memset(request, 0, sizeof(request));
	memset(kv, 0, sizeof(kv));

	//initialize intermediary data structure for fast lookup
	//as well as vars for reading and parsing database file
	datatable = newHash();
	FILE* db;
	char* database_name = argv[1];
	db = fopen(database_name, "r");
	keyArray = malloc( 1 * sizeof(char*)); //array to hold allocated char*-due to my implementation of hashtable.
	long int offset = 0; //byte offset to support O(1) look up time in file
	while (fgets(kv, BUFSIZ, db)) { //parse database file, store keys in array
		char* keybuf = malloc(32 * sizeof(char));
		sscanf(kv, "%[^,],", keybuf);
		keyArray[numKeys] = keybuf;
		add(datatable, keybuf, offset); //populate hashtable
		numKeys++;
		keyArray = realloc(keyArray, (numKeys + 1) * sizeof(char*));
		offset = ftell(db); //update offset to store into hashtable
	}
	fclose(db);

	//piping
	server_fifo = argv[2];
	mkfifo(server_fifo, 0666);
	server_fifo_fd = open(server_fifo, O_RDWR);
	int n;

	while ((n = read(server_fifo_fd, request, BUFSIZ)) != EOF && n > 0) {
		char command[3] = {0};
		sscanf(request, "%s", command);
		
		if (strcmp(command, "set") == 0) { //set protocol
			//parse server request
			char* key = malloc(32 * sizeof(char));
			char value[1000] = {0};
			sscanf(request, "%s %s %[^\n]", command, key, value);

			//update hash table, database
			db = fopen(database_name, "a+");
			fseek(db, 0, SEEK_END);
			keyArray[numKeys] = key;
			add(datatable, key, ftell(db));
			numKeys++;
			keyArray = realloc(keyArray, (numKeys + 1) * sizeof(char*));

			fprintf(db, "%s,%s\n", key, value);
			fclose(db);
		} else if (strcmp(command, "get") == 0) { //get protocol
			//parse server request
			pid_t clientPID = 0;
			char key[32] = {0};
			sscanf(request, "%s %d %s", command, &clientPID, key);

			getRequest(datatable, db, argv[1], key, clientPID);
		}

		memset(request, 0, BUFSIZ);	
		memset(command, 0, 3);

	}

	return 0;
}

void getRequest(struct HashTable* ht, FILE* db, char* dbname, char* key, pid_t PID) {
	//client specific pipe, same protocol as client
	char clientFIFO[100] = "client";
	char stringPID[100] = {0};
	sprintf(stringPID, "%d", PID);
	strcat(clientFIFO, stringPID);
	
	//correct line for key value pair using offset stored
	long int correctLine = get(ht, key);

	mkfifo(clientFIFO, 0666);
       	int client_fd = open(clientFIFO, O_RDWR);

	//Key not in database
	if (correctLine == -1) {
		write(client_fd, "ERROR", strlen("ERROR"));
		close(client_fd);
		return;
	}

	//O(1) look up in database file using byte offset
	char line[BUFSIZ] = {0};
	db = fopen(dbname, "r");
	fseek(db, correctLine, 0);
	fgets(line, BUFSIZ, db);
	char curkey[32] = {0};
	char value[1000] = {0};
	sscanf(line, "%[^,],%[^\n]", curkey, value);
	write(client_fd, value, strlen(value));

	fclose(db);
	close(client_fd);
}


void signalHandler(int signo) {
	//free all dynamically allocated vars
	for (int k = 0; k < numKeys; k++) {
			free(keyArray[k]);
	}
	free(keyArray);
	freePairs(datatable -> pairList, datatable -> capacity);
	free(datatable -> pairList);
	free(datatable);
	close(server_fifo_fd);
	unlink(server_fifo);
}
