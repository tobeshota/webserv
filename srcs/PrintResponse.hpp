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
 public:
  PrintResponse();
  ~PrintResponse();
  void print_response();
};
