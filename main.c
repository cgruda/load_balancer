#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>

#define SERVER_PORT_PATH	"server_port"
#define CLIENT_PORT_PATH	"http_port"
#define TEMP_CLIENT_PORT	5025
#define TEMP_SERVER_PORT	5026

#define MAX_SERVERS			3
#define SERVER_SOCKET_MAX_BACKLOG	3
#define CLIENT_SOCKET_MAX_BACKLOG	1

int write_port_to_file(uint16_t port, char *path)
{
	FILE *fp = fopen(path, "w");
	if (!fp) {
		strerror(errno);
		return errno;
	}
	fprintf(fp, "%d", port);
	fclose(fp);
	return 0;
}

int main()
{
	int status;

	int server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sockfd < 0) {
		strerror(errno);
		return errno;
	}

	int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_sockfd < 0) {
		strerror(errno);
		return errno;
	}

	struct sockaddr_in server_addr = {0};
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(TEMP_SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	struct sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(TEMP_CLIENT_PORT);
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	status = bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (status < 0) {
		strerror(errno);
		return errno;
	}

	status = bind(client_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));
	if (status < 0) {
		strerror(errno);
		return errno;
	}

	status = listen(server_sockfd, SERVER_SOCKET_MAX_BACKLOG);
	if (status < 0) {
		strerror(errno);
		return errno;
	}

	status = listen(client_sockfd, CLIENT_SOCKET_MAX_BACKLOG);
	if (status < 0) {
		strerror(errno);
		return errno;
	}

	int connfd[MAX_SERVERS] = {0};
	socklen_t addrlen[MAX_SERVERS] = {0};
	struct sockaddr_in conn_addr[MAX_SERVERS];
	for (int i = 0; i < MAX_SERVERS; i++) {
		printf("waiting for connection ... ");

		connfd[i] = accept(server_sockfd, (struct sockaddr *)&conn_addr[i], &addrlen[i]);
		if (connfd[i] < 0) {
			printf("err\n");
			strerror(errno);
			return errno;
		}

		printf("accepted connection!\n");
	}

	exit(2);

	return 0;
}