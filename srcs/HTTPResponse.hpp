#pragma once
#include <map>
#include <string>

#include <string>
class HTTPResponse {
 protected:
  int _httpStatusCode;
  std::string _httpStatusLine;
  std::string _httpResponseHeader;
  std::string _httpResponseBody;

 public:
  // getter
  int getHttpStatusCode() const { return _httpStatusCode; }
  const std::string& getHttpStatusLine() const { return _httpStatusLine; }
  const std::string& getHttpResponseHeader() const {
    return _httpResponseHeader;
  }
  const std::string& getHttpResponseBody() const { return _httpResponseBody; }

  // setter
  void setHttpStatusCode(int httpStatusCode) {
    _httpStatusCode = httpStatusCode;
  }
  void setHttpStatusLine(std::string httpStatusLine) {
    _httpStatusLine = httpStatusLine;
  }
  void setHttpResponseHeader(std::string httpResponseHeader) {
    _httpResponseHeader = httpResponseHeader;
  }
  void setHttpResponseBody(std::string httpResponseBody) {
    _httpResponseBody = httpResponseBody;
  }

  HTTPResponse() { ; }
  ~HTTPResponse() { ; }
};
