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

#define MAX_SERVERS			1
#define MAX_CLIENTS			1
#define SOCKET_BACKLOG			MAX_SERVERS
#define CLIENT_SOCKET_MAX_BACKLOG	1

#define HTML_MESSAGE_END	"\r\n\r\n"
#define BUFF_LEN 		100

struct load_balancer_env
{
	int server_sockfd;
	int client_sockfd;
	int servers_conn_sockfd[MAX_SERVERS];
	int client_conn_sockfd;
	int client_connection_cnt;
};


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

int sock_await_accept_conn(int sockfd, int conn_cnt, int *connfd)
{
	struct sockaddr_in connaddr = {0};
	socklen_t addrlen = {0};
	
	for (int i = 0; i < conn_cnt; i++) {
		dbg("waiting for connection ... ");
		connfd[i] = accept(sockfd, (struct sockaddr *)&connaddr, &addrlen);
		if (connfd[i] < 0) {
			dbg("%s\n", strerror(errno));
			return ERROR;
		}
		dbg("accepted connection!\n");
	}

	return 0;
}

int main()
{
	struct load_balancer_env env = {0};
	srand(time(0));

	if (load_balancer_port_init(SERVER_PORT_PATH, &env.server_sockfd) < 0) {
		dbg("%s\n", strerror(errno));
		return ERROR;
	}

	if (load_balancer_port_init(CLIENT_PORT_PATH, &env.client_sockfd) < 0) {
		dbg("%s\n", strerror(errno));
		return ERROR;
	}

	if (sock_await_accept_conn(env.server_sockfd, MAX_SERVERS, env.servers_conn_sockfd) < 0) {
		dbg("%s\n", strerror(errno));
		return ERROR;
	}

	while (1) {
		if (sock_await_accept_conn(env.client_sockfd, MAX_CLIENTS, &env.client_conn_sockfd) < 0) {
			dbg("%s\n", strerror(errno));
			return ERROR;
		}

		char buff[BUFF_LEN] = {0};

		while(1)
		{
			int size_recv =  recv(env.client_conn_sockfd , buff , BUFF_LEN , 0);
			if (size_recv < 0) {
				dbg("%s\n", strerror(errno));
				return ERROR;
			}
			
			int size_to_send = size_recv;
			char *p_start_of_end_of_http_msg = strstr(buff, HTML_MESSAGE_END);
			if (p_start_of_end_of_http_msg) {
				char *p_last_http_char = p_start_of_end_of_http_msg + strlen(HTML_MESSAGE_END);
				size_to_send = p_last_http_char - buff;
			}
			
			while(size_to_send) {
				int size_sent = send(env.servers_conn_sockfd[env.client_connection_cnt % MAX_SERVERS], buff, size_to_send, 0);
				if (size_sent < 0) {
					dbg("%s\n", strerror(errno));
					return ERROR;
				} else {
					size_to_send -= size_sent;
				}
			}

			if (p_start_of_end_of_http_msg) {
				break;
			}
		}

		env.client_connection_cnt++;

		break;
	}

	exit(2);

	return 0;
}