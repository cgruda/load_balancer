#!/usr/bin/python2.7

import socket

PATH = "server_port"
HOST = '127.0.0.1' # FIXME: this must be checked

HTTP_NOT_FOUND = ("HTTP/1.1 404 Not Found\n"
		  "Content-type: text/html\n"
		  "Content-length: 113\r\n\r\n"
		  "<html><head><title>Not Found</title></head><body>\n"
		  "Sorry, the object you requested was not found.\n"
		  "</body></html>\r\n\r\n")

def read_port_from_file(path):
	with open(path, 'r') as fp:
		port = int(fp.readline())
	return port

def connect_to_load_balancer(port):
	sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sockfd.connect((HOST, port))
	return sockfd

def get_addr_from_http_msg(msg):
	addr = msg.split(' ')[1]
	return addr

def main():
	port = read_port_from_file(PATH)
	sockfd = connect_to_load_balancer(port)
	http_msg = sockfd.recv(1024)
	print('Received: %s' % http_msg) # FIXME:
	addr = get_addr_from_http_msg(http_msg)
	if (addr != '/counter'):
		print("Sending not found..")
		sockfd.sendall(HTTP_NOT_FOUND)
	sockfd.close()

if __name__ == "__main__":
	main()