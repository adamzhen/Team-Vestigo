# Generate a csv file with random data for testing the visualization tool
# Data must include the following columns: 
# Timestamp (s) from 0-1000
# X-coordinate (m) from 0-6
# Y-coordinate (m) from 0-6
# Z-coordinate (m) from 0-10
# Orientation (degrees) from 0-360

import csv
import random
import matplotlib.pyplot as plt

csvloc = 'Visualization Tool/QtDev/simdata.csv'
heatmaploc = 'Visualization Tool/QtDev/heatmap.png'

# Generate random data
def generate_data():
    # Create a csv file
    with open(csvloc, 'w', newline='') as csvfile:
        # Create a writer object
        writer = csv.writer(csvfile)

        # Write the column names
        writer.writerow(['Timestamp (s)', 'X-coordinate (m)', 'Y-coordinate (m)', 'Z-coordinate (m)', 'Orientation (degrees)'])

        # Write the data
        for i in range(1001):
            row = [i, random.uniform(0, 6), random.uniform(0, 6), random.uniform(0, 10), random.uniform(0, 360)]
            writer.writerow(row)
            #print(row)

# Call the function
generate_data()
print(f"Data generated and csv saved to {csvloc}")

# Plot heat map of x and y coordinates
# Read the csv file
with open(csvloc, 'r') as csvfile:
    # Create a reader object
    reader = csv.reader(csvfile)
    # Skip the first row
    next(reader)
    # Create lists to store the data
    x = []
    y = []
    # Store the data in the lists
    for row in reader:
        x.append(float(row[1]))
        y.append(float(row[2]))
# Plot the data as a heat map
plt.hist2d(x, y, bins=60, cmap='hot')
plt.colorbar()
plt.xlabel('X (m)')
plt.ylabel('Y (m)')
plt.savefig(heatmaploc)
plt.close()
print(f"Heat map saved to {heatmaploc}")