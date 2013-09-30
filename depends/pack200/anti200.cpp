/*
 * This is trivial. Do what thou wilt with it. Public domain.
 */

#include <stdexcept>
#include <iostream>
#include "unpack200.h"

int main(int argc, char **argv)
{
	if (argc == 3)
	{
		try
		{
			unpack_200(argv[1], argv[2]);
		}
		catch (std::runtime_error &e)
		{
			std::cerr << "Bad things happened: " << e.what() << std::endl;
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	else
		std::cerr << "Simple pack200 unpacker!" << std::endl << "Run like this:" << std::endl
				  << "  " << argv[0] << " input.jar.lzma output.jar" << std::endl;
	return EXIT_FAILURE;
}
