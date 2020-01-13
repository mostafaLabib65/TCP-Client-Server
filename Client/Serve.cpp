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
#include "httpresponseparser.h"
#include <string>
#include <fstream>
#include <boost/algorithm/string.hpp>
#include <streambuf>
#define MAXDATASIZE 1048576
using namespace std;
using namespace httpparser;

void Serve::serve_request() {
    int valread;
    string remaining = "";
    int responses_served = 0;
    while(responses_served != this->number_of_responses){
        char buffer[MAXDATASIZE] = {0};
        if((valread = recv( this->socketFileDescriptor , buffer, MAXDATASIZE, 0)) == -1){
            perror("recv");
            exit(1);
        } else if(valread == 0){
            cout << "Server closed the connection\n";
            exit(1);
        }
//        cout << "--------------buffer--------------\n";
//        cout << buffer << endl;
//        cout << "-----------------------------------\n";
        string requests(buffer, valread);
        requests = remaining + requests;
        remaining = "";
        vector<string> splited_requests = split(requests, "\r\n\r\n");
        int i = 0;
        while( i < splited_requests.size()){
            Response response;
            HttpResponseParser parser;
            if(i == splited_requests.size() -1 && requests.substr(requests.length()-4,4) != "\r\n\r\n"){
                remaining = splited_requests[i];
                i++;
                continue;
            }
            string request_string = splited_requests[i];
            if(request_string[0] == '\000' && request_string.substr(1,4) == "HTTP"){
                request_string = request_string.substr(1);
            }
            char char_array[request_string.length() + 1];
            strcpy(char_array, request_string.c_str());
            HttpResponseParser::ParseResult res = parser.parse(response, char_array, char_array + sizeof(char_array));
            cout << "-------------response-------------\n";
            this->response = response;
            cout << response.inspect() << endl;
            if(is_get_response()){
                int result = get_data(splited_requests, i, responses_served);
                i+= (result+1);
                responses_served++;
            } else{
                responses_served++;
                i++;
            }
        }
    }
}

string Serve::get_file_name(string path){
    int index = path.find_last_of('/');
    return path.substr(index+1);
}

int Serve::get_header_length(){
    for(int i = 0; i < response.headers.size(); i++){
        if(response.headers[i].name == "Content-Length"){
            return stoi(response.headers[i].value);
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

int Serve::get_data(vector<string> splitted_responses, int index, int request_index){
    Request request = requests[request_index];
    string file_name = get_file_name(request.uri);
    std::ofstream outfile (file_name, std::ofstream::binary);

    int content_length = get_header_length();
    int read_content = 0;
    int j = 1;
    while(index+j < splitted_responses.size()){
        if(outfile.is_open()){
            outfile << splitted_responses[index+j];
            read_content += splitted_responses[index+j].length();
        } else
            cout << "Error while storing the file";
        if(read_content >= content_length || index + j == splitted_responses.size()-1)
            break;
        j++;
    }
    //8279638
    while(read_content < content_length){
        int valread;
        char remaining_data[MAXDATASIZE];
        if((valread = recv( this->socketFileDescriptor , remaining_data, MAXDATASIZE, 0)) == -1){
            perror("recv");
            exit(1);
        } else if(valread == 0){
            cout << "Server closed the connection\n";
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

bool Serve::is_get_response() {
    for(int i = 0; i < response.headers.size(); i++){
        if(response.headers[i].name == "Content-Length"){
            return true;
        }
    }
    return false;
}
//    if( res != HttpRequestParser::ParsingCompleted ){
//        std::cerr << "Parsing failed" << std::endl;
//        EXIT_FAILURE;
//    }