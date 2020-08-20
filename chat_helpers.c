#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "socket.h"
#include "chat_helpers.h"

int write_buf_to_client(struct client_sock *c, char *buf, int len) {
    // To be completed.  
    if (buf[len] != '\0'){
        return 1;
    }
    buf[len] = '\r';
    buf[len+1] = '\n';
    return write_to_socket(c->sock_fd, buf, len+2);
}

int remove_client(struct client_sock **curr, struct client_sock **clients) {
    struct client_sock **pointer = clients;   //<-- keep track of head
    if ((*curr) == (*clients)){
        (*curr) = (*clients)->next;
        (*clients) = (*curr);
        return 0;
    }
    else{
        while ((*clients) != NULL){
            if ((*clients)->next == (*curr)){
                (*clients)->next = (*curr)->next;
                (*curr) = (*clients);
                clients = pointer;
                return 0;
            }
            (*clients) = (*clients)->next;
        }
        clients = pointer;
    }
    return 1; // Couldn't find the client in the list, or empty list
}

int read_from_client(struct client_sock *curr) {
    return read_from_socket(curr->sock_fd, curr->buf, &(curr->inbuf));
}

/*returns 1 if space is there, 0 if not*/
int checkSpace(char * name){
    for (int i = 0; i < strlen(name); i++){
        if (name[i] == ' '){
            return 1;
        }
    }
    return 0;
}

int set_username(struct client_sock *curr) {
    // To be completed. Hint: Use get_message().
    char *name;
    int possible = get_message(&name, (curr->buf), &(curr->inbuf));
    if (possible == 1 || checkSpace(name)){
        return 1;
    }
    curr->username = name;
    return 0;
}


