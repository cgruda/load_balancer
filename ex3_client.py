#!/usr/bin/python2.7

import socket

TEST_STR = 'Hello world! this is a test\r\n\r\n'
PORT_PATH = "http_port"
HOST = '127.0.0.1' # FIXME: this must be checked

def read_port(path):
	with open(path, 'r') as fp:
		port = int(fp.readline())
	return port

def connect_to_load_balancer(port):
	sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sockfd.connect((HOST, port))
	return sockfd

if __name__ == "__main__":
	port = read_port(PORT_PATH)
	sockfd = connect_to_load_balancer(port)
	sockfd.sendall(TEST_STR)
	print("done!")
	sockfd.close()