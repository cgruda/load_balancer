#!/usr/bin/python2.7

import socket

HOST = '127.0.0.1' # FIXME: this must be checked
PORT_PATH = "server_port"

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
	data = sockfd.recv(1024)
	print('Received', repr(data))
	sockfd.close()
	print("done!")