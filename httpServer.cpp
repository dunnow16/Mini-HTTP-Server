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
#include <sys/stat.h>
#include <time.h>

//c++ stuff
#include <iostream> 
#include <map>
#include <string>
#include <sstream> 
#include <istream>

#define DEFAULT_SERVER_PORT 8080

using namespace std;

void createDateHeader(char* datehdr);
void createLastModHeader(char* lasthdr, char* fileName);
void createContentTypeHeader(char* cthdr, char* mt);
bool isFileModified(char* ifModSinceHdr, char* fileName);


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
    bool isLogFile = false;

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
                isLogFile = true;
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
    
    // test date header
    char* datehdr = new char[80];
    createDateHeader(datehdr);
    // printf("%s\n", datehdr);

    // Create last modified header (doesn't change? run again before sending if it does)
    char* lasthdr = new char[80];
    char* temp = (char*)logfile.c_str();
    createLastModHeader(lasthdr, temp); 
    char tmp2[] = "pdf"; 
    char* cthdr = new char[80];
    createContentTypeHeader(cthdr, tmp2);

    // Open file for writing and return pointer to file object.
    // test file / stdout writing
    FILE* pFile;
    if (isLogFile) {
        pFile = fopen(logfile.c_str(), "w");
        fprintf(pFile, lasthdr);
        //fclose(pFile);  // must close for file to update? (tested this)
    }
    //printf(lasthdr);

    char tmp3[] = "If-Modified-Since: Mon, 4 6 2018 22:15:12 GMT";
    char fn[] = "project4 notes";
    isFileModified(tmp3, fn); 

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
                    timeout.tv_sec = 3; //TODO 20sec
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
                    if(recv_len == 0) {
                        printf("Received zero bytes. Ignoring message.\n");  // prints repeatedly for some reason at times
                        continue;
                    }
                    // printf("got:\n%s\n", line);

                    std::stringstream ss(line);
                    std::string to;

                    int lineNumber = 0;
                    if (line != NULL) {
                        while(std::getline(ss,to,'\n')) {
                            cout << to <<endl;

                            lineNumber++;
                            
                            // GET request, process it.
                            if (strncmp(to.c_str(), "GET", 3) == 0) {
                                printf("GET request\n");
                                // char noget[] = "Status-Line = HTTP/1.1 200 OK\r\n\r\n";
                                // char noget[] = "HTTP/1.1 404 Not Found\r\n\r\n";
                                // send(i, noget, strlen(noget), 0);
                                
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

    fclose(pFile);  // must close for file to update? (tested this)
    return 0;
}

/**
 * Return a header of the current time and date.
 * This passes a reference to the fields to place date header into
 * strftime, which adds a null terminating character to end.
 */
void createDateHeader(char* datehdr) {
    time_t rawtime;
    struct tm* info;
    char buffer[80];

    time(&rawtime);
    info = gmtime(&rawtime);
    strftime(buffer, 80, "Date: %a, %d %b %Y %X GMT\r\n", info);
    // add a null terminator?
    
    memcpy(datehdr, buffer, strlen(buffer)); 
}

/**
 * Return a header of the last time a file was modified.
 */
void createLastModHeader(char* lasthdr, char* fileName) {
    time_t rawtime;
    struct tm* info;
    char buffer[80];
    // path code here: (don't need unless use files outside of current directory)
    // string path = "/tmp/symlink/";
    // path.append(fileName);
    // char* link = path.c_str();
    // printf("link path: %s\n", link);
    // char actualpath[200];
    // char* ptr;
    // ptr = realpath(path, actualpath);
    // printf("actual path: %s\n", actualpath);

    struct stat s;
    int status;
    status = stat(fileName, &s);
    rawtime = s.st_mtime;  // time of last data modification
    //time(&rawtime);  // current time 

    info = gmtime(&rawtime);  // what the time was in GMT
    // Create the header in proper format:
    strftime(buffer, 80, "Last-Modified: %a, %d %b %Y %X GMT\r\n", info);
    memcpy(lasthdr, buffer, strlen(buffer)); 
    //printf("%s", lasthdr);
}

/**
 * This functin determines if a file has been modified since the time
 * the client asked.
 */
bool isFileModified(char* ifModSinceHdr, char* fileName) {
    char temp[100];
    char* token;
    memcpy(temp, ifModSinceHdr, strlen(ifModSinceHdr));

    // get all values from the changed since string
    token = strtok(temp, " ,:");
    printf("%s\n", token);
    int i = 1;
    char day[] = strtok(NULL, " ,:");
    int dayInt = atoi(strtok(NULL, " ,:"));
    int month = atoi(strtok(NULL, " ,:"));
    int year = atoi(strtok(NULL, " ,:"));
    int hour = atoi(strtok(NULL, " ,:"));
    int minute = atoi(strtok(NULL, " ,:"));
    int second = atoi(strtok(NULL, " ,:"));

    // get values from file string
    char dayf[] = ;
    int dayIntf = atoi(strtok(NULL, " ,:"));
    int monthf = atoi(strtok(NULL, " ,:"));
    int yearf = atoi(strtok(NULL, " ,:"));
    int hourf = atoi(strtok(NULL, " ,:"));
    int minutef = atoi(strtok(NULL, " ,:"));
    int secondf = atoi(strtok(NULL, " ,:"));

    // now compare values to find if changed since:
    

    // while((token=strtok(NULL, " ,:")) != NULL){
    //     printf("%s\n", token);
        


    //     ++i;
    // }
    
    return true;
}

/**
 * Return a content type header.
 * todo wa to find content type without hard coding? 
 */
void createContentTypeHeader(char* cthdr, char* mt) {
    sprintf(cthdr, "Content-Type: %s\r\n", mt);
    //printf(cthdr);
}