/* Copyright (c) 2014, Ralf Jung <post@ralfj.de>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of the FreeBSD Project.
 */

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
	unsigned server_port = config.get<unsigned>("port", 53);
	
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
		execl(nsupdate.c_str(), nsupdate.c_str(), "-p", std::to_string(server_port).c_str(), "-l", (char *)NULL);
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
