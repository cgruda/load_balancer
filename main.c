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

#include <time.h>

#define ERROR			-1
#define dbg			printf
#define SERVER_PORT_PATH	"server_port"
#define CLIENT_PORT_PATH	"http_port"
#define TEMP_CLIENT_PORT	5025

#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define RAND_MAX	64000 - RAND_MIN
#define RAND_MIN	1024

#define MAX_SERVERS			3
#define SOCKET_BACKLOG			3
#define CLIENT_SOCKET_MAX_BACKLOG	1

int write_port_to_file(uint16_t port, char *path)
{
	FILE *fp = fopen(path, "w");
	if (!fp) {
		return ERROR;
	}
	fprintf(fp, "%d", port);
	fclose(fp);
	return 0;
}

int load_balancer_port_init(char *path, int *sockfd)
{
	ushort rand_port;

	*sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (*sockfd < 0) {
		return ERROR;
	}

	do {
		rand_port = rand() + RAND_MIN;

		struct sockaddr_in addr = {0};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(rand_port);

		if (!bind(*sockfd, (struct sockaddr *)&addr, sizeof(addr))) {
			break;
		}

	} while (1);

	if (write_port_to_file(rand_port, path) < 0) {
		return ERROR;
	}

	if (listen(*sockfd, SOCKET_BACKLOG) < 0) {
		return ERROR;
	}

	return 0;
}

int main()
{
	srand(time(0));

	// int status;
	int server_sockfd;

	load_balancer_port_init(SERVER_PORT_PATH, &server_sockfd);

	// int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	// if (client_sockfd < 0) {
	// 	strerror(errno);
	// 	return errno;
	// }

	// struct sockaddr_in client_addr;
	// client_addr.sin_family = AF_INET;
	// client_addr.sin_port = htons(TEMP_CLIENT_PORT);
	// client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// status = bind(client_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));
	// if (status < 0) {
	// 	strerror(errno);
	// 	return errno;
	// }

	// status = listen(client_sockfd, CLIENT_SOCKET_MAX_BACKLOG);
	// if (status < 0) {
	// 	strerror(errno);
	// 	return errno;
	// }

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