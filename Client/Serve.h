//
// Created by mostafa on 28/10/2019.
//

#ifndef SERVER_SERVE_H
#define SERVER_SERVE_H

#include "request.h"
#include "httprequestparser.h"
#include "response.h"

using namespace httpparser;
using namespace std;

class Serve{
private:
    int socketFileDescriptor;
    int number_of_responses;
    vector<Request> requests;
    Response response;
    void serve_get_response(string content, int request_index);
    string get_file_name(string path);
    int get_header_length();
    vector<string> split(const string& requests_to_be_splitted, const string& delimeter);
    int get_data(vector<string> splitted_responses, int index, int request_index);
    bool is_get_response();

public:
    Serve(int socketFileDescriptor, vector<Request> requests){
        this->socketFileDescriptor = socketFileDescriptor;
        this->number_of_responses = requests.size();
        this->requests = requests;
    }

    void serve_request();
};
#endif //SERVER_SERVE_H
