import socket
import matplotlib.pyplot as plt
import sympy as sp


def trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3):
    x, y, z = sp.symbols('x y z')

    # Define the equations for the three spheres
    eq1 = (x - point_1[0]) ** 2 + (y - point_1[1]) ** 2 + (z - point_1[2]) ** 2 - distance_1 ** 2
    eq2 = (x - point_2[0]) ** 2 + (y - point_2[1]) ** 2 + (z - point_2[2]) ** 2 - distance_2 ** 2
    eq3 = (x - point_3[0]) ** 2 + (y - point_3[1]) ** 2 + (z - point_3[2]) ** 2 - distance_3 ** 2

    # Solve the system of equations for the intersection of the three spheres
    solution = sp.solve((eq1, eq2, eq3), (x, y, z))

    # define lists
    solution_1 = [0, 0, 0]
    solution_2 = [0, 0, 0]

    # average solution in xyz
    solution_1[0], solution_1[1], solution_1[2] = solution[0]
    solution_2[0], solution_2[1], solution_2[2] = solution[1]
    return [(solution_1[0] + solution_2[0]) / 2, (solution_1[1] + solution_2[1]) / 2, (solution_1[2] + solution_2[2]) / 2]


def quadlateration(distance_with_centers):
    point_1 = distance_with_centers[0][:]
    point_2 = distance_with_centers[1][:]
    point_3 = distance_with_centers[2][:]
    point_4 = distance_with_centers[3][:]
    distance_1 = point_1.pop(3)
    distance_2 = point_2.pop(3)
    distance_3 = point_3.pop(3)
    distance_4 = point_4.pop(3)
    point_1_prime = trilateration(distance_1, distance_2, distance_3, point_1, point_2, point_3)
    point_2_prime = trilateration(distance_1, distance_2, distance_4, point_1, point_2, point_4)
    point_3_prime = trilateration(distance_2, distance_3, distance_4, point_2, point_3, point_4)
    point_4_prime = trilateration(distance_1, distance_3, distance_4, point_1, point_3, point_4)
    x_location = (point_1_prime[0] + point_2_prime[0] + point_3_prime[0] + point_4_prime[0]) / 4
    y_location = (point_1_prime[1] + point_2_prime[1] + point_3_prime[1] + point_4_prime[1]) / 4
    z_location = (point_1_prime[2] + point_2_prime[2] + point_3_prime[2] + point_4_prime[2]) / 4
    return x_location, y_location, z_location


# Centers with distances
center_1 = [0, 0, 0, 0]
center_2 = [1.71, 0, 0, 0]
center_3 = [-0.75, 2.74, 0, 0]
center_4 = [0.935, 2.79, 0, 0]

# create an empty figure and axis object
fig, ax = plt.subplots()

# create an empty list to store data
data = []

# set the y-axis limits
ax.set_ylim([0, 8])

UDP_IP_ADDRESS = "0.0.0.0"
UDP_PORT_NO = 1234

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((UDP_IP_ADDRESS, UDP_PORT_NO))

while True:
    data, addr = server_socket.recvfrom(1024)

    # Decode the bytes object into a string
    binary_str = data.decode()
    binary_str_list = list(binary_str)

    # remove unnecessary characters
    data_list = []
    for character in binary_str_list:
        if character == '[' or character == ']':
            continue
        else:
            data_list.append(character)

    # convert modified string into list of float distances
    distance_list_string = ''.join(data_list).split(',')
    distance_list = []
    for item in distance_list_string:
        distance_list.append(float(item))

    # prints out incoming data
    print(f'Distances: {distance_list}')

    center_1[3] = distance_list[0]
    center_2[3] = distance_list[1]
    center_3[3] = distance_list[2]
    center_4[3] = distance_list[3]

    distance_with_centers = [center_1, center_2, center_3, center_4]

    x, y, z = quadlateration(distance_with_centers)

    # update the x-axis limits based on the length of data
    ax.set_xlim([0, 8])

    # clear the axis object before redrawing the graph
    ax.clear()

    # plot the data on the graph
    ax.scatter([0, 1.71, -0.75, 0.935], [0, 0, 2.74, 2.79])
    ax.scatter(x, y)

    # display the graph
    plt.draw()