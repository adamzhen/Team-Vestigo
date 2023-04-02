% Define the serial port
port = "COM3"; % replace "COM4" with the port your device is connected to
baudrate = 9600;

% Create the serial port object
s = serialport(port, baudrate);

% Configure the serial port
configureTerminator(s, "LF");

% Open a text file for writing
filename = 'euler_data.txt';
fileID = fopen(filename,'w');

% Start timer
tic;

% Read and display the serial data
for c = 1:1000
    data = readline(s);
    % Get elapsed time
    elapsed_time = toc;
    % Write the array to the text file using fprintf
    fprintf(fileID, '%d,%s\n',c,data);
    disp(c)
end

% Close the file
fclose(fileID);

% Close the serial port when done
clear s;
