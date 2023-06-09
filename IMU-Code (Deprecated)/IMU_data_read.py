import socket
import ast
import matplotlib.pyplot as plt
import numpy as np
import time
from datetime import datetime

UDP_IP_ADDRESS = "0.0.0.0"
UDP_PORT_NO = 1234

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) 
server_socket.bind((UDP_IP_ADDRESS, UDP_PORT_NO))

recfile = open("IMU-Code/IMU_data.txt", "w")

while True:
    data, addr = server_socket.recvfrom(1024)

    #print("Received Data: ", data)
    datalist = ast.literal_eval(data.decode('utf-8'))
    now = datetime.now()
    recstr = now.strftime("%m-%d-%Y, %H:%M:%S") + ", " + (', '.join([str(d) for d in datalist]))
    recfile.write(recstr + "\n")

    print(recstr)

recfile.close()