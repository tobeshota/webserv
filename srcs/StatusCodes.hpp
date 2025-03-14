#pragma once

#include <map>
#include <string>

class StatusCodes {
 private:
  std::map<unsigned long, std::string> CreateStatusMessages();

 public:
  std::map<unsigned long, std::string> status_messages;
  StatusCodes();
  bool isValid(unsigned long code);
  bool isSuccess(unsigned long code);
  bool isError(unsigned long code);
  std::string getMessage(unsigned long code);
};

