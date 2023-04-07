using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;

namespace ConsoleApp1
{
    class Program
    {
        public static int var1;
        public static int var2;
        public static int var3;
        public static int var4;

        static void Main(string[] args)
        {
            // create a UDP socket and bind it to the specified port
            UdpClient udpClient = new UdpClient(1234);

            while (true)
            {
                // receive data from the socket
                IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 0);
                byte[] data = udpClient.Receive(ref remoteEP);
                string str_data = Encoding.UTF8.GetString(data);

                // parse the JSON data and extract the variables
                string[] str_array = str_data.Split(',');
                int[] array = Array.ConvertAll(str_array, s => int.Parse(s));
                var1 = array[0];
                var2 = array[1];
                var3 = array[2];
                var4 = array[3];

                // print the extracted variables
                Console.WriteLine("var1 = " + var1);
                Console.WriteLine("var2 = " + var2);
                Console.WriteLine("var3 = " + var3);
                Console.WriteLine("var4 = " + var4);

                // Calculate trilateration
                double Ax = 0, Ay = 0;
                double Bx = 3, By = 0;
                double Cx = 0, Cy = 4;
                double Dx = 3, Dy = 4;

                // Read the distances from a JSON string input
                string input = @"{
                    ""a"": " + var1 + @",
                    ""b"": " + var2 + @",
                    ""c"": " + var3 + @",
                    ""d"": " + var4 + @"
                }";

                JsonDocument jsonDoc = JsonDocument.Parse(input);
                JsonElement root = jsonDoc.RootElement;
                double a = root.GetProperty("a").GetDouble();
                double b = root.GetProperty("b").GetDouble();
                double c = root.GetProperty("c").GetDouble();
                double d = root.GetProperty("d").GetDouble();

                // Calculate the coordinates of two possible points using trilateration
                double m1 = (By - Ay) / (Bx - Ax);  // Calculate slope of line AB
                double n1 = (a * a - b * b + Ax * Ax - Bx * Bx + Ay * Ay - By * By) / (2 * (Bx - Ax));  // Calculate y-intercept of line AB that intersects circle A

                double a1 = 1 + m1 * m1;  // Calculate coefficient a for quadratic formula
                double b1 = -2 * Ax + 2 * m1 * (n1 - Ay);  // Calculate coefficient b for quadratic formula
                double c1 = Ax * Ax + (n1 - Ay) * (n1 - Ay) - a * a;  // Calculate coefficient c for quadratic formula
            
                double Px1 = (-b1 + Math.Sqrt(b1*b1 - 4*a1*c1)) / (2*a1);  // Calculate x-coordinate of possible point
                double Py1 = m1*Px1 + n1;  // Calculate y-coordinate of possible point

                double Px2 = (-b1 - Math.Sqrt(b1*b1 - 4*a1*c1)) / (2*a1);  // Calculate second possible x-coordinate
                double Py2 = m1*Px2 + n1;  // Calculate second possible y-coordinate
                
                // Determine which of the two possible points is correct
                double distance1 = Math.Sqrt((Cx - Px1)*(Cx - Px1) + (Cy - Py1)*(Cy - Py1));  // Calculate distance from point to C using first possible coordinates
                double distance2 = Math.Sqrt((Cx - Px2)*(Cx - Px2) + (Cy - Py2)*(Cy - Py2));  // Calculate distance from point to C using second possible coordinates
                
                double Px = 0, Py = 0;  // Initialize final coordinates of point
                
                if (Math.Abs(distance1 - c) < 0.01)  // Check if first possible coordinates are correct
                {
                    Px = Px1;
                    Py = Py1;
                }
                else  // Second possible coordinates are correct
                {
                    Px = Px2;
                    Py = Py2;
                }
                
                // Confirm the location of the point using the coordinates of the fourth location and its distance
                double distance = Math.Sqrt((Dx - Px)*(Dx - Px) + (Dy - Py)*(Dy - Py));  // Calculate distance from point to D
                
                if (Math.Abs(distance - d) > 0.01)  //
                {
                    Console.WriteLine("Error: the distances do not correspond to a single location.");
                }
                else
                {
                    Console.WriteLine("The coordinates of the point are: ({0}, {1})", Px, Py);
                }
        }
    }
}
}