import socket

UDP_IP_ADDRESS = "0.0.0.0"
UDP_PORT_NO = 1234

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP_ADDRESS, UDP_PORT_NO))

while True:
    data, addr = server_socket.recvfrom(1024)

    print("Received Data: ", data)