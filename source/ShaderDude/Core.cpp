#include "Core.h"
#include <fstream>
#include <iostream>
#include <exception>

std::string ReadFile(const char* path)
{
	std::string file;
	std::ifstream fileStream(path, std::ios::in);
	if (fileStream.is_open())
	{
		std::string line = "";
		while (getline(fileStream, line))
			file += "\n" + line;
		fileStream.close();
		return file;
	}
	//throw std::exception("we failed to load the file fam");
	std::cout << "Couldn't find " << path << "\n";
	return std::string();
}
