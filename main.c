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
#define HTTP_PORT_PATH		"http_port"
#define TEMP_HTTP_PORT		5025
#define TEMP_SERVER_PORT	5026

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


int main(int argc, char **argv)
{
	struct hostent *host = NULL;
	fd_set readfds = { 0 };
	int result = 0;

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		strerror(errno);
		return errno;
	}

	struct sockaddr_in http_server;
	http_server.sin_family = AF_INET;
	http_server.sin_port = htons(TEMP_SERVER_PORT);
	// http_server.sin_addr.s_addr = inet_addr(dns_ip);

	// struct sockaddr_in http_client;
	// dns_server.sin_family = AF_INET;
	// dns_server.sin_port = htons(IN_PORT);
	// dns_server.sin_addr.s_addr = inet_addr(dns_ip);
	// so_reuseaddr
	
	return 0;
}