/**
 * Owen Dunn, Reuben Wattenhofer, Cody Krueger
 * Project 4: Mini HTTP Server 
 * CIS 457 Data Communications
 * 3 DEC 2018
 */

//
// compile: g++ httpServer.cpp -o s
//

#include <sys/socket.h> // How to send/receive information over networks
#include <netinet/in.h> //includes information specific to internet protocol
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For close()
#include <arpa/inet.h>
#include <sys/select.h>
#include <getopt.h>  // parsing command options

//c++ stuff
#include <iostream> 
#include <map>
#include <string>
#include <sstream> 
#include <istream>

#define DEFAULT_SERVER_PORT 8080

using namespace std;


int main(int argc, char** argv) {
    // How to launch server:
    // ./s -p<port number> -docroot<directory to search for files> 
    //     -logfile<file for log messages>
    int port;          // any less than 1024 requires root access
    string directory,  // dir the server looks for requested files 
           logfile;    // file for log messages to be written

    // Parse the command line arguments (reads "-" options)
    int opt, i;
    const char* short_opt = "p:d:l:";
    struct option long_opt[] =
    {
       {"port", required_argument, NULL, 'p'},
       {"docroot", required_argument, NULL, 'd'},
       {"logfile", required_argument, NULL, 'l'},
       {NULL,      0,                 NULL, 0  }
    };

    while((opt = getopt_long_only(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch (opt) {
            case -1:  // no more arguments
            case 0:   // long options toggles
                break;
            case 'p':
                printf("p: you entered \"%s\"\n", optarg);
                break;
            case 'd':  // docroot
                // call getopt for all docroot chars here before option?
                // i = opind;
                // option.append(1, opt);
                // while ((opt = getopt(arc, argv, ""))) {

                // }
                // break;
                printf("d: you entered \"%s\"\n", optarg);
                break;
            case 'l':  // logfile
                printf("l: you entered \"%s\"\n", optarg);
                break;
            case '?':
                printf("?: you entered \"%s\"\n", optarg);
                break;
            default: /* '?' */
                fprintf(stderr, 
                    "Usage: %s [-p portnumber] [-docroot directory] [-logfile file]\n",
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Network communication done using stream (tcp) sockets
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serveraddr, clientaddr;  // todo clientaddr code needed?
    serveraddr.sin_family = AF_INET;
    // need permission to use 80, which we don't have; we'll just always use 8080
    serveraddr.sin_port = htons(port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr*) &serveraddr, sizeof(serveraddr));
    listen(sockfd, 10);

    // select() related code (if using)
    fd_set sockets;
    FD_ZERO(&sockets);
    FD_SET(sockfd, &sockets);

    // Accept client requests from a browser. Send a response fitting a
    // given request.
    bool quit = false;
    while(!quit) {




        quit = true;
    }

    return 0;
}
