#include "http.h"
#include "connect.h"
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define HTTP_MSG_END		"\r\n\r\n"
#define HTTP_MSG_END_LEN	4
#define BUFF_ALLOC_CHUNK	256

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
	int buff_len = 0, peek_len = 0, http_msg_len = -1, ret_val = -1;
	do {
		if (peek_len == buff_len) {
			*p_buff = realloc(*p_buff, buff_len + BUFF_ALLOC_CHUNK + 1);
			if (!*p_buff) {
				break;
			}
			buff_len += BUFF_ALLOC_CHUNK;
			memset(*p_buff, 0, buff_len + 1);
		}
		peek_len = recv(sockfd, *p_buff, buff_len, MSG_PEEK);
		if (peek_len < 0) {
			break;
		}
	} while (!is_http_msg_in_buff(*p_buff, &http_msg_len));

	if (http_msg_len > 0) {
		memset(*p_buff, 0, buff_len);
		if (recv(sockfd, *p_buff, http_msg_len, 0) == http_msg_len) {
			ret_val = 0;
		}
	}

	if (ret_val < 0) {
		free(*p_buff);
	}
	return ret_val;
}

int http_msg_send(int sockfd, char *http_msg_buff)
{
	int ret_val = 0;
	int tot_sent_len = 0;
	int msg_len = strlen(http_msg_buff);

	do {
		int sent_len = send(sockfd, http_msg_buff + tot_sent_len, msg_len - tot_sent_len, 0);
		if (sent_len < 0) {
			ret_val = -1;
			break;
		}

		tot_sent_len += sent_len;

	} while (tot_sent_len < msg_len);

	return ret_val;
}

int http_tunnel_msg(int srcfd, int destfd)
{
	char *buff = NULL;
	int ret_val = 0;

	do {
		if (http_msg_recv(srcfd, &buff) < 0) {
			ret_val = -1;
			break;
		}

		if (http_msg_send(destfd, buff) < 0) {
			ret_val = -1;
			break;
		}
	} while (0);

	free(buff);
	return ret_val;
}

int http_session(int server_connfd, int client_sockfd)
{
	int ret_val = -1;

	int *client_connfd = accept_connections(client_sockfd, 1);
	if (!client_connfd) {
		return -1;
	}

	do {
		if (http_tunnel_msg(*client_connfd, server_connfd) < 0) {
			break;
		}

		if (http_tunnel_msg(server_connfd, *client_connfd) < 0) {
			break;
		}

		if (http_tunnel_msg(server_connfd, *client_connfd) < 0) {
			break;
		}

		ret_val = 0;

	} while (0);

	gracefull_disconnect(client_connfd, 1);
	free(client_connfd);

	return ret_val;
}