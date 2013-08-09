#include <iostream>
#include <fstream>

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

using namespace boost;

int main(int argc, const char ** argv)
{
	static const regex regex_ip("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}");
	static const regex regex_user("[a-zA-Z]+");
	
	
	if (argc != 5) {
		std::cerr << "Usage: " << argv[0] << " <username> <password> <domain> <IP address>" << std::endl;
		return 1;
	}
	
	/* Obtain and validate inpit */
	std::string user = argv[1];
	std::string password = argv[2];
	std::string domain = argv[3];
	std::string ip = argv[4];
	
	if (!regex_match(ip, regex_ip)) {
		std::cerr << "Invalid IP address " << ip << "." << std::endl;
		return 1;
	}
	if (!regex_match(user, regex_user)) {
		std::cerr << "Invalid username " << user << "." << std::endl;
		return 1;
	}
	
	/* read configuration */
	property_tree::ptree config;
	property_tree::ini_parser::read_ini(CONFIG_FILE, config);
	std::string keyfile = config.get<std::string>("key");
	
	/* Check username, password, domain */
	optional<std::string> correct_password = config.get_optional<std::string>(user+".password");
	if (!correct_password || *correct_password != password) {
		std::cerr << "Username or password incorrect." << std::endl;
		return 1;
	}
	if (config.get<std::string>(user+".domain") != domain) {
		std::cerr << "Domain incorrect." << std::endl;
		return 1;
	}
	
	std::cout << "It's all right, using key " << keyfile << std::endl;
	
	
	return 0;
}
