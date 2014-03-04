#include <iostream>
#include <fstream>
#include <sys/wait.h>

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

using namespace boost;
typedef property_tree::ptree::path_type path;

static void write(int fd, const char *str)
{
	size_t len = strlen(str);
	ssize_t written = write(fd, str, len);
	if (written < 0 || (size_t)written != len) {
		std::cerr << "Error writing pipe." << std::endl;
		exit(1);
	}
}

int main(int argc, const char ** argv)
{
	static const regex regex_ip("\\d{1,3}.\\d{1,3}.\\d{1,3}.\\d{1,3}");
	static const regex regex_password("[a-zA-Z0-9.:;,_-]+");
	static const regex regex_domain("[a-zA-Z0-9.]+");
	
	
	if (argc != 4) {
		std::cerr << "Usage: " << argv[0] << " <domain> <password> <IP address>" << std::endl;
		return 1;
	}
	
	/* Obtain input */
	std::string domain = argv[1];
	std::string password = argv[2];
	std::string ip = argv[3];
	
	/* Validate input */
	if (!regex_match(ip, regex_ip)) {
		std::cerr << "Invalid IP address " << ip << "." << std::endl;
		exit(1);
	}
	if (!regex_match(domain, regex_domain)) {
		std::cerr << "Invalid domain " << domain << "." << std::endl;
		exit(1);
	}
	if (!regex_match(password, regex_password)) {
		std::cerr << "Invalid password " << password << "." << std::endl;
		exit(1);
	}
	
	/* read configuration */
	property_tree::ptree config;
	property_tree::ini_parser::read_ini(CONFIG_FILE, config);
	std::string nsupdate = config.get<std::string>("nsupdate");
	
	/* Given the domain, check whether the password matches */
	optional<std::string> correct_password = config.get_optional<std::string>(path(domain+"/password", '/'));
	if (!correct_password || *correct_password != password) {
		std::cerr << "Password incorrect." << std::endl;
		exit(1);
	}
	
	/* preapre the pipe */
	int pipe_ends[2];
	if (pipe(pipe_ends) < 0) {
		std::cerr << "Error opening pipe." << std::endl;
		exit(1);
	}

	/* Launch nsupdate */
	pid_t child_pid = fork();
	if (child_pid < 0) {
		std::cerr << "Error while forking." << std::endl;
		exit(1);
	}
	if (child_pid == 0) {
		/* We're in the child */
		/* Close write end, use read end as stdin */
		close(pipe_ends[1]);
		if (dup2(pipe_ends[0], fileno(stdin)) < 0) {
			std::cerr << "There was an error redirecting stdin." << std::endl;
			exit(1);
		}
		/* exec nsupdate */
		execl(nsupdate.c_str(), nsupdate.c_str(), "-l", (char *)NULL);
		/* There was an error */
		std::cerr << "There was an error executing nsupdate." << std::endl;
		exit(1);
	}
	
	/* Send it the command */
	write(pipe_ends[1], "update delete ");
	write(pipe_ends[1], domain.c_str());
	write(pipe_ends[1], ". A\n");
	
	write(pipe_ends[1], "update add ");
	write(pipe_ends[1], domain.c_str());
	write(pipe_ends[1], ". 60 A ");
	write(pipe_ends[1], ip.c_str());
	write(pipe_ends[1], "\n");
	
	write(pipe_ends[1], "send\n");
	
	/* Close both ends */
	close(pipe_ends[0]);
	close(pipe_ends[1]);
	
	/* Wait for child to be gone */
	int child_status;
	waitpid(child_pid, &child_status, 0);
	if (child_status != 0) {
		std::cerr << "There was an error in the child." << std::endl;
		exit(1);
	}
	
	return 0;
}
