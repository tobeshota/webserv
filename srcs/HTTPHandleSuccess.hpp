#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <map>
#include <string>

// HTTPレスポンスの構造体が含まれる
class HTTPResponse {
 private:
  std::string _version;
  int _status_code;
  std::string _status_message;
  std::map<std::string, std::string> _headers;
  std::string _body;

 public:
  HTTPResponse();
  ~HTTPResponse();

  void setStatus(int code, const std::string& message);
  void setHeader(const std::string& key, const std::string& value);
  void setBody(const std::string& body);
  std::string connect_str(FILE* file) const;
  HTTPResponse ceateResponseData(RunServer& run_server, size_t i);
};
