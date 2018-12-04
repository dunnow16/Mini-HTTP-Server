/**
 * Owen Dunn, Reuben Wattenhofer, Cody Krueger
 * Project 4: Mini HTTP Server 
 * CIS 457 Data Communications
 * 3 DEC 2018
 */

//
// compile: g++ httpServer.cpp -o s
//
// run from browser: http://localhost:<portnum>/index.html
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
#include <fstream>

#define DEFAULT_SERVER_PORT 8080

using namespace std;

struct fileData {
    char* data;
    int length;
};

//Function Headers
void createDateHeader(char* datehdr);
void createLastModHeader(char* lasthdr, char* fileName);
// void createContentTypeHeader(char* cthdr, char* mt);
char* createContentTypeHeader(char* fileName);

char* createStatus(int code);
char* httpHeader (char* fileName, int code, int sock);
int sendFile(char* fileName, int sock);

char* getFileExtension(char* fileName);
char* createContentLength(char* fileName);
// char* createContentLength(int length);

bool fileExists(char* fileName);

bool logInfo(string fileName, bool logToFile, string message);

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
    //TESTING
    char* lasthdr = new char[80];
    char* temp = (char*)logfile.c_str();
    createLastModHeader(lasthdr, temp); 
    //TESTING
    // char tmp2[] = "pdf"; 
    // char* cthdr = new char[80];
    // createContentTypeHeader(cthdr, tmp2);

    // Open file for writing and return pointer to file object.
    // test file / stdout writing
    FILE* pFile;
    if (isLogFile) {
        // pFile = fopen(logfile.c_str(), "w");
        // fprintf(pFile, lasthdr);
        // fclose(pFile);  // must close for file to update? (tested this)
    }
    //printf(lasthdr);

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

                    
                    int len = sizeof(clientaddr);
                    int clientsocket = 
                        accept(sockfd, (struct sockaddr*)&clientaddr, (socklen_t*) &len);

                    logInfo(logfile, isLogFile,
                        "A client connected on " + to_string(clientsocket) + "\n");

                    // Set timeout for each socket.
                    struct timeval timeout; 
                    timeout.tv_sec = 20; 
                    timeout.tv_usec = 0;

                    setsockopt(clientsocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
                    FD_SET(clientsocket, &sockets);
                }
                // Existing client, serve them
                else {
                    char* line = new char[5000];  // TODO will mem leaks cause a crash if runs too long?
                    // unsigned char* line = new unsigned char[5000];
                    // i is our socket number
                    unsigned int recv_len = recv(i, line, 5000, 0);
                    // Existing client, serve them
                    //if client closes connection we will recieve 0 bytes
                    if(recv_len == 0) {
                        //if the client has closed the connection 
                        //we need to remove the client from the list of sockets
                        logInfo(logfile, isLogFile,
                         "Received zero bytes. Client closed connection.\n");

                        close(i);
						FD_CLR(i, &sockets);
                        continue;
                    }


                    // printf("got:\n%s\n", line);

                    logInfo(logfile, isLogFile,
                        "got:\n" + string(line) + "\n");

                    std::stringstream ss(line);
                    std::string to;

                    int lineNumber = 0;
                    if (line != NULL) {
                        while(std::getline(ss,to,'\n')) {
                            cout << to <<endl;

                            lineNumber++;

                            // Close connection                            
                            string connection_close = "Connection: close";
                            if (strncmp(to.c_str(), connection_close.c_str(), connection_close.length()) == 0) {
                                logInfo(logfile, isLogFile,
                                    "Connection closed by client\n");
                                close(i);
                                FD_CLR(i, &sockets);
                                continue;
                            }
                            // GET request, process it.
                            else if (strncmp(to.c_str(), "GET", 3) == 0) {
                                
                                // logInfo(logfile, isLogFile,
                                //     "GET request\n");
                                // printf("GET request\n");
                                // char noget[] = "Status-Line = HTTP/1.1 200 OK\r\n\r\n";
                                // char noget[] = "HTTP/1.1 404 Not Found\r\n\r\n";
                                // send(i, noget, strlen(noget), 0);

                                // char temp[] = "index.html";
                                // char temp2[] = "OK";
                                // char* response = httpHeader(temp, 200, temp2);

                                // char* response = httpHeader("index.html", 200, i);
                                //cout << response << endl;
                                
                                string delimiter = " ";
                                // for (int j = 0; j < 2; j++) {
                                    // string token = to.substr(0, to.find(delimiter));
                                    to.erase(0, to.find(delimiter) + delimiter.length());
                                    string token = to.substr(0, to.find(delimiter));
                                    // cout << "address: " << token << "\n";

                                    char* fullPath = new char[100];
                                    strcpy(fullPath, directory.c_str());
                                    strcat(fullPath, token.c_str());
                                    
                                    // if (fileExists(((char*)token.c_str())+1)) {
                                    if (fileExists(fullPath)) {
                                        // char* response = httpHeader(((char*)token.c_str())+1, 200, i);
                                        char* response = httpHeader(fullPath, 200, i);
                                        logInfo(logfile, isLogFile,
                                            response);
                                        // logInfo(logfile, isLogFile,
                                        //     "file sent\n");
                                    } else {
                                        char* response = httpHeader("404NotFound.html", 404, i);
                                        logInfo(logfile, isLogFile,
                                            response);
                                        // logInfo(logfile, isLogFile,
                                        //     "file not found\n");
                                    }

                                    // Don't let the client go outside the base directory!        
                                    if (strncmp(token.c_str(), "/..", 3) == 0) {
                                        // cout << "Client tried to escape the base directory\n";
                                        char* response = httpHeader("400BadRequest.html", 400, i);
                                        logInfo(logfile, isLogFile,
                                            response);
                                        // logInfo(logfile, isLogFile,
                                        //     "Client tried to escape the base directory\n");
                                        continue;
                                    }
                                    
                                // }
                            }
                            // Not a GET request!!!!
                            else if (lineNumber == 1) {
                                // printf("not a GET request\n");
                                char* response = httpHeader("501NotImplemented.html", 501, i);
                                // logInfo(logfile, isLogFile,
                                //     "Client attempted a non-GET request\n");
                                logInfo(logfile, isLogFile,
                                    response);

                                // char* m
                                // send(i, ) HTTP/1.1 404 Not Found
                                //"HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n"
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

bool logInfo(string fileName, bool logToFile, string message) {
    if (logToFile) {
        std::ofstream outfile;

        outfile.open(fileName.c_str(), std::ios_base::app);
        outfile << message;
    }
    cout << message;
    return true;
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
    // strftime(buffer, 80, "Date: %a, %d %b %Y %X GMT\r\n", info);
    memcpy(datehdr, buffer, strlen(buffer)+1);
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
 * 
 * x/status code and text
 * x/date
 * /last-modified
 * /content-type
 * x/length
 * x/file data
 * 
 **/
char* httpHeader (char* fileName, int code, int sock) {
    char* content = new char[6000];
    
    // cout << getFileExtension(fileName);

    //produce fields to add to header
    char* statusField = createStatus(code);
    char* dateField = new char[50];
    createDateHeader(dateField);
    char* contentTypeField = createContentTypeHeader(fileName);
    // Content-Length: <length>
    char* contentLengthField = createContentLength(fileName);

    char* lastModifiedField = new char[100];
    createLastModHeader(lastModifiedField, fileName);


    strcpy(content, statusField);
    strcat(content, dateField);
    strcat(content, lastModifiedField);
    strcat(content, contentLengthField);
    strcat(content, contentTypeField);
    strcat(content, "\r\n");

    // memcpy(content)

    //send header
    send(sock, content, strlen(content), 0);
    if (code == 304) {
        //do not resend file if it is in their cache
    } else {
        sendFile(fileName, sock);
    }


    delete statusField;
    delete dateField;
    delete contentLengthField;

    cout << content << endl;

    return content;
}

// char* createContentLength(int length) {//char* fileName) {
char* createContentLength(char* fileName) {
    char* contentLength = new char[100];

    strcpy(contentLength, "Content-Length: ");
    // cout << "test: " << contentLength << endl;

    // int fileLength = 0;
    FILE *fptr;
    size_t length = 0; 
    ssize_t read;
    char* data = new char[5000];

    // from https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
    // line is fileName here
    if( access( fileName, F_OK ) != -1 ) {
        fptr = fopen(fileName, "r");

        fseek (fptr, 0, SEEK_END);   // non-portable
        read = ftell (fptr);        // printf("Sending, size is %zd\n", read);
        fclose(fptr);
    } else {
        // file doesn't exist
        printf("The file does not exist!\n");
    }

    strcat(contentLength, to_string(read).c_str());
    strcat(contentLength, "\r\n");

    return contentLength;
}

/**
 * returns the appropriate html file type for content header
 * returns 501 for a file type that is unsupported
 */
char* getFileExtension(char* fileName) {
    char* extension = new char[25];
    if (strncmp( fileName + strlen(fileName) - 4, "html", 4) == 0) {
        strcpy(extension, "text/html");
    }
    else if (strncmp( fileName + strlen(fileName) - 3, "txt", 3) == 0) {
        strcpy(extension, "text/plain");
    }
    else if ((strncmp( fileName + strlen(fileName) - 3, "jpg", 3) == 0 ) || 
        (strncmp( fileName + strlen(fileName) - 4, "jpeg", 4) == 0 )) {
        strcpy(extension, "image/jpeg");
    }
    else if (strncmp( fileName + strlen(fileName) - 3, "pdf", 3) == 0) {
        strcpy(extension, "application/pdf");
    }
    else {
        memcpy(extension, "501", 3);
    }

    return extension;
}

/**
 * Return a content type header.
 * todo way to find content type without hard coding? 
 */
// void createContentTypeHeader(char* cthdr, char* mt) {
//     sprintf(cthdr, "Content-Type: %s\r\n", mt);
//     //printf(cthdr);
// }
char* createContentTypeHeader(char* fileName) {
    char* cthdr = new char[100];

    char* xt = getFileExtension(fileName);

    sprintf(cthdr, "Content-Type: %s\r\n", xt);

    return cthdr;
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
    memcpy(lasthdr, buffer, strlen(buffer)+1);
    //printf("%s", lasthdr);
}

bool fileExists(char* fileName) {
    // from https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
    // line is fileName here
    if( access( fileName, F_OK ) != -1 ) {
        return true;
    } else {
        return false;
    }
}

int sendFile(char* fileName, int sock) {
    FILE *fptr;
    size_t length = 0; 
    ssize_t read;
    char* data = new char[5000];
    int total = 0;

    // struct fileData* fData = new struct fileData;

    //if ((fptr = fopen("C:\\program.txt","r")) == NULL){
    //    printf("Error! opening file");
    //    // Program exits if the file pointer returns NULL.
    //    exit(1);
    //}

    // from https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
    // line is fileName here
    if( access( fileName, F_OK ) != -1 ) {
        // file exists
        printf("File found.\n");
        fptr = fopen(fileName, "r");
        //read = getline(&line2, &length, fptr);
        // read = fread(data, 1, 5000, fptr);
        // printf("Sending, size is %zd\n", read);
        // send(sock, data, read, 0);      
        // fData->data = data;
        // fData->length = read; 
        
        // while ((read = getline(&line2, &length, fptr)) != -1) {
        //    printf("%s", line2); //for error checking
        //    //strcat(line3, line2);
        //    send(clientsocket, line2, strlen(line2), 0);
        // }
        //send(clientsocket, line3, strlen(line3)+1, 0);
        
        while ((read = fread(data, 1, 5000, fptr)) > 0) {
            send(sock, data, read, 0);
            total+=read;
        }

        fclose(fptr);
        return total;
    } else {
        // file doesn't exist
        printf("The file does not exist!\n");
        return 0;
    }
    // return fData;
}

