#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <fcntl.h> 

#include "socket.h"

struct server_sock {
    int sock_fd;
    char buf[BUF_SIZE];
    int inbuf;
};

void parser(char *get_message){
    //why is there a \276??
    char *name = malloc(sizeof(char) * (MAX_NAME + 2));
    int counter = 0;
    int protoCode = (int) get_message[counter];
    counter+=1;
    while (get_message[counter] != ' ')
    {   
        name[counter] = get_message[counter];
        counter++;
    }
    if (protoCode == 49){
        name[counter] = '\0';
        char *the_message = malloc(sizeof(char) * (MAX_USER_MSG + 1));
        counter += 1;
        int pos = 0;
        while (counter < strlen(get_message)){
            the_message[pos] = get_message[counter];
            pos += 1;
            counter += 1;
        }
        the_message[pos] = '\0';
        printf("%s: %s\n", name+1, the_message);
	fflush(stdout);
        free(the_message);
    }
    free(name);
}


int main(void) {
    struct server_sock s;
    s.inbuf = 0;
    int exit_status = 0;
    
    // Create the socket FD.
    s.sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s.sock_fd < 0) {
        perror("client: socket");
        exit(1);
    }

    // Set the IP and port of the server to connect to.
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1) {
        perror("client: inet_pton");
        close(s.sock_fd);
        exit(1);
    }

    // Connect to the server.
    if (connect(s.sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("client: connect");
        close(s.sock_fd);
        exit(1);
    }

    char *buf = NULL; // Buffer to read name from stdin
    int name_valid = 0;
    while(!name_valid) {
        printf("Please enter a username: ");
        fflush(stdout);
        size_t buf_len = 0;
        size_t name_len = getline(&buf, &buf_len, stdin);
        if (name_len < 0) {
            perror("getline");
            fprintf(stderr, "Error reading username.\n");
            free(buf);
            exit(1);
        }
        
        if (name_len - 1 > MAX_NAME) { // name_len includes '\n'
            printf("Username can be at most %d characters.\n", MAX_NAME);
        }
        else {
            // Replace LF+NULL with CR+LF
            buf[name_len-1] = '\r';
            buf[name_len] = '\n';
            char newName[MAX_NAME+3];
            strcpy(newName, "1");
            strncat(newName, buf, name_len+1);
            if (write_to_socket(s.sock_fd, newName, name_len+2)) {
                fprintf(stderr, "Error sending username.\n");
                free(buf);
                exit(1);
            }
            name_valid = 1;
            free(buf);
        }
    }
    
    /*
     * See here for why getline() is used above instead of fgets():
     * https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=87152445
     */
    
    /*
     * Step 1: Prepare to read from stdin as well as the socket,
     * by setting up a file descriptor set and allocating a buffer
     * to read into. It is suggested that you use buf for saving data
     * read from stdin, and s.buf for saving data read from the socket.
     * Why? Because note that the maximum size of a user-sent message
     * is MAX_USR_MSG + 2, whereas the maximum size of a server-sent
     * message is MAX_NAME + 1 + MAX_USER_MSG + 2. Refer to the macros
     * defined in socket.h.
     */
    buf = malloc(sizeof(char) * (MAX_USER_MSG + 2));

    fd_set copy_fs, read_fs;
    FD_ZERO(&read_fs);
    FD_ZERO(&copy_fs);
    FD_SET(s.sock_fd, &read_fs);
    FD_SET(STDIN_FILENO, &read_fs);
    
    int max_fd;
    if (STDIN_FILENO > s.sock_fd){
        max_fd = STDERR_FILENO + 1;
    }
    else{
        max_fd = s.sock_fd + 1;
    }
    
    /*
     * Step 2: Using select, monitor the socket for incoming mesages
     * from the server and stdin for data typed in by the user.
     */
    while(1) {
        copy_fs = read_fs;
        int ready = select(max_fd, &copy_fs, NULL, NULL, NULL);
        if (ready == -1) {
            perror("server: select");
            exit_status = 1;
            break;
        }
        /*
         * Step 3: Read user-entered message from the standard input
         * stream. We should read at most MAX_USR_MSG bytes at a time.
         * If the user types in a message longer than MAX_USR_MSG,
         * we should leave the rest of the message in the standard
         * input stream so that we can read it later when we loop
         * back around.
         * 
         * In other words, an oversized messages will be split up
         * into smaller messages. For example, assuming that
         * MAX_USR_MSG is 10 bytes, a message of 22 bytes would be
         * split up into 3 messages of length 10, 10, and 2,
         * respectively.
         * 
         * It will probably be easier to do this using a combination of
         * fgets() and ungetc(). You probably don't want to use
         * getline() as was used for reading the user name, because
         * that would read all the bytes off of the standard input
         * stream, even if it exceeds MAX_USR_MSG.
         */
        if (FD_ISSET(STDIN_FILENO, &copy_fs)){
            fgets(buf, MAX_USER_MSG + 1, stdin);
            int num = (int) buf[0];
            int num2 = (int) buf[1];
            int length = strlen(buf);
            if (num == 46 && num2 == 107){ //do error check
                char newBuf[MAX_NAME+5];
                newBuf[0] = '\0';
                strcpy(newBuf, "0");
                strncat(newBuf, buf+3, length-3);
                newBuf[strlen(newBuf)-1] = '\r';
                newBuf[strlen(newBuf)-0] = '\n';
                write_to_socket(s.sock_fd, newBuf, length-1);
            }
            else
            {
                // int length = strlen(buf);
                char newbuf[MAX_USER_MSG+1];
                strcpy(newbuf, "1");
                buf[length-1] = '\r';
                buf[length] = '\n';
                strncat(newbuf, buf, length+1);
                write_to_socket(s.sock_fd, newbuf, length+2);
            }
            
        }

        /*
         * Step 4: Read server-sent messages from the socket.
         * The read_from_socket() and get_message() helper functions
         * will be useful here. This will look similar to the
         * server-side code.
         */
        if (FD_ISSET(s.sock_fd, &copy_fs)){
            int read_status = read_from_socket(s.sock_fd, s.buf, &(s.inbuf));
            if (read_status == -1){
                close(s.sock_fd);
                exit(1);
            }
            char * dest = malloc(sizeof(char) * MAX_USER_MSG + 1);
            int get_status = get_message(&dest, s.buf, &(s.inbuf));
            if (read_status == 0 && get_status == 0){
                parser(dest);
            }
        }
    }
    close(s.sock_fd);
    exit(exit_status);
}
