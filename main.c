
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <stdbool.h>
#include <time.h>

#ifdef RAND_MAX
#undef RAND_MAX
#endif

#define SUCCESS			0
#define FAILURE			-1
#define SERVER_PORT_PATH	"server_port"
#define CLIENT_PORT_PATH	"http_port"
#define RAND_MAX		64000 - RAND_MIN
#define RAND_MIN		1024
#define MAX_SERVERS		1
#define MAX_CLIENTS		1
#define SOCKET_BACKLOG		MAX_SERVERS
#define HTTP_MSG_END		"\r\n\r\n"
#define HTTP_MSG_END_LEN	4
#define BUFF_ALLOC_CHUNK	256

struct load_balancer_env
{
	int server_sockfd;
	int client_sockfd;
	int *server_connfd;
	int client_conn_cnt;
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

void close_connections(int *connfd, int count)
{
	for (int i = 0; i < count; i++) {
		close(connfd[i]);
	}
}

int *accept_connections(int sockfd, int connection_cnt)
{
	int status = SUCCESS;
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
			status = FAILURE;
			break;
		}
		accepted_connections++;
	}

	if (status == FAILURE) {
		close_connections(connfd_arr, accepted_connections);
		free(connfd_arr);
		connfd_arr = NULL;
	}

	return connfd_arr;
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
	int ret_val = FAILURE;

	int *client_connfd = accept_connections(client_sockfd, 1);
	if (!client_connfd) {
		return FAILURE;
	}

	do {
		if (tunnel_msg(*client_connfd, server_connfd) < 0) {
			break;
		}

		if (tunnel_msg(server_connfd, *client_connfd) < 0) {
			break;
		}

		if (tunnel_msg(server_connfd, *client_connfd) < 0) {
			break;
		}

		ret_val = SUCCESS;

	} while (0);

	close_connections(client_connfd, 1);
	free(client_connfd);

	return ret_val;
}

int socket_bind_listen(char *path)
{
	int ret_val = FAILURE;
	int sockfd;
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

int load_balancer_init(struct load_balancer_env *env)
{
	int ret_val = FAILURE;

	srand(time(0));

	do {
		env->server_sockfd = socket_bind_listen(SERVER_PORT_PATH);
		if (env->server_sockfd < 0) {
			break;
		}

		env->client_sockfd = socket_bind_listen(CLIENT_PORT_PATH);
		if (env->client_sockfd < 0) {
			break;
		}

		env->server_connfd = accept_connections(env->server_sockfd, MAX_SERVERS);
		if (!env->server_connfd) {
			break;
		}

		ret_val = SUCCESS;

	} while(0);

	if (ret_val == FAILURE) {
		close(env->server_sockfd);
		close(env->client_sockfd);
	}

	return ret_val;
}

void load_balancer_cleanup(struct load_balancer_env *env)
{
	close_connections(env->server_connfd, MAX_SERVERS);
	free(env->server_connfd);
	close(env->server_sockfd);
	close(env->client_sockfd);
}

int main()
{
	int ret_val = EXIT_SUCCESS;
	struct load_balancer_env env = {0};

	if (load_balancer_init(&env) < 0) {
		return EXIT_FAILURE;
	}

	while (1) {
		int server_idx = env.client_conn_cnt % MAX_SERVERS;
		int connfd = env.server_connfd[server_idx];
		if (http_session(connfd, env.client_sockfd) < 0) {
			ret_val = EXIT_FAILURE;
			break;
		}
		env.client_conn_cnt++;
	}

	load_balancer_cleanup(&env);

	return ret_val;
}