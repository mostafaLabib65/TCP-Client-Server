/*
** server.c -- a stream socket server demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "Serve.h"
#include <sys/wait.h>
#include <signal.h>
#include <iostream>


#define BACKLOG 10 // how many pending connections queue will hold

int numOfProcesses = 0;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
    numOfProcesses--;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void init_timer(){
    if (!fork()) {

    }
}

int main(int argc, char *argv[]){
    int socketFileDescriptor, new_fd; // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *successfulServInfo;
    struct sockaddr_storage their_addr; // connector'IPAddress address information
    socklen_t sin_size;
    struct sigaction signalAction;
    int yes=1;
    char IPAddress[INET6_ADDRSTRLEN];
    int rv;
    if (argc != 2) {
        fprintf(stderr,"Error: Port number is missing.\n");
        return 1;
    }
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP
    if ((rv = getaddrinfo(nullptr, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and bind to the first we can
    for(successfulServInfo = servinfo; successfulServInfo != nullptr; successfulServInfo = successfulServInfo->ai_next) {
        if ((socketFileDescriptor = socket(successfulServInfo->ai_family, successfulServInfo->ai_socktype,
                                           successfulServInfo->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        if (setsockopt(socketFileDescriptor, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(socketFileDescriptor, successfulServInfo->ai_addr, successfulServInfo->ai_addrlen) == -1) {
            close(socketFileDescriptor);
            perror("server: bind");
            continue;
        }
        break;
    }
    if (successfulServInfo == nullptr) {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (listen(socketFileDescriptor, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }
    signalAction.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&signalAction.sa_mask);
    signalAction.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &signalAction, nullptr) == -1) {
        perror("sigaction");
        exit(1);
    }
    printf("server: waiting for connections...\n");
    while(1) { // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(socketFileDescriptor, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  IPAddress, sizeof IPAddress);
        cout << "-----------------------------------\n";
        printf("server: got connection from %s\n", IPAddress);
        numOfProcesses ++;
        if (!fork()) { // this is the child process
            close(socketFileDescriptor); // child doesn't need the listener
            while (true){
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(new_fd, &read_fds);
                struct timeval tv;
                cout << numOfProcesses << endl;
                tv.tv_sec = 100/numOfProcesses; // will
                tv.tv_usec = 0;
                int n;
                if ((n = select(new_fd+1, &read_fds, NULL, NULL, &tv)) == -1) {
                    perror("select");
                    exit(4);
                } else if(n == 0){
                    cout << "Client is idle closing the connection\n";
                    close(new_fd);
                    exit(0);
                }
                Serve *serve_request = new Serve(new_fd);
                serve_request->serve_request();
            }
            exit(0);
        }
        close(new_fd); // parent doesn't need this
    }
}