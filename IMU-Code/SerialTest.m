% Define the serial port
port = "COM3";
baudrate = 9600;

% Create the serial port object
s = serialport(port, baudrate);

% Configure the serial port
configureTerminator(s, "LF");

% Open a text file for writing
filename = 'euler_and_acceleration_data.txt';
fileID = fopen(filename,'w');

% Start timer
tic;

% Read and display the serial data
for c = 1:1000
    data = readline(s);
    % Write the array to the text file using fprintf
    fprintf(fileID, '%d,%s\n',c,data);
    disp(c)
end

% Get elapsed time
elapsed_time = toc;
disp(elapsed_time)

% Close the file
fclose(fileID);

% Close the serial port when done
clear s;
