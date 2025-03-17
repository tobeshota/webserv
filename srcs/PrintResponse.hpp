#pragma once

#ifndef PRINTRESPONSE_HPP
#define PRINTRESPONSE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include "Handler.hpp"
#include "HTTPResponse.hpp"
#include <stdexcept>

#include <iostream>
#define BUFFER_SIZE 1024

class PrintResponse :public Handler {
 private:
  int client_socket;
 public:
  PrintResponse(int client_socket);
  ~PrintResponse();
  void handleRequest(HTTPResponse& httpResponse);
  static std::vector<std::string> asshuku(int fd);
};

#endif
