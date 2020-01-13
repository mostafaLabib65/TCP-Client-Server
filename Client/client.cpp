#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include <vector>
#include "Serve.h"
#include <fstream>
#include "response.h"
#include "httpresponseparser.h"
#include "request.h"

using namespace httpparser;
using namespace std;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
vector<string> split(const string& requests_to_be_splitted, const string& delimeter) {
    std::vector<std::string> splitted_requests;
    int startIndex = 0;
    int endIndex = 0;
    while ((endIndex = requests_to_be_splitted.find(delimeter, startIndex)) < requests_to_be_splitted.size()) {

        std::string val = requests_to_be_splitted.substr(startIndex, endIndex - startIndex);
        splitted_requests.push_back(val);
        startIndex = endIndex + delimeter.size();

    }
    if (startIndex < requests_to_be_splitted.size()) {
        std::string val = requests_to_be_splitted.substr(startIndex);
        splitted_requests.push_back(val);
    }
    return splitted_requests;
}

Request prepare_get_request(string command){
    vector<string> splitted_command = split(command, " ");
    string host_number = "80";
    if(splitted_command.size() == 4)
        host_number = splitted_command[3];
    Request request;
    request.method = "GET";
    request.keepAlive = true;
    request.versionMinor = 1;
    request.versionMajor = 1;
    request.uri = splitted_command[1];
    request.add_header("Accept", "text/html,image/png,image/jpg");
    request.add_header("Host", splitted_command[2] + ":" + splitted_command[3]);
    return request;
}

Request prepare_post_request(string command){
    vector<string> splitted_command = split(command, " ");
    string host_number = "80";
    if(splitted_command.size() == 4)
        host_number = splitted_command[3];
    Request request;
    request.method = "POST";
    request.keepAlive = true;
    request.versionMinor = 1;
    request.versionMajor = 1;
    request.uri = splitted_command[1];
    string path = splitted_command[1];
    ifstream file(path.erase(0,1), std::ofstream::binary);
    string content;
    if(file) {
        ostringstream ss;
        ss << file.rdbuf();
        content = ss.str();
        vector<char> v(content.begin(), content.end());
        request.content = v;
        request.add_header("Content-Length", to_string(content.length()));
    }
    request.add_header("Host", splitted_command[2] + ":" + splitted_command[3]);
    return request;
}
int main(int argc, char *argv[])
{
    string number = "80";
    char* host = argv[1];
    if(argc == 3){
        number = argv[2];
    }
    char port[number.length() + 1];
    strcpy(port, number.c_str());
    int sockfd, numbytes;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != nullptr; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == nullptr) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure
    vector<Request> requests;
    string command;
    ifstream myfile ("requests.txt");
    if (myfile.is_open()){
        while ( myfile.good() ){
            getline (myfile, command);
            if(command[7] == 'g'){
                requests.push_back(prepare_get_request(command));
            } else{
                requests.push_back(prepare_post_request(command));
            }
        }
        myfile.close();
    }else {
        cout << "Unable to open file";
        return 1;
    }
    string requestss = "";
    for(int i = 0; i < requests.size(); i++){
        string req = requests[i].inspect();
        requestss = requestss + req;
        cout << req;
    }
    const char* char_array = requestss.data();
    if (send(sockfd , char_array , requestss.length()+1 , 0 ) == -1)
        perror("send");

    Serve *serve_request = new Serve(sockfd, requests);
    serve_request->serve_request();
    sleep(700);
    return 0;
}
