#pragma once

#include <map>
#include <string>

class StatusCodes {
 public:
  static std::map<unsigned long, std::string> CreateStatusMessages();
};
