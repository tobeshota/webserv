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
      _keepAlive(keepAlive) {
  // Hostヘッダがある場合は、_server_nameに値を設定
  std::map<std::string, std::string>::const_iterator it = _headers.find("Host");
  if (it != _headers.end()) {
    std::string host = it->second;
    size_t pos = host.find(":");
    if (pos != std::string::npos) {
      _server_name = host.substr(0, pos);
    } else {
      // ここには来ない．
      // ∵"<サーバー名>:<ポート番号>"と":"が絶対につくため
      _server_name = host;
    }
  }
}
HTTPRequest::~HTTPRequest() { ; }

std::string HTTPRequest::getHeader(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = _headers.find(key);
  if (it != _headers.end()) {
    return it->second;
  }
  return "";
}
