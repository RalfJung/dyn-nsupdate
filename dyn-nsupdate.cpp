#include <iostream>
#include <fstream>
#include <sys/wait.h>

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

using namespace boost;

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
		exit(1);
	}
	if (!regex_match(user, regex_user)) {
		std::cerr << "Invalid username " << user << "." << std::endl;
		exit(1);
	}
	
	/* read configuration */
	property_tree::ptree config;
	property_tree::ini_parser::read_ini(CONFIG_FILE, config);
	std::string nsupdate = config.get<std::string>("nsupdate");
	
	/* Check username, password, domain */
	optional<std::string> correct_password = config.get_optional<std::string>(user+".password");
	if (!correct_password || *correct_password != password) {
		std::cerr << "Username or password incorrect." << std::endl;
		exit(1);
	}
	if (config.get<std::string>(user+".domain") != domain) {
		std::cerr << "Domain incorrect." << std::endl;
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
		/* Close write end, use read and as stdin */
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
