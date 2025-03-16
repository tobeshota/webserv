#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest() : _valid(false), _keepAlive(false) {}

HTTPRequest::HTTPRequest(const std::string& method, const std::string& url,
                         const std::string& version,
                         const std::map<std::string, std::string>& headers,
                         const std::string& body, bool keepAlive)
    : _method(method),
      _url(url),
      _version(version),
      _headers(headers),
      _body(body),
      _valid(true),
      _keepAlive(keepAlive) {}

HTTPRequest::~HTTPRequest() {}

std::string HTTPRequest::getHeader(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = _headers.find(key);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}
