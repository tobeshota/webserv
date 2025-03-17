#pragma once
#include <map>
#include <string>
// HTTPリクエストの構造体が含まれる
class HTTPRequest {
 private:
  std::string _method;
  std::string _url;
  std::string _version;
  std::map<std::string, std::string> _headers;
  std::string _body;
  bool _valid;
  bool _keepAlive;  // Keep-Alive接続かどうかを示すフラグを追加

 public:
  HTTPRequest();
  HTTPRequest(
      const std::string& method, const std::string& url,
      const std::string& version,
      const std::map<std::string, std::string>& headers,
      const std::string& body,
      bool keepAlive = false);  // keepAliveパラメータをデフォルト値付きで追加
  ~HTTPRequest();

  // アクセサメソッド
  std::string getMethod() const { return _method; }
  std::string getURL() const { return _url; }
  std::string getVersion() const { return _version; }
  std::string getBody() const { return _body; }
  const std::map<std::string, std::string>& getHeaders() const {
    return _headers;
  }
  std::string getHeader(const std::string& key) const;
  bool isValid() const { return _valid; }
  bool isKeepAlive() const { return _keepAlive; }  // keepAliveのゲッターを追加
};
