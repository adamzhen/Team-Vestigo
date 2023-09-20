#include <iostream>
#include <string>
#include <cmath>
#include <SDL.h>

#undef main

int main(int argc, char* argv[])
{
	std::cout << "good morning" << std::endl;
	std::string name;
	std::cin >> name;
	std::cout << "hello " << name << std::endl;

	return 0;
}