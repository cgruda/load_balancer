#!/usr/bin/python2.7

# import socket

# TEST_STR = 'Hello world! this is a test\r\n\r\n'
# HOST = '127.0.0.1' # FIXME: this must be checked

# def read_port(path):
# 	with open(path, 'r') as fp:
# 		port = int(fp.readline())
# 	return port

# def connect_to_load_balancer(port):
# 	sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# 	sockfd.connect((HOST, port))
# 	return sockfd

# if __name__ == "__main__":
# 	port = read_port(PORT_PATH)
# 	sockfd = connect_to_load_balancer(port)
# 	sockfd.sendall(TEST_STR)
# 	print("done!")
# 	sockfd.close()



import sys
from socket import *
import random
import time

def get_port(path):
	with open(path, 'r') as fp:
		port = int(fp.readline())
	return port

request = \
"""GET /counter HTTP/1.1\r
Host: nova.cs.tau.ac.il\r
Connection: keep-alive\r
Cache-Control: max-age=0\r
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r
User-Agent: Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.57 Safari/537.36\r
Accept-Encoding: gzip,deflate,sdch\r
Accept-Language: en-US,en;q=0.8,he;q=0.6\r\n\r\n"""

s = socket()
port = get_port("http_port")
s.connect(('localhost', port))
first_part_len = random.randint(0, len(request))
s.send(request[:first_part_len])
time.sleep(0.1)
s.send(request[first_part_len:])

response = ''
while 1:
	response += s.recv(1024)
	if response.count('\r\n\r\n') == 2:
		break

lines = response.split('\r\n')
assert len(lines) == 7
assert lines[0] == 'HTTP/1.0 200 OK'
assert lines[1] == 'Content-Type: text/html'
assert lines[2].startswith('Content-Length: ')
content_length = int(lines[2][len("Content-Length: "):])
assert lines[3] == ''

assert len(lines[4]) == content_length
content = lines[4]
assert lines[-2:] == ['', '']

print content
