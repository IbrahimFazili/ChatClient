#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>     /* inet_ntoa */
#include <netdb.h>         /* gethostname */
#include <netinet/in.h>    /* struct sockaddr_in */

#include "socket.h"

void setup_server_socket(struct listen_sock *s) {
    if(!(s->addr = malloc(sizeof(struct sockaddr_in)))) {
        perror("malloc");
        exit(1);
    }
    // Allow sockets across machines.
    s->addr->sin_family = AF_INET;
    // The port the process will listen on.
    s->addr->sin_port = htons(SERVER_PORT);
    // Clear this field; sin_zero is used for padding for the struct.
    memset(&(s->addr->sin_zero), 0, 8);
    // Listen on all network interfaces.
    s->addr->sin_addr.s_addr = INADDR_ANY;

    s->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->sock_fd < 0) {
        perror("server socket");
        exit(1);
    }

    // Make sure we can reuse the port immediately after the
    // server terminates. Avoids the "address in use" error
    int on = 1;
    int status = setsockopt(s->sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (const char *) &on, sizeof(on));
    if (status < 0) {
        perror("setsockopt");
        exit(1);
    }

    // Bind the selected port to the socket.
    if (bind(s->sock_fd, (struct sockaddr *)s->addr, sizeof(*(s->addr))) < 0) {
        perror("server: bind");
        close(s->sock_fd);
        exit(1);
    }

    // Announce willingness to accept connections on this socket.
    if (listen(s->sock_fd, MAX_BACKLOG) < 0) {
        perror("server: listen");
        close(s->sock_fd);
        exit(1);
    }
}

/* Insert Tutorial 10 helper functions here. */

int find_network_newline(const char *buf, int inbuf) {
    int counter = 0;
    while (counter < inbuf-1)
    {
        if (buf[counter] == '\r' && buf[counter+1] == '\n'){
            return counter+2;
        }
        counter++;
    }
    return -1;
}

int read_from_socket(int sock_fd, char *buf, int *inbuf) {
    // if (*inbuf >= BUF_SIZE){
    //     return -1;
    // }
    int bytesRead = read(sock_fd, buf + *inbuf, BUF_SIZE - *inbuf);
    *inbuf += bytesRead;
    int line = find_network_newline(buf, *inbuf);
    if(bytesRead == -1){
        return -1;
    } 
    else if(bytesRead == 0){
        return 1;
    }
    
    if(line > -1){
        return 0;
    }
    else{
        return 2;
    }   
}

int get_message(char **dst, char *src, int *inbuf) {
    int networkLine = find_network_newline(src, *inbuf);
    if (networkLine == -1){
        return 1;
    }
    *dst = malloc(sizeof(char) * (networkLine));
    int counter = 0;
    while (counter < networkLine){ //check -2
        (*dst)[counter] = src[counter];
        counter++;
    }
    // strncpy(*dst, src, networkLine-2); //made this -2, next -1
    (*dst)[networkLine-2] = '\0'; 
    *inbuf -= networkLine;
    memmove(src, src + networkLine, *inbuf);
    return 0;
}

/* Helper function to be completed for Tutorial 11. */

int write_to_socket(int sock_fd, char *buf, int len) {
    if (sock_fd == 0){
        return 2;
    }
    int writeStatus = write(sock_fd, buf, len);
    if (writeStatus == -1){
        return 1;
    }
    else if (writeStatus == 0){
        return 2;
    }
    return 0;
}
