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
#include <sys/time.h>
#include <time.h>

//c++ stuff
#include <iostream> 
#include <map>
#include <string>
#include <sstream> 
#include <istream>

#define DEFAULT_SERVER_PORT 8080

using namespace std;

//Function Headers
void createDateHeader(char* datehdr);
char* createStatus(int code);
char* httpHeader (char* fileName, int code, char* pstatus);

int main(int argc, char** argv) {
    cout << "---------------Mini HTTP Server---------------" << endl;
    // How to launch server:
    // ./s -p <port number> -docroot <directory to search for files> 
    //     -logfile <file for log messages>
    // command line: valid argument passing (or combinations of below)
    // ./s -p 8080 -docroot . -logfile logs
    // ./s -p8080 -docroot=. -logfile=logs
    // ./s -p8080 --docroot=. --logfile=logs
    
    // Assign default server values if none given later.
    int port = DEFAULT_SERVER_PORT;  // any less than 1024 requires root access
    string directory = ".",     // dir the server looks for requested files 
           logfile = "stdout";  // file for log messages to be written

    // Parse the command line arguments:
    // https://stackoverflow.com/questions/7489093/getopt-long-proper-way-to-use-it
    // optarg is a char array
    int opt;
    const char* short_opt = "p:d:l:";
    struct option long_opt[] =
    {
        // {const char* name, int has_arg, int* flag, int val} (option struct values in order)
        {"port",    required_argument, NULL, 'p'},  // val is return from getopt()
        {"docroot", required_argument, NULL, 'd'},
        {"logfile", required_argument, NULL, 'l'},
        {NULL,      0,                 NULL, 0}
    };
    while((opt = getopt_long_only(argc, argv, short_opt, long_opt, NULL)) != -1) {
        switch (opt) {
            case -1:  // no more arguments
            case 0:   // long options toggles
                break;
            case 'p':
                port = atoi(optarg);
                printf("port: you entered \"%d\"\n", port);
                if (port < 0 || port > 65535) {
                    printf("Please enter a valid port number.");
                    return 1;
                }
                break;
            case 'd':  // docroot
                directory = optarg;
                // todo error check

                printf("docroot: you entered \"%s\"\n", directory.c_str());
                break;
            case 'l':  // logfile
                logfile = optarg;
                // todo error check

                printf("logfile: you entered \"%s\"\n", logfile.c_str());
                break;
            case '?':
                printf("?: you entered \"%s\"\n", optarg);
                break;
            default:
                fprintf(stderr, 
                    "Usage: %s [-p portnumber] [-docroot directory] [-logfile file]\n",
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Network communication done using stream (tcp) sockets
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // // Set timeout for each socket.
    // struct timeval timeout;
    // timeout.tv_sec = 3;
    // timeout.tv_usec = 0;

    // setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));

    // Bind socket.
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

    char* datehdr = new char[80];
    createDateHeader(datehdr);
    printf("%s\n", datehdr);

    // Accept client requests from a browser. Send a response fitting a
    // given request.
    bool quit = false;
    int i;
    while(!quit) {
        fd_set tmp_set = sockets;
		//checks to see if we can read from the sockets
		int n = select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);  // todo timeout time for last arg
        for (i = 0; i < FD_SETSIZE; i++) {
            if(FD_ISSET(i, &tmp_set)) {
                if (i == sockfd) {  // accepting clients?
                    printf("A client connected\n");
                    int len = sizeof(clientaddr);
                    int clientsocket = 
                        accept(sockfd, (struct sockaddr*)&clientaddr, (socklen_t*) &len);

                    // Set timeout for each socket.
                    struct timeval timeout; 
                    timeout.tv_sec = 20; //TODO 20sec
                    timeout.tv_usec = 0;

                    setsockopt(clientsocket,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
                    FD_SET(clientsocket, &sockets);
                }
                // Existing client, serve them
                else {
                    char* line = new char[5000];  // TODO will mem leaks cause a crash if runs too long?
                    // unsigned char* line = new unsigned char[5000];
                    // i is our socket number
                    unsigned int recv_len = recv(i, line, 5000, 0);
                    // Existing client, serve them
                    // if(recv_len == 0) { //commented out because it was causing issues and seemed unnessesary
                    //     printf("Received zero bytes. Ignoring message.\n");  // prints repeatedly for some reason at times
                    //     continue;
                    // }
                    // printf("got:\n%s\n", line);

                    std::stringstream ss(line);
                    std::string to;

                    int lineNumber = 0;
                    if (line != NULL)
                    {
                        while(std::getline(ss,to,'\n')){
                            cout << to <<endl;

                            lineNumber++;
                            
                            // GET request, process it.
                            if (strncmp(to.c_str(), "GET", 3) == 0) {
                                printf("GET request\n");
                                // char noget[] = "Status-Line = HTTP/1.1 200 OK\r\n\r\n";
                                // char noget[] = "HTTP/1.1 404 Not Found\r\n\r\n";
                                // send(i, noget, strlen(noget), 0);

                                // char temp[] = "index.html";
                                // char temp2[] = "OK";
                                // char* response = httpHeader(temp, 200, temp2);
                                char* response = httpHeader("index.html", 200, "OK");
                                cout << response << endl;
                                send(i, response, strlen(response), 0);


                                string delimiter = " ";
                                // for (int j = 0; j < 2; j++) {
                                    // string token = to.substr(0, to.find(delimiter));
                                    to.erase(0, to.find(delimiter) + delimiter.length());
                                    string token = to.substr(0, to.find(delimiter));
                                    cout << "address: " << token << "\n";

                                    // Don't let the client go outside the base directory!        
                                    if (strncmp(token.c_str(), "/..", 3) == 0) {
                                        cout << "Client tried to escape the base directory\n";
                                        continue;
                                    }
                                    
                                // }
                            }
                            // Not a GET request!!!!
                            else if (lineNumber == 1) {
                                printf("not a GET request\n");
                                // char* m
                                // send(i, ) HTTP/1.1 404 Not Found
                            }

                        }

                    }
                    
                    delete line;
                }
            }
        }
    }

    return 0;
}

//pass a reference to the fields to place date header into
//strftime adds a null terminating character to end 
void createDateHeader(char* datehdr) {
    time_t rawtime;
    struct tm* info;
    char buffer[80];

    time(&rawtime);
    info = gmtime(&rawtime);
    strftime(buffer, 80, "Date: %a, %d %b %Y %X GMT\r\n", info);
    memcpy(datehdr, buffer, strlen(buffer)); 
}

char* createStatus(int code) {
    char* status = new char[100];
    strcpy(status, "HTTP/1.1 ");

    if (code == 200) {
        strcat(status, "200 OK");
    }
    else if (code == 404) {
        strcat(status, "404 Not Found");
    }
    else if (code == 304) {
        strcat(status, "304 Not Modified");
    }
    else if (code == 501) {
        strcat(status, "501 Not Implemented");
    }
    else {
        strcat(status, "400 Bad Request");
    }
    strcat(status, "\r\n");

    return status;
}

/**
 * http header:
 * /file?
 * /status code and text
 * /date
 * /last-modified
 * /content-type
 * /length
 * 
 * 
 **/
char* httpHeader (char* fileName, int code, char* pstatus) {
    char* content = new char[1500];
    
    //produce fields to add to header
    char* statusField = createStatus(code);
    char* dateField = new char[50];
    createDateHeader(dateField);

    strcpy(content, statusField);
    strcat(content, dateField);
    //strcpy(content, dateField);

    delete statusField;
    delete dateField;

    cout << content << endl;

    return content;
}
