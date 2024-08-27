#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

//CLIENT IMPLEMENTATION

int main(int argc, char* argv[]) {
	if (argc < 4 || argc > 5) {
		printf("Usage: ./kvclient <named pipe> <get or set> <key> [<value>]\n");
		return 1;
	}

	//initialization of vars
	char request[2048] = {0};
	char* server_fifo = argv[1];
	char* key = argv[3];
	mkfifo(server_fifo, 0666);
	int server_fifo_fd = open(server_fifo, O_RDWR);

	if (argc == 4) {
		if(strcmp(argv[2], "get") != 0) {
			printf("Usage: ./kvclient <named pipe> <get or set> <key> [<value>]\n");
			return 1;
		}

		//Creation of client specific pipe for get request
		pid_t PID = getpid();
		char clientFIFO[100] = "client";
		char stringPID[100] = {0};
		sprintf(stringPID, "%d", PID);
		strcat(clientFIFO, stringPID);
		mkfifo(clientFIFO, 0666);
		int client_fd = open(clientFIFO, O_RDWR);
		char value[1000] = {0};

		//send request to server, read reply
		sprintf(request, "get %d %s", PID, key);
		write(server_fifo_fd, request, strlen(request));
		read(client_fd, value, 1000);

		//Error checking
		if (strcmp(value, "ERROR") == 0) {
			printf("Key %s does not exist.\n", key);
		} else {
			printf("%s\n", value);
		}

		close(client_fd);
		unlink(clientFIFO);
	} else if (argc == 5){
		if(strcmp(argv[2], "set") != 0) {
			printf("Usage: ./kvclient <named pipe> <get or set> <key> [<value>]\n");
			return 1;
		}

		//send request to server
		char* value = argv[4];
		sprintf(request, "set %s %s", key, value);
		write(server_fifo_fd, request, strlen(request));
	}

	close(server_fifo_fd);
	return 0;
}
