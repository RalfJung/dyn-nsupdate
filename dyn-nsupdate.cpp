#include <iostream>
#include <fstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace boost::property_tree;

int main(int, const char **)
{
	ptree config;
	ini_parser::read_ini(CONFIG_FILE, config);
	
	std::cout << "Hi world! " << CONFIG_FILE << std::endl;
	return 0;
}
