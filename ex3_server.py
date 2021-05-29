#!/usr/bin/python2.7

# import socket

# HOST = '127.0.0.1' # FIXME: this must be checked

# HTTP_NOT_FOUND = ("HTTP/1.1 404 Not Found\n"
# 		  "Content-type: text/html\n"
# 		  "Content-length: 113\r\n\r\n"
# 		  "<html><head><title>Not Found</title></head><body>\n"
# 		  "Sorry, the object you requested was not found.\n"
# 		  "</body></html>\r\n\r\n")

# def read_port_from_file(path):
# 	with open(path, 'r') as fp:
# 		port = int(fp.readline())
# 	return port

# def connect_to_load_balancer(port):
# 	sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 	sockfd.connect((HOST, port))
# 	return sockfd

# def get_addr_from_http_msg(msg):
# 	addr = msg.split(' ')[1]
# 	return addr

# def main():
# 	port = read_port_from_file(PATH)
# 	sockfd = connect_to_load_balancer(port)
# 	http_msg = sockfd.recv(1024)
# 	print('Received: %s' % http_msg) # FIXME:
# 	addr = get_addr_from_http_msg(http_msg)
# 	if (addr != '/counter'):
# 		print("Sending not found..")
# 		sockfd.sendall(HTTP_NOT_FOUND)
# 	sockfd.close()

# if __name__ == "__main__":
# 	main()


import sys
from socket import *
import random
import time

msg_error = \
"""HTTP/1.1 404 Not Found\r
Content-type: text/html\r
Content-length: 113\r\n\r
<html><head><title>Not Found</title></head><body>\r
Sorry, the object you requested was not found.\r
</body></html>\r\n\r\n"""

msg_counter = \
"""HTTP/1.0 200 OK\r
Content-Type: text/html\r
Content-Length: {}\r\n\r
{}\r\n\r\n"""

port_path = "server_port"
counter = 0

def get_port(path):
	with open(path, 'r') as fp:
		port = int(fp.readline())
	return port

def recv_req(sock):
	buff = ''
	while (1):
		buff += sock.recv(1024)
		if '\r\n\r\n' in buff:
			break
	return buff

def get_resp(addr):
	global counter
	if (addr == "/counter"):
		msg = msg_counter.format(len(str(counter)), counter)
		counter += 1
	else:
		msg = msg_error
	return msg

def get_addr(req):
	return req.split(' ')[1]

def send_resp(sock, buff):
	sock.sendall(buff)

def serv_session(sock):
	req = recv_req(sock)
	addr = get_addr(req)
	resp = get_resp(addr)
	send_resp(sock, resp)

sock = socket()
port = get_port(port_path)
sock.connect(('localhost', port))
while (1):
	serv_session(sock)