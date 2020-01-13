cmake_minimum_required(VERSION 3.15)
project(Server)

set(CMAKE_CXX_STANDARD 14)

add_executable(Server Server.cpp Serve.h Serve.cpp response.h request.h urlparser.h httpresponseparser.h httprequestparser.h)