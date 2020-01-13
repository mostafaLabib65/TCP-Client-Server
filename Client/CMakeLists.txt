cmake_minimum_required(VERSION 3.15)
project(Client)

set(CMAKE_CXX_STANDARD 14)

add_executable(Client client.cpp httprequestparser.h httpresponseparser.h request.h response.h urlparser.h Serve.cpp Serve.h)