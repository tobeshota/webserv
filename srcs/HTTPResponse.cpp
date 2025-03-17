#include "HTTPResponse.hpp"

HTTPResponse::HTTPResponse() {
  _version = "HTTP/1.1";
  _status_code = 200;
  _status_message = "OK";
}

HTTPResponse::~HTTPResponse() {}

void HTTPResponse::setStatus(int code, const std::string& message) {
  _status_code = code;
  _status_message = message;
}

void HTTPResponse::setHeader(const std::string& key, const std::string& value) {
  _headers[key] = value;
}

void HTTPResponse::setBody(const std::string& body) { _body = body; }

// std::string HTTPResponse::toString() const {
//   std::string response = _version + " " + std::to_string(_status_code) + " "
//   + _status_message + "\n"; for (const auto& header : _headers) {
//     response += header.first + ": " + header.second + "\n";
//   }
//   response += "\n";
//   response += _body;
//   return response;
// }
