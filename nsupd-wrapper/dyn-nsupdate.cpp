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
*/

#include <iostream>
#include <fstream>
#include <sys/wait.h>

#include <boost/regex.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>

namespace pt = boost::property_tree;
namespace po = boost::program_options;
using std::string;
using boost::regex;
using boost::optional;

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
    try {
        static const regex regex_ipv4("\\d{1,3}(\\.\\d{1,3}){3}|");
        static const regex regex_ipv6("[a-fA-F0-9]{1,4}(:[a-fA-F0-9]{1,4}){7}|");
        static const regex regex_password("[a-zA-Z0-9.:;,_-]+");
        static const regex regex_domain("[a-zA-Z0-9.]+");
        
        // Declare the supported options.
        po::options_description desc("Allowed options");
        desc.add_options()
            ("help", "produce help message")
            ("domain", po::value<string>()->required(), "the domain to update")
            ("password", po::value<string>()->required(), "the password for the domain")
            ("ipv4", po::value<string>(), "the new IPv4 address (empty to delete the A record)")
            ("ipv6", po::value<string>(), "the new IPv6 address (empty to delete the AAAA record)")
        ;
        
        // parse arguments
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);    
        if (vm.count("help")) {
            std::cout << "dyn-nsupdate -- a safe setuid wrapper for nsupdate" << std::endl << std::endl;
            std::cout << desc << "\n";
            return 1;
        }
        string domain = vm["domain"].as<string>();
        string password = vm["password"].as<string>();
        bool haveIPv4 = vm.count("ipv4");
        string ipv4 = haveIPv4 ? vm["ipv4"].as<string>() : "";
        bool haveIPv6 = vm.count("ipv6");
        string ipv6 = haveIPv6 ? vm["ipv6"].as<string>() : "";
        
        /* Validate input */
        if (!regex_match(ipv4, regex_ipv4)) {
            throw std::runtime_error("Invalid IPv4 address" + ipv4);
        }
        if (!regex_match(ipv6, regex_ipv6)) {
            throw std::runtime_error("Invalid IPv6 address: " + ipv6);
        }
        if (!regex_match(domain, regex_domain)) {
            throw std::runtime_error("Invalid Domain: " + domain);
        }
        if (!regex_match(password, regex_password)) {
           throw std::runtime_error("Invalid Password: " + password);
        }
        
        /* read configuration */
        pt::ptree config;
        pt::ini_parser::read_ini(CONFIG_FILE, config);
        std::string nsupdate = config.get<std::string>("nsupdate");
        unsigned server_port = config.get<unsigned>("port", 53);
        
        /* Given the domain, check whether the password matches */
        optional<std::string> correct_password = config.get_optional<std::string>(pt::ptree::path_type(domain+"/password", '/'));
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
        if (haveIPv4) {
            write(pipe_ends[1], "update delete ");
            write(pipe_ends[1], domain.c_str());
            write(pipe_ends[1], ". A\n");
            
            if (!ipv4.empty()) {
                write(pipe_ends[1], "update add ");
                write(pipe_ends[1], domain.c_str());
                write(pipe_ends[1], ". 60 A ");
                write(pipe_ends[1], ipv4.c_str());
                write(pipe_ends[1], "\n");
            }
        }
        
        if (haveIPv6) {
            write(pipe_ends[1], "update delete ");
            write(pipe_ends[1], domain.c_str());
            write(pipe_ends[1], ". AAAA\n");
            
            if (!ipv6.empty()) {
                write(pipe_ends[1], "update add ");
                write(pipe_ends[1], domain.c_str());
                write(pipe_ends[1], ". 60 AAAA ");
                write(pipe_ends[1], ipv6.c_str());
                write(pipe_ends[1], "\n");
            }
        }
        
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
    }
    catch(std::exception &e) {
        std::cout << e.what() << "\n";
        return 1;
    } 
    
    return 0;
}
