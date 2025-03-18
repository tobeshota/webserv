#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "../../srcs/HTTPRequest.hpp"
#include "../../srcs/HTTPRequestParser.hpp"

class HTTPRequestParserTest : public ::testing::Test {
 protected:
  void SetUp() override { parser = new HTTPRequestParser(); }

  void TearDown() override { delete parser; }

  // HTTPRequestParserの公開APIを使用する
  HTTPRequest parseRequest(const std::string& request) {
    return parser->parseRequest(request);  // parseRequestは文字列の参照を取る
  }

  HTTPRequestParser* parser;
};

TEST_F(HTTPRequestParserTest, BasicGetRequest) {
  std::string request = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(),
            "/index.html");  // Changed from getURI() to getURL()
  EXPECT_EQ(result.getVersion(), "HTTP/1.1");
  EXPECT_EQ(result.getHeaders().size(), 1);
  EXPECT_EQ(result.getHeader("Host"), "localhost");
}

TEST_F(HTTPRequestParserTest, PostRequestWithBody) {
  std::string request =
      "POST /submit-form HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: 27\r\n\r\n"
      "username=john&password=1234";

  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(),
            "/submit-form");  // Changed from getURI() to getURL()
  EXPECT_EQ(result.getBody(), "username=john&password=1234");
  EXPECT_EQ(result.getHeader("Content-Type"),
            "application/x-www-form-urlencoded");
  EXPECT_EQ(result.getHeader("Content-Length"), "27");
}

TEST_F(HTTPRequestParserTest, PutRequest) {
  std::string request =
      "PUT /resource HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 15\r\n\r\n"
      "Updated content";

  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "PUT");
  EXPECT_EQ(result.getURL(), "/resource");  // Changed from getURI() to getURL()
  EXPECT_EQ(result.getBody(), "Updated content");
}

TEST_F(HTTPRequestParserTest, DeleteRequest) {
  std::string request =
      "DELETE /resource/123 HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "DELETE");
  EXPECT_EQ(result.getURL(),
            "/resource/123");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, MultipleHeaders) {
  std::string request =
      "GET /page HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: Mozilla/5.0\r\n"
      "Accept: text/html\r\n"
      "Accept-Language: en-US\r\n"
      "Cookie: sessionid=12345\r\n\r\n";

  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getHeaders().size(), 5);
  EXPECT_EQ(result.getHeader("Host"), "example.com");
  EXPECT_EQ(result.getHeader("User-Agent"), "Mozilla/5.0");
  EXPECT_EQ(result.getHeader("Accept"), "text/html");
  EXPECT_EQ(result.getHeader("Accept-Language"), "en-US");
  EXPECT_EQ(result.getHeader("Cookie"), "sessionid=12345");
}

TEST_F(HTTPRequestParserTest, InvalidRequestLine) {
  std::string request =
      "INVALID /path HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, MalformedRequest) {
  std::string request =
      "GET /path\r\n"
      "Host: example.com\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, EmptyRequest) {
  std::string request = "";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, RequestWithoutHeaders) {
  std::string request = "GET /path HTTP/1.1\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path");  // Changed from getURI() to getURL()
  EXPECT_EQ(result.getHeaders().size(), 0);
}

TEST_F(HTTPRequestParserTest, RequestWithQueryParams) {
  std::string request =
      "GET /search?q=test&page=1 HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getURL(),
            "/search?q=test&page=1");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, RequestWithFragment) {
  std::string request =
      "GET /page#section1 HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getURL(),
            "/page#section1");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, LongUri) {
  std::string longPath(2000, 'a');
  std::string request = "GET /" + longPath +
                        " HTTP/1.1\r\n"
                        "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getURL(),
            "/" + longPath);  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, HeaderContinuationLine) {
  std::string request =
      "GET /path HTTP/1.1\r\n"
      "User-Agent: Mozilla/5.0\r\n"
      " (Windows NT 10.0; Win64; x64)\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("User-Agent"),
            "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
}

// 以下、新規追加するテストケース

TEST_F(HTTPRequestParserTest, IncompleteRequestLine) {
  std::string request = "GET\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, MissingRequestVersion) {
  std::string request = "GET /index.html\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// TEST_F(HTTPRequestParserTest, InvalidHttpVersion) {
//   std::string request = "GET /index.html HTTP/2.0\r\nHost:
//   example.com\r\n\r\n"; EXPECT_THROW(parseRequest(request),
//   std::runtime_error);
// }

TEST_F(HTTPRequestParserTest, MalformedHeaderLine) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "InvalidHeader\r\n"
      "Host: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, HeaderWithEmptyValue) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "EmptyHeader: \r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("EmptyHeader"), "");
}

TEST_F(HTTPRequestParserTest, HeaderWithMultipleColons) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Complex-Header: value:with:colons\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("Complex-Header"), "value:with:colons");
}

// チャンク転送エンコーディングのテスト
TEST_F(HTTPRequestParserTest, ChunkedTransferEncoding) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\n"
      "Wiki\r\n"
      "5\r\n"
      "pedia\r\n"
      "E\r\n"
      " in\r\n\r\nchunks.\r\n"
      "0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(), "/upload");
  EXPECT_EQ(result.getBody(), "Wikipedia in\r\n\r\nchunks.");
  EXPECT_EQ(result.getHeader("Transfer-Encoding"), "chunked");
}

// HTTP/1.0リクエストのテスト
TEST_F(HTTPRequestParserTest, HTTP10Request) {
  std::string request = "GET /old-page.html HTTP/1.0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/old-page.html");
  EXPECT_EQ(result.getVersion(), "HTTP/1.0");
  EXPECT_FALSE(
      result.isKeepAlive());  // HTTP/1.0はデフォルトでkeep-aliveではない
}

// Keep-Aliveヘッダーを持つHTTP/1.0リクエストのテスト
TEST_F(HTTPRequestParserTest, HTTP10KeepAliveRequest) {
  std::string request =
      "GET /page.html HTTP/1.0\r\n"
      "Connection: keep-alive\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getVersion(), "HTTP/1.0");
  EXPECT_TRUE(result.isKeepAlive());  // Connection: keep-aliveが指定されている
}

// HTTP/1.1リクエストでのKeep-Alive動作のテスト
TEST_F(HTTPRequestParserTest, HTTP11DefaultKeepAlive) {
  std::string request =
      "GET /page.html HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getVersion(), "HTTP/1.1");
  EXPECT_TRUE(result.isKeepAlive());  // HTTP/1.1はデフォルトでkeep-alive
}

// HTTP/1.1でKeep-Aliveを明示的に閉じるテスト
TEST_F(HTTPRequestParserTest, HTTP11ExplicitClose) {
  std::string request =
      "GET /page.html HTTP/1.1\r\n"
      "Connection: close\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getVersion(), "HTTP/1.1");
  EXPECT_FALSE(result.isKeepAlive());  // Connection: closeが指定されている
}

// // 大文字小文字混在ヘッダーのテスト
// TEST_F(HTTPRequestParserTest, CaseInsensitiveHeaders) {
//   std::string request =
//       "GET /index.html HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "User-AGENT: Mozilla/5.0\r\n"
//       "ACCEPT: text/html\r\n"
//       "accept-Language: en-US\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getHeader("User-AGENT"), "Mozilla/5.0");
//   EXPECT_EQ(result.getHeader("ACCEPT"), "text/html");
//   EXPECT_EQ(result.getHeader("accept-Language"), "en-US");

//   // 大文字小文字を無視して取得できることを確認
//   EXPECT_EQ(result.getHeader("user-agent"), "Mozilla/5.0");
//   EXPECT_EQ(result.getHeader("accept"), "text/html");
//   EXPECT_EQ(result.getHeader("Accept-language"), "en-US");
// }

// 複数のボディパートを持つリクエストのテスト
TEST_F(HTTPRequestParserTest, MultipartFormData) {
  std::string boundary = "----WebKitFormBoundaryX3xWAR56B6AIbH5s";
  std::string body = "--" + boundary +
                     "\r\n"
                     "Content-Disposition: form-data; name=\"field1\"\r\n\r\n"
                     "value1\r\n"
                     "--" +
                     boundary +
                     "\r\n"
                     "Content-Disposition: form-data; name=\"field2\"\r\n\r\n"
                     "value2\r\n"
                     "--" +
                     boundary + "--\r\n";

  std::string request =
      "POST /form HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: multipart/form-data; boundary=" +
      boundary +
      "\r\n"
      "Content-Length: " +
      std::to_string(body.length()) + "\r\n\r\n" + body;

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(), "/form");
  EXPECT_EQ(result.getHeader("Content-Type"),
            "multipart/form-data; boundary=" + boundary);
  EXPECT_EQ(result.getBody(), body);
}

// URLエンコードされたフォームデータのテスト
TEST_F(HTTPRequestParserTest, UrlEncodedFormData) {
  std::string body =
      "field1=value%201&field2=value%202&special=%21%40%23%24%25";
  std::string request =
      "POST /form HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: application/x-www-form-urlencoded\r\n"
      "Content-Length: " +
      std::to_string(body.length()) + "\r\n\r\n" + body;

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(), "/form");
  EXPECT_EQ(result.getBody(), body);
}

// 特殊文字を含むURLのテスト
TEST_F(HTTPRequestParserTest, URLWithSpecialCharacters) {
  std::string request =
      "GET /path/with/special/%20%21%40%23%24%25%5E%26%2A%28%29 HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(),
            "/path/with/special/%20%21%40%23%24%25%5E%26%2A%28%29");
}

// 非常に長いヘッダーのテスト
TEST_F(HTTPRequestParserTest, VeryLongHeader) {
  std::string longValue(4096, 'x');  // 4096文字の長さの値
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "X-Long-Header: " +
      longValue + "\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getHeader("X-Long-Header"), longValue);
}

// 複数行にまたがるヘッダー値のテスト（RFC 2616に基づく古い形式）
TEST_F(HTTPRequestParserTest, MultilineHeaderValue) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "X-Multiline-Header: value1\r\n"
      " value2\r\n"
      "\tvalue3\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getHeader("X-Multiline-Header"), "value1 value2 value3");
}

// 空のリクエストボディのテスト
TEST_F(HTTPRequestParserTest, EmptyRequestBody) {
  std::string request =
      "POST /form HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(), "/form");
  EXPECT_EQ(result.getBody(), "");
}

// 非標準的なHTTPメソッドのテスト
TEST_F(HTTPRequestParserTest, CustomHttpMethod) {
  std::string request =
      "CUSTOM /resource HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// ホストヘッダーのないHTTP/1.1リクエストのテスト（RFC 7230に基づくとエラー）
TEST_F(HTTPRequestParserTest, MissingHostHeaderInHttp11) {
  std::string request = "GET /index.html HTTP/1.1\r\n\r\n";

  // 現在の実装ではホストヘッダーのチェックを行っていないが、
  // 将来的には実装するかもしれないので、このテストを追加
  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_TRUE(result.getHeader("Host").empty());
}