
#include "classfile.h"
#include "annotations.h"
#include <fstream>
#include <iostream>

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		std::ifstream file_in(argv[1]);
		if(file_in.is_open())
		{
			file_in.seekg(0, std::_S_end);
			auto length = file_in.tellg();
			char * data = new char[length];
			file_in.seekg(0);
			file_in.read(data,length);
			java::classfile cf (data, length);
			java::annotation_table atable = cf.visible_class_annotations;
			for(int i = 0; i < atable.size(); i++)
			{
				std::cout << atable[i]->toString() << std::endl;
			}
			return 0;
		}
		else
		{
			std::cerr << "Failed to open file : " << argv[1] << std::endl;
			return 1;
		}
	}
	std::cerr << "No file to open :(" << std::endl;
	return 1;
}