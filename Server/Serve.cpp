//
// Created by mostafa on 28/10/2019.
//

#include <cstdio>
#include <sys/socket.h>
#include <zconf.h>
#include <cstring>
#include <iostream>
#include "Serve.h"
#include "response.h"
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <streambuf>
#define MAXDATASIZE 8192
using namespace std;
using namespace httpparser;

void Serve::serve_request() {
    int valread;
    string remaining = "";
//    while(true){
        char buffer[MAXDATASIZE] = {0};
        if((valread = recv( this->socketFileDescriptor , buffer, MAXDATASIZE, 0)) == -1){
            perror("recv");
            exit(1);
        } else if(valread == 0){
            cout << "Client closed the connection\n";
            close(this->socketFileDescriptor);
            exit(1);
        }
//        cout << "--------------buffer--------------\n";
//        cout << buffer << endl;
//        cout << "-----------------------------------\n";
        string requests(buffer, valread);
        if(requests[0] == '\000' && requests.length() == 1)
            return;
        requests = remaining + requests;
        remaining = "";
        vector<string> splited_requests = split(requests, "\r\n\r\n");
        int i = 0;
        while( i < splited_requests.size()){
            Request request;
            HttpRequestParser parser;
            if(i == splited_requests.size() -1 && requests.substr(requests.length()-4,4) != "\r\n\r\n"){
                remaining = splited_requests[i];
                i++;
                continue;
            }
            string request_string = splited_requests[i];
            char char_array[request_string.length() + 1];
            strcpy(char_array, request_string.c_str());
            HttpRequestParser::ParseResult res = parser.parse(request, char_array, char_array + sizeof(char_array));
            cout << "-------------request-------------\n";
            this->request = request;
            cout << request.inspect() << endl;
            if(request.method == "GET"){
                serve_get_request();
                i++;
            } else{
                int result = get_data(splited_requests, i);
                serve_post_request();
                i+= (result+1);
            }
        }
//    }
}

void Serve::serve_get_request(){
    string response_msg;
    string path = this->request.uri;
    ifstream file(path.erase(0,1), std::ios::binary | std::ios::in);
    string content;
    if(file) {
        ostringstream ss;
        ss << file.rdbuf();
        content = ss.str();
        response_msg = prepare_success_response(content);
    } else{
        response_msg = prepare_failure_response();
    }
    const char* char_array = response_msg.data();
    cout << "--------------response---------------\n";
    cout << response_msg << endl;
    int v;
    if ((v = send(this->socketFileDescriptor, char_array, response_msg.length()+1, 0)) == -1)
        perror("send");
}
void Serve::serve_post_request() {
    string response_msg;
    response_msg = prepare_success_response();
    cout << "--------------response---------------\n";
    cout << response_msg << endl;
    char char_array[response_msg.length() + 1];
    strcpy(char_array, response_msg.c_str());
    if (send(this->socketFileDescriptor, char_array, response_msg.length()+1, 0) == -1)
        perror("send");
}
string Serve::prepare_success_response(string data){
    Response response = init_response();
    response.status = "OK";
    response.statusCode = 200;
    response.keepAlive = true;
    vector<char> v(data.begin(), data.end());
    response.content = v;
    response.add_header("Content-Length", to_string(data.length()));
    return response.inspect();
}
string Serve::prepare_success_response(){
    Response response = init_response();
    response.status = "OK";
    response.statusCode = 200;
    return response.inspect();
}

string Serve::prepare_failure_response(){
    Response response = init_response();
    response.status = "NOT FOUND";
    response.statusCode = 404;
    return response.inspect();
}
Response Serve::init_response(){
    Response response;
    response.versionMajor = this->request.versionMajor;
    response.versionMinor = this->request.versionMinor;
    response.keepAlive = this->request.keepAlive;
    return response;
}

string Serve::get_file_name(string path){
    int index = path.find_last_of('/');
    return path.substr(index+1);
}

int Serve::get_header_length(){
    for(int i = 0; i < request.headers.size(); i++){
        if(request.headers[i].name == "Content-Length"){
            return stoi(request.headers[i].value);
        }
    }
}


vector<string> Serve::split(const string& requests_to_be_splitted, const string& delimeter) {
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

int Serve::get_data(vector<string> splitted_requests, int index){
    string response_msg;
    string file_name = get_file_name(this->request.uri);
    std::ofstream outfile (file_name, std::ofstream::binary);

    int content_length = get_header_length();
    int read_content = 0;
    int j = 1;
    while(index+j < splitted_requests.size()){
        if(outfile.is_open()){
            outfile << splitted_requests[index+j];
            read_content += splitted_requests[index+j].length();
        } else
            cout << "Error while storing the file";
        if(read_content >= content_length || index + j == splitted_requests.size()-1)
            break;
        j++;
    }
    while(read_content < content_length){
        int valread;
        char remaining_data[MAXDATASIZE];
        if((valread = recv( this->socketFileDescriptor , remaining_data, MAXDATASIZE, 0)) == -1){
            perror("recv");
            exit(1);
        } else if(valread == 0){
            cout << "Client closed the connection\n";
            exit(1);
        }
        if(outfile.is_open()){
            outfile.write(remaining_data, valread);
            read_content += valread;
        } else
            cout << "Error while storing the file";
    }
    outfile.close();
    return j;
}


//    if( res != HttpRequestParser::ParsingCompleted ){
//        std::cerr << "Parsing failed" << std::endl;
//        EXIT_FAILURE;
//    }