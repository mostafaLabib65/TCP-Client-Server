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
    Request request;
    Response response;
    void serve_get_request();
    void serve_post_request();
    string prepare_success_response(string data);
    string prepare_success_response();
    string prepare_failure_response();
    Response init_response();
    string get_file_name(string path);
    int get_header_length();
    vector<string> split(const string& requests_to_be_splitted, const string& delimeter);
    int get_data(vector<string> splitted_requests, int index);

public:
    Serve(int socketFileDescriptor){
        this->socketFileDescriptor = socketFileDescriptor;
    }

    void serve_request();
};
#endif //SERVER_SERVE_H
