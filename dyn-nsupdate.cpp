#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include <sys/stat.h>

int main(int argc, const char **argv)
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <configuration file>" << std::endl;
		return 1;
	}
	const char *filename = argv[1];
	
	struct stat file_stat;
	int ret = lstat(filename, &file_stat);
	if (ret != 0) {
		std::cerr << "Unable to stat " << filename << "." << std::endl;
		return 1;
	}
	/* Check if the file is suited */
	if (!S_ISREG(file_stat.st_mode)) {
		std::cerr << filename << " is not a file." << std::endl;
		return 1;
	}
	if (file_stat.st_uid != geteuid()) {
		std::cerr << filename << " must be owned by user executing " << argv[0] << "." << std::endl;
		return 1;
	}
	if (file_stat.st_mode & (S_IWGRP | S_IWOTH)) { /* can be written by group/others */
		std::cerr << filename << " must not be writeable by group or others." << std::endl;
		return 1;
	}
	
	std::cout << "Hi world!" << std::endl;
	return 0;
}
