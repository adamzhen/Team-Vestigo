import socket
import ast
import matplotlib.pyplot as plt
import numpy as np
import time
from datetime import datetime

UDP_IP_ADDRESS = "0.0.0.0"
UDP_PORT_NO = 1221

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP_ADDRESS, UDP_PORT_NO))

while True:
    data, addr = server_socket.recvfrom(1024)

    #print("Received Data: ", data)
    angles = ast.literal_eval(data.decode('utf-8'))
    now = datetime.now()
    current_time = now.strftime("%m-%d-%Y %H:%M:%S")

    print(current_time, angles)
