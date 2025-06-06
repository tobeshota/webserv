#include "StatusCodes.hpp"

#include <iostream>
#include <map>
#include <string>

std::map<unsigned long, std::string> StatusCodes::CreateStatusMessages() {
  std::map<unsigned long, std::string> status_messages;
  status_messages[100] = "Continue";
  status_messages[101] = "Switching Protocols";
  status_messages[102] = "Processing";
  status_messages[103] = "Early Hints";
  status_messages[200] = "OK";
  status_messages[201] = "Created";
  status_messages[202] = "Accepted";
  status_messages[203] = "Non-Authoritative Information";
  status_messages[204] = "No Content";
  status_messages[205] = "Reset Content";
  status_messages[206] = "Partial Content";
  status_messages[207] = "Multi-Status";
  status_messages[208] = "Already Reported";
  status_messages[226] = "IM Used";
  status_messages[300] = "Multiple Choices";
  status_messages[301] = "Moved Permanently";
  status_messages[302] = "Found";
  status_messages[303] = "See Other";
  status_messages[304] = "Not Modified";
  status_messages[305] = "Use Proxy";
  status_messages[306] = "(Unused)";
  status_messages[307] = "Temporary Redirect";
  status_messages[308] = "Permanent Redirect";
  status_messages[400] = "Bad Request";
  status_messages[401] = "Unauthorized";
  status_messages[402] = "Payment Required";
  status_messages[403] = "Forbidden";
  status_messages[404] = "Not Found";
  status_messages[405] = "Method Not Allowed";
  status_messages[406] = "Not Acceptable";
  status_messages[407] = "Proxy Authentication Required";
  status_messages[408] = "Request Timeout";
  status_messages[409] = "Conflict";
  status_messages[410] = "Gone";
  status_messages[411] = "Length Required";
  status_messages[412] = "Precondition Failed";
  status_messages[413] = "Content Too Large";
  status_messages[414] = "URI Too Long";
  status_messages[415] = "Unsupported Media Type";
  status_messages[416] = "Range Not Satisfiable";
  status_messages[417] = "Expectation Failed";
  status_messages[418] = "(Unused)";
  status_messages[421] = "Misdirected Request";
  status_messages[422] = "Unprocessable Content";
  status_messages[423] = "Locked";
  status_messages[424] = "Failed Dependency";
  status_messages[425] = "Too Early";
  status_messages[426] = "Upgrade Required";
  status_messages[428] = "Precondition Required";
  status_messages[429] = "Too Many Requests";
  status_messages[431] = "Request Header Fields Too Large";
  status_messages[451] = "Unavailable For Legal Reasons";
  status_messages[500] = "Internal Server Error";
  status_messages[501] = "Not Implemented";
  status_messages[502] = "Bad Gateway";
  status_messages[503] = "Service Unavailable";
  status_messages[504] = "Gateway Timeout";
  status_messages[505] = "HTTP Version Not Supported";
  status_messages[506] = "Variant Also Negotiates";
  status_messages[507] = "Insufficient Storage";
  status_messages[508] = "Loop Detected";
  status_messages[510] = "Not Extended";
  status_messages[511] = "Network Authentication Required";

  return status_messages;
}

StatusCodes::StatusCodes() { status_messages = CreateStatusMessages(); }

std::string StatusCodes::getMessage(unsigned long code) {
  std::map<unsigned long, std::string> status_messages = CreateStatusMessages();
  return status_messages[code];
}

bool StatusCodes::isError(unsigned long code) {
  std::map<unsigned long, std::string> status_messages = CreateStatusMessages();
  if (code >= 400) {
    return true;
  } else {
    return false;
  }
}

bool StatusCodes::isSuccess(unsigned long code) {
  std::map<unsigned long, std::string> status_messages = CreateStatusMessages();
  if (code < 400) {
    return true;
  } else {
    return false;
  }
}

bool StatusCodes::isValid(unsigned long code) {
  std::map<unsigned long, std::string> status_messages = CreateStatusMessages();
  if (status_messages.find(code) != status_messages.end()) {
    return true;
  } else {
    return false;
  }
}
