/*
 * tcp-echo-server.c
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

static void die(const char *s) { perror(s); exit(1); }

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: %s <server_port <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
        exit(1);
    }

    unsigned short serverPort = atoi(argv[1]);
    //char *serverPort = argv[1];
    char *webRoot = argv[2];
    char *mdbHost = argv[3];
    unsigned short mdbPort = atoi(argv[4]);
   // char *mdbPort argv[4];

    struct hostent *he;

    // get server ip from server name
    if ((he = gethostbyname(mdbHost)) == NULL) {
	die("gethostbyname failed");
    }
    char *serverIP = inet_ntoa(*(struct in_addr *)he->h_addr);
   
    // Create a socket for TCP connection

    int sock; // socket descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");

    // Construct a server address structure for TCP

    struct sockaddr_in servaddress;
    memset(&servaddress, 0, sizeof(servaddress)); // must zero out the structure
    servaddress.sin_family      = AF_INET;
    servaddress.sin_addr.s_addr = inet_addr(serverIP); // IP conversion
    servaddress.sin_port        = htons(mdbPort); // must be in network byte order

    // Establish a TCP connection to the server

    if (connect(sock, (struct sockaddr *) &servaddress, sizeof(servaddress)) < 0)
        die("connect failed");

    FILE *cp = fdopen(sock, "r");
    if (cp == NULL) {
        perror("failed to create file");
        exit(1);
    }



    // Create a listening socket (also called server socket) 

    int servsock;
    if ((servsock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("socket failed");
 
   

    // Construct local address structure

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    servaddr.sin_port = htons(serverPort);

    // Bind to the local address

    if (bind(servsock, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
        die("bind failed");

    // Start listening for incoming connections

    if (listen(servsock, 5 /* queue size for connection requests */ ) < 0)
        die("listen failed");

    //char buf[10];

    int clntsock;
    socklen_t clntlen;
    struct sockaddr_in clntaddr;

    while (1) {

        // Accept an incoming connection

        clntlen = sizeof(clntaddr); // initialize the in-out parameter

        if ((clntsock = accept(servsock,
                        (struct sockaddr *) &clntaddr, &clntlen)) < 0)
            die("accept failed");

        // accept() returned a connected socket (also called client socket)
        // and filled in the client's address into clntaddr


        // Read the request from the client
        FILE *input = fdopen(clntsock, "r"); //reachable block
        if ((input = fdopen(clntsock, "r")) == NULL) {
            die("fdopen failed");
        }
      
        char response[1000]; 
        if (fgets(response, sizeof(response), input) == NULL) {
            if (ferror(input)) {
                die("IO error");
            }
            else {
                fprintf(stderr, "server terminated connection without response"); 
                fclose(input);
                    exit(1);
            }
        }

	char *token_separators = "\t \r\n"; // tab, space, new line
        char *method = "";
        char *requestURI = "";
        char *httpVersion = "";
	method = strtok(response, token_separators);
	requestURI = strtok(NULL, token_separators);
	httpVersion = strtok(NULL, token_separators);
        char statusCode[500];
        snprintf(statusCode, sizeof(statusCode), "HTTP/1.0 501 Not Implemented\r\n\r\n<html><body>\r\n<h1>501 Not Implemented</h1>\r\n</body></html>");

        if (method == NULL) {
	     if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
		 fclose(input);
		 continue;
	     }

            fclose(input);
            continue;
        }

	if (requestURI == NULL) {
	     if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
		 fclose(input);
		 continue;
	     }

	    fclose(input);
	    continue;
	}

	if (httpVersion == NULL) {
	     if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
		 fclose(input);
		 continue;
	     }

	    fclose(input);
	    continue;
	}

        char results[4096];

        const char *form =
	    "<h1>mdb-lookup</h1>\n"
	    "<p>\n"
	    "<form method=GET action=/mdb-lookup>\n"
	    "lookup: <input type=text name=key>\n"
	    "<input type=submit>\n"
	    "</form>\n"
	    "<p>\n";

	char responseHeader[4096];
	snprintf(responseHeader, sizeof(responseHeader), "HTTP/1.0 200 OK\r\n\r\n");


        char first[100];

        if (strstr(requestURI, "/mdb-lookup") != NULL) {
            if (strcmp(requestURI, "/mdb-lookup") == 0) {
                
                 if (send(clntsock, responseHeader, strlen(responseHeader), 0) != strlen(responseHeader)) {
                         fclose(input);
                         continue;
                 }

                snprintf(first, sizeof(first), "%s \"%s %s %s\" 200 OK\r\n", inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
                fprintf(stdout, "%s", first);
                
                if (send(clntsock, form, strlen(form), 0) != strlen(form)) {
                    fclose(input);
                    continue;
                }
            }
         if (strstr(requestURI, "/mdb-lookup?key=") != NULL) {
             send(clntsock, responseHeader, strlen(responseHeader), 0);

             char *word = "";
             word = strrchr(requestURI, '=');
             word++; //remember to append \n
	     snprintf(first, sizeof(first), "looking up [%s]: %s \"%s %s %s\" 200 OK\r\n", word, inet_ntoa(clntaddr.sin_addr), method, requestURI, httpVersion);
	     fprintf(stdout, "%s", first);

             strcat(word, "\n");
             if (send(sock, word, strlen(word), 0) != strlen(word)) { // send WHERE?
                 fclose(input);
                 continue;
             }
             
             // read results line by line
             char begin[100];
             snprintf(begin, sizeof(begin), "<html><body>\r\n"); 

             char tableBegin[100];
             snprintf(tableBegin, sizeof(tableBegin), "<p><table border>");

             char tableEnd[100];
             snprintf(tableEnd, sizeof(tableEnd), "</table>\r\n</body></html>");

             if (send(clntsock, begin, strlen(begin), 0) != strlen(begin)) {
                 fclose(input);
                 continue;
             }

	     if (send(clntsock, form, strlen(form), 0) != strlen(form)) {
		 fclose(input);
		 continue;
	     }

	     if (send(clntsock, tableBegin, strlen(tableBegin), 0) != strlen(tableBegin)) {
		 fclose(input);
		 continue;
	     }



             int counter = 0;
             while (fgets(results, sizeof(results), cp) != NULL && strcmp(results, "\n") != 0) {
                 char tableRow[500] = "";


                 if (counter % 2 == 0) {
                     snprintf(tableRow, sizeof(tableRow), "<tr><td>%s\r\n", results);
                 }

                 else {
                     snprintf(tableRow, sizeof(tableRow), "<tr><td bgcolor=yellow>%s\r\n", results);
                 }

                 if (send(clntsock, tableRow, strlen(tableRow), 0) != strlen(tableRow)) {
                     fclose(input);
                     continue;
                 }

                 counter++;

             }

             if (send(clntsock, tableEnd, strlen(tableEnd), 0) != strlen(tableEnd)) {
                 fclose(input);
                 continue;
             }

              if (send(clntsock, tableEnd, strlen(tableEnd), 0) != strlen(tableEnd)) {
                 fclose(input);
                 continue;
              }

         }

      }

        else { 
        char respondToLine[500];
        snprintf(respondToLine, sizeof(respondToLine), "HTTP/1.0 400 Bad Request\r\n\r\n<html><body>\r\n<h1>400 Bad Request</h1>\r\n</body></html>");

        if(requestURI[0] != '/') {
             if (send(clntsock, respondToLine, strlen(respondToLine), 0) != strlen(respondToLine)) {
                 fclose(input);
                 continue;
                 //close and continue
             }
        }

        char *endURI = strrchr(requestURI, '/');
        char *contains = strstr(requestURI, "/../");
        char URI[500];
        
        //strcat(URI, requestURI);
        strncpy(URI, requestURI, sizeof(URI)); 


        char firstLine[4096];

        snprintf(firstLine, sizeof(firstLine), "%s \"%s %s %s\" 200 OK\r\n", inet_ntoa(clntaddr.sin_addr), method, URI, httpVersion);

        if (strncmp("HTTP/1.0", httpVersion, 8) != 0 && strncmp("HTTP/1.1", httpVersion, 8) != 0) { 
            if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
                fclose(input);
                continue;
            }
            fprintf(stderr, "i am here");
            fclose(input);
            continue;
        }

        if ((contains != NULL) || (strncmp("/..", endURI, 4) == 0)) {
           if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
               fclose(input);
               continue;
           }
           fclose(input);
           continue;
        }

        if (strncmp("GET", method, 8) != 0) {
            if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
                    fclose(input);
                    continue;
            }
            fclose(input);
            continue;
        }

        char noFile[500];
        sprintf(noFile, "404 Not Found\r\n<html><body>\r\n<h1>404 Not Found>/h1>\r\n</body></html>");

        if (requestURI[strlen(requestURI) - 1] == '/') {
            //append
            strcat(URI, "index.html");
        }

	char path[500];
	snprintf(path, sizeof(path), "%s%s", webRoot, URI);

	struct stat statStructure;
	struct stat *statPointer = &statStructure;
	
 
	if (stat(path, statPointer) == -1) {
	    if (send(clntsock, noFile, strlen(noFile), 0) != strlen(noFile)) {
		fclose(input);
		continue;
	    }
        }
 
        else {
            if S_ISDIR(statStructure.st_mode) {
                if (requestURI[strlen(requestURI) - 1] != '/') {
		    if (send(clntsock, statusCode, strlen(statusCode), 0) != strlen(statusCode)) {
			fclose(input);
			continue;
		    }
                }

            }
        }

        //static requests only
	FILE *fp = fopen(path, "rb");
	if (fp == NULL) {
            if (send(clntsock, noFile, strlen(noFile), 0) != strlen(noFile)) {
                fclose(input);
                continue;
            }
	    fclose(input);
            continue;
	}

	char headers[4096]; 
        for (;;) {
	     if (fgets(headers, sizeof(headers), input) == NULL) {
		 if (ferror(input))
		     die("IO error");
		 else {
		     fprintf(stderr, "server terminated connection without sending file");
		     fclose(input);
		     exit(1);
		 }
	     }
	     if (strcmp("\r\n", headers) == 0) {
		 // this marks the end of header lines
		 // get out of the for loop.
		 break;
	     }
         }

            if (send(clntsock, responseHeader, strlen(responseHeader), 0) != strlen(responseHeader)) {
                fclose(input);
                continue;
            }
        

        fprintf(stdout, "%s", firstLine);


        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
            if (send(clntsock, buf, n, 0) != n) {
                die("send failed");
            }
            //die?
        }
        fclose(fp);
        fclose(input);
        fprintf(stderr, "%s", "i have closed");

        if (ferror(input)) {
            die("fread failed");
        }

        }

        // Client closed the connection (r==0) or there was an error
        // Either way, close the client socket and go back to accept()
    close(clntsock);

    }
    fclose(cp);
    close(sock);
}
