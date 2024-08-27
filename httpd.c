#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>

#define DEFAULT_BACKLOG 100
#define MAXLEN 1000

void signalHandler(int signo);

//Global vars for signal handling
int sock_fd;
int newsock_fd;
int server_fifo_fd;
char* fifo_name;

int main(int argc, char* argv[]) {
	//Signal handler
	signal(SIGQUIT, signalHandler);

	//Arg count error handling
	if (argc != 3) {
		printf("Usage: ./httpd [fifo name] [port #]\n");
		exit(1);
	}

	//Vars for parsing command line, socket handling, fifo
        fifo_name = argv[1];
	mkfifo(fifo_name, 0666);
	server_fifo_fd = open(fifo_name, O_RDWR);
	if (server_fifo_fd == -1) {
		printf("Error opening well-known fifo\n");
		exit(1);
	}


	int mlen = 0;
	int enable = 1;
	int port = atoi(argv[2]);
	socklen_t len;
	struct sockaddr_in rs;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
	if (sock_fd < 0) {
		printf("Error creating socket\n");
		exit(1);
	}

	rs.sin_family = AF_INET;
	rs.sin_port = htons(port);
	rs.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(sock_fd, (struct sockaddr*) &rs, sizeof(rs));

	int l = listen(sock_fd, DEFAULT_BACKLOG);
	if (l == -1) {
		printf("Error listening\n");
		exit(1);
	}

	char request[BUFSIZ] = {0};
	while (1) {
		struct sockaddr_in clientaddr;
		len = sizeof(clientaddr);
		newsock_fd = accept(sock_fd, (struct sockaddr *) &clientaddr, &len);
		pid_t PID = fork();
		if (PID < 0) { //Error forking
			printf("Error forking from parent process\n");
			exit(1);
		} else if (PID == 0) { //CHILD
			//Handle requests
			int rlen = 0;
			mlen = recv(newsock_fd, request, sizeof(request), 0);
			//if client closes connection
			if (mlen <= 0) {
				printf("Client closed connection");
				exit(1);
			}

			char parsReq1[100] = {0}; //verb
			char parsReq2[100] = {0}; //endpoint
			char parsReq3[100] = {0}; //version
			sscanf(request, "%s %s %s", parsReq1, parsReq2, parsReq3);
			char reply[BUFSIZ] = {0};

			if ((strcmp("GET", parsReq1) != 0 && strcmp("HEAD", parsReq1) != 0 && strcmp("PUT", parsReq1) != 0) && strcmp("HTTP/1.1", parsReq3) != 0) { //bad request
				char* code = "400 Bad Request";
				long int i = 0;
				sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, i, "Error 400 Bad Request\n");
				size_t repLength = strlen(reply);
				rlen = send(newsock_fd, reply, repLength, 0);
			}

			else if (strstr(parsReq2, "/kv/") != NULL) { //kv functionality
				//Get key
				char key[32] = {0};
				sscanf(parsReq2, "/kv/%s", key);
				if (strcmp("GET", parsReq1) == 0) {
					//Creation of client specific pipe for get request
         			        pid_t PID = getpid();
				        char clientFIFO[100] = "client";
                			char stringPID[100] = {0};
                			sprintf(stringPID, "%d", PID);
                			strcat(clientFIFO, stringPID);
                			mkfifo(clientFIFO, 0666);
                			int client_fd = open(clientFIFO, O_RDWR);
					if (client_fd == -1) {
						//Error 500 Internal Error
						char* code = "500 Internal Error";
						char* content = "Error 500 Internal Error";
						size_t contLen = strlen(content);
						sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, content);
					}

                			char value[1000] = {0};

                			//send request to server, read reply
                			sprintf(request, "get %d %s", PID, key);
                			write(server_fifo_fd, request, strlen(request));
                			read(client_fd, value, 1000);

                			if (strcmp(value, "ERROR") == 0) { //key not found
						char* content = "Key does not exist.\n";
						char* code = "404 Not Found";
						size_t contentLength = strlen(content);
						sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contentLength, content);
                			} else {
						//HTTP Reply
						char* code = "202 OK";
						size_t contentLength = strlen(value);
						sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contentLength, value);
                			}

					//send reply
					size_t repLength = strlen(reply);
					rlen = send(newsock_fd, reply, repLength, 0);

                			close(client_fd);
                			unlink(clientFIFO);
				}
				
				else if (strcmp("PUT", parsReq1) == 0) {
					//get content from request
					char value[1000] = {0};
					char* contentDelim = "\r\n\r\n";
					char* contentSplit = strstr(request, contentDelim);
					sscanf(contentSplit, "\r\n\r\n %[^]]", value);
                			sprintf(request, "set %s %s", key, value);
                			write(server_fifo_fd, request, strlen(request));

					//HTTP REPLY
					char* code = "202 OK";
					long int i = 0;
					sprintf(reply, "HTTP/1.0 %s\r\nContent-Length: %ld\r\n\r\n", code, i);

					size_t repLength = strlen(reply);
					rlen = send(newsock_fd, reply, repLength, 0);
				} else { //HEAD, other valid requests
					//Error 501 Not Implemented
					char* code = "501 Not Implemented";
					char* errcontent = "Error 501 Not Implemented";
					size_t contLen = strlen(errcontent);
					sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, errcontent);
					printf("reply: \n%s\n", reply);
					size_t repLength = strlen(reply);
					rlen = send(newsock_fd, reply, repLength, 0);
				}
				
			} else { //file functionality
				if (strcmp("GET", parsReq1) == 0 || strcmp("HEAD", parsReq1) == 0) {
					//get file name
					char fileName[1024] = {0};
					sscanf(parsReq2, "/%s", fileName);

					if (strstr(fileName, "~") != NULL) { //".." removed automatically when request is sent
						//Error 403 Permission Denied
						char* code = "403 Permission Denied";
						char* content = "Error 403 Permission Denied";
						size_t contLen = strlen(content);
						sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, content);
						size_t repLength = strlen(reply);
						rlen = send(newsock_fd, reply, repLength, 0);
					} else {
						char buf[BUFSIZ] = {0};
						char* content = buf;
						FILE* f = fopen(fileName, "rb");
						if (f == NULL) {
							//Error 404 File Not Found
							char* code = "404 File Not Found";
							char* errcontent = "Error 404 File Not Found";
							size_t contLen = strlen(errcontent);
							sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, errcontent);
							size_t repLength = strlen(reply);
							rlen = send(newsock_fd, reply, repLength, 0);
							
						} else {
							//put entire file contents into buffer to send back
							fseek(f, 0, SEEK_END);
							long fsize = ftell(f);
							rewind(f);
							fread(content, fsize, 1, f);
							fclose(f);

							//HTTP Reply
							char* code = "202 OK";
							size_t contLen = strlen(content);
							if (strcmp("GET", parsReq1) == 0) {
								sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, content);
							} else if (strcmp("HEAD", parsReq1) == 0) {
								sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n", code, contLen);
							}
							size_t repLength = strlen(reply);
							rlen = send(newsock_fd, reply, repLength, 0);
						}
					}
				}
				
				else if (strcmp("PUT", parsReq1) == 0) {
					//Error 400 Bad Request
					char* code = "400 Bad Request";
					char* errcontent = "Error 400 Bad Request";
					size_t contLen = strlen(errcontent);
					sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, errcontent);
					size_t repLength = strlen(reply);
					rlen = send(newsock_fd, reply, repLength, 0);
				}
				
				else {
					//Error 501 Not Implemented
					char* code = "501 Not Implemented";
					char* errcontent = "Error 501 Not Implemented";
					size_t contLen = strlen(errcontent);
					sprintf(reply, "HTTP/1.0 %s\r\nContent-Type: text/html\r\nContent-Length: %ld\r\n\r\n%s", code, contLen, errcontent);
					size_t repLength = strlen(reply);
					rlen = send(newsock_fd, reply, repLength, 0);
				}

			}

			exit(0);
		} else { //PARENT
			//wait for child to finish, handle errors if any
			pid_t finishedChild = wait(NULL);

			if (finishedChild == -1) {
				printf("Child process terminated request erroneously\n");
			}

		}

		memset(request, 0, BUFSIZ);
	}

	return 0;
}

void signalHandler(int signo) {
	close(server_fifo_fd);
	unlink(fifo_name);
	shutdown(sock_fd, SHUT_RDWR);	
	shutdown(newsock_fd, SHUT_RDWR);
	exit(0);
}
