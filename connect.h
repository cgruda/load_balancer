#ifndef _CONNECT_H_
#define _CONNECT_H_

int socket_bind_listen(char *path);
int *accept_connections(int sockfd, int connection_cnt);
void gracefull_disconnect(int *connfd, int count);

#endif