#include "connect.h"
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MIN	1024
#define RAND_MAX	64000 - RAND_MIN

#define SOCKET_BACKLOG	3

int write_port_to_file(ushort port, char *path)
{
	FILE *fp = fopen(path, "w");
	if (!fp) {
		return -1;
	}
	fprintf(fp, "%d", port);
	fclose(fp);
	return 0;
}

int *accept_connections(int sockfd, int connection_cnt)
{
	int status = 0;
	int accepted_connections = 0;
	struct sockaddr_in addr = {0};
	socklen_t len = {0};

	int *connfd_arr = calloc(connection_cnt, sizeof(*connfd_arr));
	if (!connfd_arr) {
		return NULL;
	}

	for (int i = 0; i < connection_cnt; i++) {
		connfd_arr[i] = accept(sockfd, (struct sockaddr *)&addr, &len);
		if (connfd_arr[i] < 0) {
			status = -1;
			break;
		}
		accepted_connections++;
	}

	if (status < 0) {
		gracefull_disconnect(connfd_arr, accepted_connections);
		free(connfd_arr);
		connfd_arr = NULL;
	}

	return connfd_arr;
}

int socket_bind_listen(char *path)
{
	int ret_val = -1, sockfd;
	ushort port;
	struct sockaddr_in addr = {0}; 

	do {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			break;
		}

		do {
			port = rand() + RAND_MIN;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = htonl(INADDR_ANY);
			addr.sin_port = htons(port);
		} while (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)));

		if (write_port_to_file(port, path) < 0) {
			break;
		}
		if (listen(sockfd, SOCKET_BACKLOG) < 0) {
			break;
		}
		ret_val = sockfd;
	} while(0);

	return ret_val;
}

void gracefull_disconnect(int *connfd, int count)
{
	char dummy[1];
	for (int i = 0; i < count; i++) {
		while (recv(connfd[i], dummy, 1, 0));
		if (shutdown(connfd[i], SHUT_WR) < 0) {
			return;
		}
		close(connfd[i]);
	}
}
