import socket

HOST = '127.0.0.1' # FIXME: this must be checked
PORT_PATH = "server_port"

def read_port(path):
	with open(path, 'r') as fp:
		port = int(fp.readline())
	return port

def connect_to_load_balancer(port):
	try:
		sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		sockfd.connect((HOST, port))
		# sockfd.sendall(b'Hello, world')
		# data = sockfd.recv(1024)
	except:
		print("Error: socket() failed")
	finally:
		sockfd.close()

# print('Received', repr(data))

if __name__ == "__main__":
	port = read_port(PORT_PATH)
	print(port) # FIXME: this is for debug
	connect_to_load_balancer(port)
	print("done!")