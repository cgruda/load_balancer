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
#include <stdbool.h>
#include <time.h>

#define SUCCESS			0
#define FAILURE			-1
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

#define HTTP_MSG_END		"\r\n\r\n"
#define HTTP_MSG_END_LEN	4
#define BUFF_LEN 		100

#define BUFF_ALLOC_CHUNK		256

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
		return FAILURE;
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
		return FAILURE;
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
		return FAILURE;
	}

	if (listen(*sockfd, SOCKET_BACKLOG) < 0) {
		return FAILURE;
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
			return FAILURE;
		}
		dbg("accepted connection!\n");
	}

	return 0;
}

bool is_http_msg_in_buff(char *buff, int *http_msg_len)
{
	bool ret_val = false;
	*http_msg_len = -1;

	char *p_empty_line = strstr(buff, HTTP_MSG_END);
	if (p_empty_line) {
		*http_msg_len = p_empty_line - buff + HTTP_MSG_END_LEN;
		ret_val = true;
	}
	
	return ret_val;
}

int http_msg_recv(int sockfd, char **p_buff)
{
	*p_buff = NULL;
	int buff_len = 0, peek_len = 0;
	int recv_len, http_msg_len;
	int ret_val = SUCCESS;
	do {
		if (peek_len == buff_len) {
			*p_buff = realloc(*p_buff, buff_len + BUFF_ALLOC_CHUNK + 1);
			if (!*p_buff) {
				ret_val = FAILURE;
				break;
			}
			buff_len += BUFF_ALLOC_CHUNK;
			memset(*p_buff, 0, buff_len + 1);
		}

		peek_len = recv(sockfd, *p_buff, buff_len, MSG_PEEK);
		if (peek_len < 0) {
			ret_val = FAILURE;
			break;
		}

		if (!is_http_msg_in_buff(*p_buff, &http_msg_len)) {
			continue;
		}

		memset(*p_buff, 0, buff_len);
		recv_len = recv(sockfd, *p_buff, http_msg_len, 0);
		if (recv_len != http_msg_len) {
			ret_val = FAILURE;
		}

		ret_val = recv_len;
		break;

	} while (1);

	if (ret_val == FAILURE) {
		free(*p_buff);
		p_buff = NULL;
	}

	return ret_val;
}

int http_msg_send(int sockfd, char *http_msg_buff)
{
	int ret_val = SUCCESS;
	int tot_sent_len = 0;
	int msg_len = strlen(http_msg_buff);

	do {
		int sent_len = send(sockfd, http_msg_buff + tot_sent_len, msg_len - tot_sent_len, 0);
		if (sent_len < 0) {
			ret_val = FAILURE;
			break;
		}

		tot_sent_len += sent_len;

	} while (tot_sent_len < msg_len);

	return ret_val;
}

int tunnel_msg(int srcfd, int destfd)
{
	char *buff = NULL;
	int ret_val = SUCCESS;

	do {
		if (http_msg_recv(srcfd, &buff) < 0) {
			ret_val = FAILURE;
			break;
		}

		if (http_msg_send(destfd, buff) < 0) {
			ret_val = FAILURE;
			break;
		}
	} while (0);

	free(buff);
	return ret_val;
}

int http_session(int server_connfd, int client_sockfd)
{
		int client_connfd;

		if (sock_await_accept_conn(client_sockfd, MAX_CLIENTS, &client_connfd) < 0) {
			return FAILURE;
		}

		tunnel_msg(client_connfd, server_connfd);
		tunnel_msg(server_connfd, client_connfd);
		tunnel_msg(server_connfd, client_connfd);
		close(client_connfd);

		return 0;
}

int main()
{
	struct load_balancer_env env = {0};
	srand(time(0));

	if (load_balancer_port_init(SERVER_PORT_PATH, &env.server_sockfd) < 0) {
		return FAILURE;
	}

	if (load_balancer_port_init(CLIENT_PORT_PATH, &env.client_sockfd) < 0) {
		return FAILURE;
	}

	if (sock_await_accept_conn(env.server_sockfd, MAX_SERVERS, env.servers_conn_sockfd) < 0) {
		return FAILURE;
	}

	while (1) {
		int server_connfd = env.servers_conn_sockfd[env.client_connection_cnt % MAX_SERVERS];
		http_session(server_connfd, env.client_sockfd);
		env.client_connection_cnt++;
	}

	exit(2);

	return 0;
}