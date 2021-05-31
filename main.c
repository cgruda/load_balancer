
#include "connect.h"
#include "http.h"
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define SERVER_PORT_PATH	"server_port"
#define CLIENT_PORT_PATH	"http_port"
#define MAX_SERVERS		3

struct load_balancer_env
{
	int server_sockfd;
	int client_sockfd;
	int *server_connfd;
	int session_cnt;
};

int load_balancer_init(struct load_balancer_env *env);
int load_balancer_cleanup(struct load_balancer_env *env);

int main()
{
	int ret_val = 0;
	struct load_balancer_env env = {0};

	if (load_balancer_init(&env) < 0) {
		return -1;
	}

	while (1) {
		int server_idx = env.session_cnt % MAX_SERVERS;
		if (http_session(env.server_connfd[server_idx], env.client_sockfd) < 0) {
			ret_val = -1;
			break;
		}
		env.session_cnt++;
	}

	load_balancer_cleanup(&env);

	return ret_val;
}

int load_balancer_init(struct load_balancer_env *env)
{
	int ret_val = -1;
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
		ret_val = 0;
	} while(0);

	if (ret_val < 0) {
		gracefull_disconnect(&env->server_sockfd, 1);
		gracefull_disconnect(&env->client_sockfd, 1);
	}

	return ret_val;
}

int load_balancer_cleanup(struct load_balancer_env *env)
{
	gracefull_disconnect(env->server_connfd, MAX_SERVERS);
	gracefull_disconnect(&env->server_sockfd, 1);
	gracefull_disconnect(&env->client_sockfd, 1);
	free(env->server_connfd);
	return 0;
}