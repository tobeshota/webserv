#pragma once
#include <map>
#include <string>
class HTTPResponse {
<<<<<<< HEAD
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
=======
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
  std::string toString() const;
>>>>>>> 5fdd9d0 (runserver改造中。動く。)
};
