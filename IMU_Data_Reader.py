import statistics
from math import *
import numpy as np
from matplotlib import pyplot as plt

def moving_average(data, window_size):
    moving_average_data = []
    for i in range(len(data) - window_size + 1):
        window = data[i:i + window_size]
        moving_average_data.append(sum(window) / window_size)
    return moving_average_data


def smooth_data(data, loops, window_size):
    for i in range(loops):
        data = moving_average(data, window_size)
    return data

loops = 3
window_size = 3

with open("Tracked Location") as f:
    data = f.readlines()

theta = []
pitch = []
roll = []
ax = []
ay = []
az = []
vector = []
raw_ax = []
raw_ay = []
raw_az = []
gpt_ax = []
gpt_ay = []
gpt_az = []
dt = []
total_time = 0
for item in data:
    line_list = item.split(",")

    theta_data = float(line_list[0])
    pitch_data = float(line_list[1])
    roll_data = float(line_list[2])
    ax_data = float(line_list[3])
    ay_data = float(line_list[4])
    az_data = float(line_list[5])
    raw_ax_data = float(line_list[6])
    raw_ay_data = float(line_list[7])
    raw_az_data = float(line_list[8])
    gpt_ax_data = float(line_list[9])
    gpt_ay_data = float(line_list[10])
    gpt_az_data = float(line_list[11])
    dt_data = float(line_list[12])

    total_time += dt_data

    ax.append(ax_data)
    ay.append(ay_data)
    az.append(az_data)
    theta.append(theta_data)
    pitch.append(pitch_data)
    roll.append(roll_data)
    raw_ax.append(raw_ax_data)
    raw_ay.append(raw_ay_data)
    raw_az.append(raw_az_data)
    gpt_ax.append(gpt_ax_data)
    gpt_ay.append(gpt_ay_data)
    gpt_az.append(gpt_az_data)
    dt.append(total_time)

print(f'    AX: {statistics.mean(ax):.5f} +/- {statistics.stdev(ax):.5f}')
print(f'    AY: {statistics.mean(ay):.5f} +/- {statistics.stdev(ay):.5f}')
print(f'    AZ: {statistics.mean(az):.5f} +/- {statistics.stdev(az):.5f}')

ax_fine = smooth_data(ax, loops, window_size)
ay_fine = smooth_data(ay, loops, window_size)
az_fine = smooth_data(az, loops, window_size)

fig, px = plt.subplots(nrows=2, ncols=3)

px[0][0].plot(ax_fine)
px[0][0].set_title("AX")
px[0][1].plot(ay_fine)
px[0][1].set_title("AY")
px[0][2].plot(az_fine)
px[0][2].set_title("AZ")
px[1][0].plot(theta)
px[1][0].set_title("Theta")
px[1][2].plot(pitch)
px[1][2].set_title("Pitch")
px[1][1].plot(roll)
px[1][1].set_title("Roll")

plt.tight_layout()
plt.show()

fig_2, px = plt.subplots(nrows=2, ncols=2)

px[0][0].hist(ax)
px[0][0].set_title("AX")
px[1][0].hist(ay)
px[1][0].set_title("AY")
px[0][1].hist(az)
px[0][1].set_title("AZ")


plt.tight_layout()
plt.show()


x1 = dt
y1 = ay_fine

while len(y1) > len(x1):
    y1.pop(0)

while len(x1) > len(y1):
    x1.pop(0)

fig_3, ax1 = plt.subplots()
ax1.plot(x1, y1, 'b-')
ax1.set_xlabel('X Label')
ax1.set_ylabel('Ay', color='b')

# create the second plot with a new y-axis
x2 = dt
y2 = roll

while len(y2) > len(x2):
    y2.pop(0)

while len(x2) > len(y2):
    x2.pop(0)

ax2 = ax1.twinx()
ax2.plot(x2, y2, 'r-')
ax2.set_ylabel('Roll', color='r')

plt.show()