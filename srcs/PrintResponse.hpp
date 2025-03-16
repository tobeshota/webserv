#pragma once

// #include "HandleSuccess.hpp"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFER_SIZE 1024

class PrintResponse {
private:
public:
  PrintResponse();
  ~PrintResponse();
  void send_http_response(int client_socket, const char *filename);
  void send_header(int client_socket, FILE *file);
};
