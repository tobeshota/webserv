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
  EXPECT_EQ(result.getServerName(), "localhost");
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
  EXPECT_EQ(result.getServerName(), "example.com");
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
  EXPECT_TRUE(result.getServerName().empty());
}

// パーサーのリセット機能テスト
TEST_F(HTTPRequestParserTest, ParserReset) {
  std::string request1 =
      "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  HTTPRequest result1 = parseRequest(request1);

  EXPECT_EQ(result1.getMethod(), "GET");
  EXPECT_EQ(result1.getURL(), "/index.html");

  // 異なるリクエストで再度テスト
  std::string request2 = "POST /submit HTTP/1.1\r\nHost: example.com\r\n\r\n";
  HTTPRequest result2 = parseRequest(request2);

  EXPECT_EQ(result2.getMethod(), "POST");
  EXPECT_EQ(result2.getURL(), "/submit");
}

// HEADメソッドテスト
TEST_F(HTTPRequestParserTest, HeadMethod) {
  std::string request =
      "HEAD /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "HEAD");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_TRUE(result.getBody().empty());
}

// OPTIONSメソッドテスト
TEST_F(HTTPRequestParserTest, OptionsMethod) {
  std::string request = "OPTIONS * HTTP/1.1\r\nHost: example.com\r\n\r\n";
  HTTPRequest result = parseRequest(request);

  EXPECT_EQ(result.getMethod(), "OPTIONS");
  EXPECT_EQ(result.getURL(), "*");
}

// 複雑なチャンク転送エンコーディングテスト
TEST_F(HTTPRequestParserTest, ComplexChunkedEncoding) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "A\r\n"  // 10バイト
      "1234567890\r\n"
      "5\r\n"  // 5バイト
      "12345\r\n"
      "0\r\n"
      "Trailer: value\r\n"
      "\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getURL(), "/upload");
  EXPECT_EQ(result.getBody(), "123456789012345");
}

// // チャンク拡張のテスト
// TEST_F(HTTPRequestParserTest, ChunkExtensions) {
//   std::string request =
//       "POST /upload HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Transfer-Encoding: chunked\r\n\r\n"
//       "A;name=value\r\n"  // チャンク拡張付き
//       "1234567890\r\n"
//       "0\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "POST");
//   EXPECT_EQ(result.getBody(), "1234567890");
// }

// より複雑なリクエストURLのテスト
TEST_F(HTTPRequestParserTest, ComplexRequestURL) {
  std::string request =
      "GET "
      "/path/to/resource/with/multiple/"
      "segments?param1=value1&param2=value2#fragment HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(),
            "/path/to/resource/with/multiple/"
            "segments?param1=value1&param2=value2#fragment");
}

// // ホスト名にポート番号を含むリクエストのテスト
// TEST_F(HTTPRequestParserTest, HostWithPort) {
//   std::string request =
//       "GET /index.html HTTP/1.1\r\n"
//       "Host: example.com:8080\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "GET");
//   EXPECT_EQ(result.getURL(), "/index.html");
//   EXPECT_EQ(result.getServerName(), "example.com:8080");
// }

// // 複数のContent-Lengthヘッダーを持つ不正なリクエストのテスト
// TEST_F(HTTPRequestParserTest, MultipleContentLengthHeaders) {
//   std::string request =
//       "POST /submit HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Content-Length: 10\r\n"
//       "Content-Length: 20\r\n\r\n"
//       "1234567890";

//   // 現在の実装では最後の値が使用される
//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "POST");
//   EXPECT_EQ(result.getBody(), "1234567890");
// }

// より大きなリクエストボディのテスト
TEST_F(HTTPRequestParserTest, LargeRequestBody) {
  std::string largeBody(8192, 'x');  // 8KBのボディ
  std::string request =
      "POST /large-data HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 8192\r\n\r\n" +
      largeBody;

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), largeBody);
  EXPECT_EQ(result.getBody().length(), 8192);
}

// Content-Lengthが0のPOSTリクエストのテスト
TEST_F(HTTPRequestParserTest, ZeroLengthBody) {
  std::string request =
      "POST /empty HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_TRUE(result.getBody().empty());
}

// 不正なContent-Length値のテスト
TEST_F(HTTPRequestParserTest, InvalidContentLength) {
  std::string request =
      "POST /data HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: invalid\r\n\r\n"
      "Some data";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// リクエストラインの最大長テスト
TEST_F(HTTPRequestParserTest, VeryLongRequestLine) {
  std::string veryLongPath(8000, 'a');
  std::string request =
      "GET /" + veryLongPath + " HTTP/1.1\r\nHost: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/" + veryLongPath);
}

// 追加のHTTPメソッドテスト - TRACE
TEST_F(HTTPRequestParserTest, TraceMethod) {
  std::string request = "TRACE /path HTTP/1.1\r\nHost: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "TRACE");
  EXPECT_EQ(result.getURL(), "/path");
}

// 追加のHTTPメソッドテスト - CONNECT
TEST_F(HTTPRequestParserTest, ConnectMethod) {
  std::string request =
      "CONNECT example.com:443 HTTP/1.1\r\nHost: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "CONNECT");
  EXPECT_EQ(result.getURL(), "example.com:443");
}

// PATCHメソッドテスト
// TEST_F(HTTPRequestParserTest, PatchMethod) {
//   std::string request =
//       "PATCH /resource HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Content-Length: 18\r\n\r\n"
//       "patch update data";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "PATCH");
//   EXPECT_EQ(result.getURL(), "/resource");
//   EXPECT_EQ(result.getBody(), "patch update data");
// }

// 空パスのリクエストテスト
TEST_F(HTTPRequestParserTest, EmptyPath) {
  std::string request = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/");
}

// 部分的なデータフィードのテスト
TEST_F(HTTPRequestParserTest, PartialFeed) {
  // テスト実装 - 低レベルAPIを使用
  HTTPRequestParser localParser;
  std::string request1 = "GET /index.html HTTP/1.1\r\n";
  std::string request2 = "Host: example.com\r\n\r\n";

  EXPECT_FALSE(localParser.feed(request1.c_str(), request1.length()));
  EXPECT_FALSE(localParser.isComplete());

  EXPECT_TRUE(localParser.feed(request2.c_str(), request2.length()));
  EXPECT_TRUE(localParser.isComplete());

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getServerName(), "example.com");
}

// 大きなチャンクのテスト
// TEST_F(HTTPRequestParserTest, LargeChunks) {
//   std::string chunk1(4000, 'A');
//   std::string chunk2(2000, 'B');

//   std::string request =
//       "POST /upload HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Transfer-Encoding: chunked\r\n\r\n"
//       + std::to_string(chunk1.length()) + "\r\n"
//       + chunk1 + "\r\n"
//       + std::to_string(chunk2.length()) + "\r\n"
//       + chunk2 + "\r\n"
//       "0\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "POST");
//   EXPECT_EQ(result.getBody().length(), 6000);
//   EXPECT_EQ(result.getBody().substr(0, 10), "AAAAAAAAAA");
//   EXPECT_EQ(result.getBody().substr(4000, 10), "BBBBBBBBBB");
// }

// 16進数のチャンクサイズテスト
TEST_F(HTTPRequestParserTest, HexChunkSize) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "A\r\n"  // 16進数の10バイト
      "0123456789\r\n"
      "10\r\n"  // 16進数の16バイト
      "0123456789ABCDEF\r\n"
      "0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "01234567890123456789ABCDEF");
}

// 大文字小文字混在のヘッダー名テスト
TEST_F(HTTPRequestParserTest, MixedCaseHeaderNames) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-TYPE: text/html\r\n"
      "User-AGENT: TestClient\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("Content-TYPE"), "text/html");
  EXPECT_EQ(result.getHeader("User-AGENT"), "TestClient");
}

// エラー状態の詳細テスト - 無効なHTTPバージョン形式
TEST_F(HTTPRequestParserTest, InvalidHttpVersionFormat) {
  std::string request =
      "GET /index.html HTTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 空のホストヘッダーテスト
TEST_F(HTTPRequestParserTest, EmptyHostHeader) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: \r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_TRUE(result.getServerName().empty());
}

// 特殊な形式のURLテスト
TEST_F(HTTPRequestParserTest, SpecialFormattedURL) {
  std::string request =
      "GET /path//with///multiple////slashes HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path//with///multiple////slashes");
}

// 複数のクエリパラメータを持つURLテスト
TEST_F(HTTPRequestParserTest, MultipleQueryParams) {
  std::string request =
      "GET /search?q=test&page=1&sort=desc&filter=active HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/search?q=test&page=1&sort=desc&filter=active");
}

// 空のクエリパラメータを持つURLテスト
TEST_F(HTTPRequestParserTest, EmptyQueryParams) {
  std::string request =
      "GET /search?q=&page=&sort= HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/search?q=&page=&sort=");
}

// 複数のフラグメントを持つURLテスト
TEST_F(HTTPRequestParserTest, URLWithComplexFragment) {
  std::string request =
      "GET /page#section1/subsection#item HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/page#section1/subsection#item");
}

// エンコードされたスペースを含むURLテスト
TEST_F(HTTPRequestParserTest, URLWithEncodedSpaces) {
  std::string request =
      "GET /path/with%20encoded%20spaces HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path/with%20encoded%20spaces");
}

// 異常に長いヘッダー名テスト
TEST_F(HTTPRequestParserTest, VeryLongHeaderName) {
  std::string longHeaderName(200, 'X');
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n" +
      longHeaderName + ": value\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getHeader(longHeaderName), "value");
}

// リクエストヘッダー継続のエッジケーステスト
// TEST_F(HTTPRequestParserTest, HeaderContinuationEdgeCases) {
//   std::string request =
//       "GET /index.html HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "X-Multiline:    value1\r\n"
//       "  \t  value2  \r\n"
//       " value3\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getHeader("X-Multiline"), "value1 value2 value3");
// }

// テストパーサーリセット後の動作
TEST_F(HTTPRequestParserTest, ResetBetweenIncompleteRequests) {
  HTTPRequestParser localParser;

  // 不完全なリクエスト
  std::string request1 = "GET /page1 HTTP/1.1\r\n";
  EXPECT_FALSE(localParser.feed(request1.c_str(), request1.length()));

  // リセット
  localParser.reset();

  // 新しい完全なリクエスト
  std::string request2 = "GET /page2 HTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_TRUE(localParser.feed(request2.c_str(), request2.length()));

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/page2");
}

// リクエストパーサーのエラーメッセージテスト
// TEST_F(HTTPRequestParserTest, ErrorMessageTest) {
//   HTTPRequestParser localParser;

//   // 不正なリクエスト
//   std::string badRequest = "INVALID!!! REQUEST!!!";
//   EXPECT_FALSE(localParser.feed(badRequest.c_str(), badRequest.length()));
//   EXPECT_TRUE(localParser.hasError());
//   EXPECT_FALSE(localParser.getErrorMessage().empty());
// }

// エラー回復テスト
// TEST_F(HTTPRequestParserTest, ErrorRecoveryAfterReset) {
//   HTTPRequestParser localParser;

//   // 不正なリクエスト
//   std::string badRequest = "BAD /invalid HTTP/1.1\r\n\r\n";
//   EXPECT_FALSE(localParser.feed(badRequest.c_str(), badRequest.length()));
//   EXPECT_TRUE(localParser.hasError());

//   // リセット
//   localParser.reset();
//   EXPECT_FALSE(localParser.hasError());

//   // 有効なリクエスト
//   std::string goodRequest = "GET /valid HTTP/1.1\r\nHost:
//   example.com\r\n\r\n"; EXPECT_TRUE(localParser.feed(goodRequest.c_str(),
//   goodRequest.length())); EXPECT_FALSE(localParser.hasError());

//   HTTPRequest result = localParser.createRequest();
//   EXPECT_EQ(result.getMethod(), "GET");
//   EXPECT_EQ(result.getURL(), "/valid");
// }

// 特殊なヘッダーフィールドテスト
TEST_F(HTTPRequestParserTest, SpecialHeaderFields) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "If-Modified-Since: Sat, 29 Oct 2023 19:43:31 GMT\r\n"
      "If-None-Match: \"686897696a7c876b7e\"\r\n"
      "X-Custom-Header: Test/1.0\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getHeader("If-Modified-Since"),
            "Sat, 29 Oct 2023 19:43:31 GMT");
  EXPECT_EQ(result.getHeader("If-None-Match"), "\"686897696a7c876b7e\"");
  EXPECT_EQ(result.getHeader("X-Custom-Header"), "Test/1.0");
}

// フィードAPIの段階的テスト
TEST_F(HTTPRequestParserTest, IncrementalFeedTest) {
  HTTPRequestParser localParser;

  // リクエストを小さな部分に分割
  std::string part1 = "GET /ind";
  std::string part2 = "ex.html HTT";
  std::string part3 = "P/1.1\r\nHost: ";
  std::string part4 = "example.com\r\n\r\n";

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_FALSE(localParser.isComplete());

  EXPECT_FALSE(localParser.feed(part2.c_str(), part2.length()));
  EXPECT_FALSE(localParser.isComplete());

  EXPECT_FALSE(localParser.feed(part3.c_str(), part3.length()));
  EXPECT_FALSE(localParser.isComplete());

  EXPECT_TRUE(localParser.feed(part4.c_str(), part4.length()));
  EXPECT_TRUE(localParser.isComplete());

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getServerName(), "example.com");
}

// チャンクエンコーディングの複雑なシナリオ
TEST_F(HTTPRequestParserTest, ComplexChunkedScenario) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "3\r\n"  // 3バイトチャンク
      "abc\r\n"
      "0\r\n"                         // 終了チャンク
      "X-Trailer: trailer-value\r\n"  // トレーラーヘッダー
      "\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "abc");
}

// ドットセグメントを含むパスのテスト
TEST_F(HTTPRequestParserTest, PathWithDotSegments) {
  std::string request =
      "GET /path/./to/../resource HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path/./to/../resource");
}

// 無効なリクエストメソッド開始文字のテスト
TEST_F(HTTPRequestParserTest, InvalidRequestMethodStartChar) {
  std::string request =
      "!GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// リクエストメソッド中の無効文字テスト
TEST_F(HTTPRequestParserTest, InvalidCharInMethod) {
  std::string request =
      "GE\tT /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なURI開始文字テスト
TEST_F(HTTPRequestParserTest, InvalidURIStartChar) {
  std::string request =
      "GET \x01index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// URIに制御文字を含むテスト
TEST_F(HTTPRequestParserTest, ControlCharInURI) {
  std::string request =
      "GET /index\x00.html HTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// HTTPバージョン解析の各ステップのエラーテスト
TEST_F(HTTPRequestParserTest, InvalidHttpVersionH) {
  std::string request = "GET /index.html XTTP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, InvalidHttpVersionHT) {
  std::string request = "GET /index.html HTXP/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, InvalidHttpVersionHTT) {
  std::string request = "GET /index.html HTTX/1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, InvalidHttpVersionHTTP) {
  std::string request = "GET /index.html HTTPX1.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なHTTPバージョンメジャー開始のテスト
TEST_F(HTTPRequestParserTest, InvalidHttpVersionMajorStart) {
  std::string request = "GET /index.html HTTP/A.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なHTTPバージョンメジャーの途中のテスト
TEST_F(HTTPRequestParserTest, InvalidHttpVersionMajor) {
  std::string request =
      "GET /index.html HTTP/1A.1\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なHTTPバージョンマイナー開始のテスト
TEST_F(HTTPRequestParserTest, InvalidHttpVersionMinorStart) {
  std::string request = "GET /index.html HTTP/1.A\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なHTTPバージョンマイナーの途中のテスト
TEST_F(HTTPRequestParserTest, InvalidHttpVersionMinor) {
  std::string request =
      "GET /index.html HTTP/1.1A\r\nHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 改行文字がない場合のテスト
TEST_F(HTTPRequestParserTest, MissingNewline1) {
  std::string request = "GET /index.html HTTP/1.1\rHost: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// ヘッダーラインの開始での無効文字テスト
TEST_F(HTTPRequestParserTest, InvalidHeaderNameStart) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n:Invalid: value\r\nHost: "
      "example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// ヘッダー名の無効文字テスト
TEST_F(HTTPRequestParserTest, InvalidCharInHeaderName) {
  std::string request =
      "GET /index.html HTTP/1.1\r\nHo\x01st: example.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// ヘッダー値の無効文字テスト
TEST_F(HTTPRequestParserTest, InvalidCharInHeaderValue) {
  std::string request =
      "GET /index.html HTTP/1.1\r\nHost: example\x00.com\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// ヘッダー値前のスペース後の改行テスト
TEST_F(HTTPRequestParserTest, NewlineAfterSpaceBeforeHeaderValue) {
  std::string request =
      "GET /index.html HTTP/1.1\r\nHeader: \r\nHost: example.com\r\n\r\n";
  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("Header"), "");
}

// 2回目の改行文字がない場合のテスト
TEST_F(HTTPRequestParserTest, MissingNewline2) {
  std::string request =
      "GET /index.html HTTP/1.1\r\nHost: example.com\rContent-Type: "
      "text/html\r\n\r\n";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 3回目の改行文字がない場合のテスト
TEST_F(HTTPRequestParserTest, MissingNewline3) {
  std::string request = "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r";
  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクサイズが空のテスト
TEST_F(HTTPRequestParserTest, EmptyChunkSize) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "\r\n"  // 空のチャンクサイズ
      "data\r\n"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// 無効なチャンクサイズ文字テスト
TEST_F(HTTPRequestParserTest, InvalidChunkSizeChar) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "XYZ\r\n"  // 16進数以外
      "data\r\n"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクサイズ後のCRがない場合のテスト
TEST_F(HTTPRequestParserTest, MissingCRAfterChunkSize) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "3\ndata\r\n"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクサイズ後のLFがない場合のテスト
TEST_F(HTTPRequestParserTest, MissingLFAfterChunkSize) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "3\rdata\r\n"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクデータ後のCRがない場合のテスト
TEST_F(HTTPRequestParserTest, MissingCRAfterChunkData) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\ndata\n"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクデータ後のLFがない場合のテスト
TEST_F(HTTPRequestParserTest, MissingLFAfterChunkData) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\ndata\r"
      "0\r\n\r\n";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンクトレーラー後の改行文字がない場合のテスト
TEST_F(HTTPRequestParserTest, MissingNewlineAfterChunkTrailer) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\ndata\r\n"
      "0\r\n"
      "X-Trailer: value\r";

  EXPECT_THROW(parseRequest(request), std::runtime_error);
}

// チャンク拡張テスト
// TEST_F(HTTPRequestParserTest, ChunkExtensionTest) {
//   std::string request =
//       "POST /upload HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Transfer-Encoding: chunked\r\n\r\n"
//       "4;name=value\r\n"  // チャンク拡張あり
//       "data\r\n"
//       "0\r\n\r\n";

//   HTTPRequest result = parseRequest(request);
//   EXPECT_EQ(result.getMethod(), "POST");
//   EXPECT_EQ(result.getBody(), "data");
// }

// 複数バイトのチャンクトレーラーテスト
TEST_F(HTTPRequestParserTest, MultilineChunkTrailer) {
  std::string request =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\ndata\r\n"
      "0\r\n"
      "X-Trailer1: value1\r\n"
      "X-Trailer2: value2\r\n"
      "\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "data");
}

// HTTPバージョンに複数桁の数字を含むケース
TEST_F(HTTPRequestParserTest, MultiDigitHttpVersion) {
  std::string request =
      "GET /index.html HTTP/11.22\r\nHost: example.com\r\n\r\n";
  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getVersion(), "HTTP/11.22");
}

// 特殊制御文字が連続するヘッダーテスト
TEST_F(HTTPRequestParserTest, HeaderWithConsecutiveControls) {
  std::string request =
      "GET /index.html HTTP/1.1\r\n"
      "X-Header: value\r\n"
      " \t \t value2\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getHeader("X-Header"), "value value2");
}

// パスにドット以外の特殊文字を含むURLテスト
TEST_F(HTTPRequestParserTest, URLWithSpecialPathChars) {
  std::string request =
      "GET /path/with~special$chars+and-more HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path/with~special$chars+and-more");
}

// 絵文字を含むURLテスト
TEST_F(HTTPRequestParserTest, URLWithEmoji) {
  std::string request =
      "GET /path/with/%F0%9F%98%8A/emoji HTTP/1.1\r\n"
      "Host: example.com\r\n\r\n";

  HTTPRequest result = parseRequest(request);
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/path/with/%F0%9F%98%8A/emoji");
}

// フィードで解析できる最小限のリクエスト
TEST_F(HTTPRequestParserTest, MinimalValidRequest) {
  HTTPRequestParser localParser;
  std::string minimalRequest = "GET / HTTP/1.0\r\n\r\n";

  EXPECT_TRUE(
      localParser.feed(minimalRequest.c_str(), minimalRequest.length()));
  EXPECT_TRUE(localParser.isComplete());

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/");
  EXPECT_EQ(result.getVersion(), "HTTP/1.0");
}

// バイト単位で送信する詳細テスト
TEST_F(HTTPRequestParserTest, ByteByByteFeed) {
  HTTPRequestParser localParser;
  std::string fullRequest =
      "GET /index.html HTTP/1.1\r\nHost: example.com\r\n\r\n";

  for (size_t i = 0; i < fullRequest.length() - 1; ++i) {
    EXPECT_FALSE(localParser.feed(&fullRequest[i], 1));
  }

  // 最後のバイト
  EXPECT_TRUE(localParser.feed(&fullRequest[fullRequest.length() - 1], 1));
  EXPECT_TRUE(localParser.isComplete());

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/index.html");
  EXPECT_EQ(result.getServerName(), "example.com");
}

// 部分的チャンクを扱うテスト
TEST_F(HTTPRequestParserTest, PartialChunkedContent) {
  HTTPRequestParser localParser;

  std::string part1 =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "4\r\n";

  std::string part2 =
      "data\r\n"
      "0\r\n\r\n";

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_TRUE(localParser.feed(part2.c_str(), part2.length()));

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "data");
}

// Content-Length付きの部分的なボディ送信テスト
TEST_F(HTTPRequestParserTest, PartialBodyWithContentLength) {
  HTTPRequestParser localParser;

  std::string part1 =
      "POST /submit HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Content-Length: 10\r\n\r\n"
      "12345";

  std::string part2 = "67890";

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_TRUE(localParser.feed(part2.c_str(), part2.length()));

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "1234567890");
}

// 不完全なチャンクを扱うテスト
TEST_F(HTTPRequestParserTest, IncompleteChunk) {
  HTTPRequestParser localParser;

  std::string part1 =
      "POST /upload HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "Transfer-Encoding: chunked\r\n\r\n"
      "A\r\n"   // 10バイトのチャンク
      "12345";  // 5バイトのみ

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_FALSE(localParser.isComplete());

  std::string part2 =
      "67890\r\n"   // 残りの5バイトとチャンク終端
      "0\r\n\r\n";  // 終端チャンク

  EXPECT_TRUE(localParser.feed(part2.c_str(), part2.length()));
  EXPECT_TRUE(localParser.isComplete());

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "POST");
  EXPECT_EQ(result.getBody(), "1234567890");
}

// 特に多いリクエストメソッドテスト
TEST_F(HTTPRequestParserTest, LongRequestMethod) {
  // RFC7231で定義されていないメソッドは実装によって拒否されるかもしれないことに注意
  EXPECT_THROW(parseRequest("AVERYLONGMETHODNAME /index.html HTTP/1.1\r\nHost: "
                            "example.com\r\n\r\n"),
               std::runtime_error);
}

// バッファ境界でのボディ解析テスト
// TEST_F(HTTPRequestParserTest, BodyParsingAtBufferBoundaries) {
//   HTTPRequestParser localParser;

//   std::string headers =
//       "POST /submit HTTP/1.1\r\n"
//       "Host: example.com\r\n"
//       "Content-Length: 20\r\n\r\n";

//   std::string body1 = "12345678901";  // 11バイト
//   std::string body2 = "23456789";     // 9バイト - 合計20バイト

//   EXPECT_FALSE(localParser.feed(headers.c_str(), headers.length()));
//   EXPECT_FALSE(localParser.feed(body1.c_str(), body1.length()));
//   EXPECT_TRUE(localParser.feed(body2.c_str(), body2.length()));

//   HTTPRequest result = localParser.createRequest();
//   EXPECT_EQ(result.getMethod(), "POST");
//   EXPECT_EQ(result.getBody().length(), 20);
//   EXPECT_EQ(result.getBody(), "1234567890123456789");
// }

// 2バイトでの正確なURIバウンダリテスト
TEST_F(HTTPRequestParserTest, TwoByteURIBoundary) {
  HTTPRequestParser localParser;

  std::string part1 = "GET /";
  std::string part2 = " HTTP/1.1\r\nHost: example.com\r\n\r\n";

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_TRUE(localParser.feed(part2.c_str(), part2.length()));

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getMethod(), "GET");
  EXPECT_EQ(result.getURL(), "/");
}

// 状態の切り替わりポイントのテスト - ヘッダー名と値の境界
TEST_F(HTTPRequestParserTest, HeaderNameValueBoundary) {
  HTTPRequestParser localParser;

  std::string part1 = "GET / HTTP/1.1\r\nTest-Header";
  std::string part2 = ": value\r\n\r\n";

  EXPECT_FALSE(localParser.feed(part1.c_str(), part1.length()));
  EXPECT_TRUE(localParser.feed(part2.c_str(), part2.length()));

  HTTPRequest result = localParser.createRequest();
  EXPECT_EQ(result.getHeader("Test-Header"), "value");
}

// isComplete関数のエッジケースのテスト
TEST_F(HTTPRequestParserTest, IsCompleteEdgeCases) {
  HTTPRequestParser localParser;

  // まだ何もフィードしていない
  EXPECT_FALSE(localParser.isComplete());

  // 不完全なリクエスト
  std::string partialRequest = "GET / HTTP/1.1\r\n";
  EXPECT_FALSE(
      localParser.feed(partialRequest.c_str(), partialRequest.length()));
  EXPECT_FALSE(localParser.isComplete());

  // 完全なリクエスト
  std::string remainder = "Host: example.com\r\n\r\n";
  EXPECT_TRUE(localParser.feed(remainder.c_str(), remainder.length()));
  EXPECT_TRUE(localParser.isComplete());
}

// createRequest関数のエッジケースのテスト
TEST_F(HTTPRequestParserTest, CreateRequestEdgeCases) {
  HTTPRequestParser localParser;

  // 不完全なリクエストから作成を試みる
  std::string partialRequest = "GET / HTTP/1.1\r\n";
  EXPECT_FALSE(
      localParser.feed(partialRequest.c_str(), partialRequest.length()));

  // 不完全なリクエストからはデフォルトリクエストが返る
  HTTPRequest incompleteResult = localParser.createRequest();
  EXPECT_TRUE(incompleteResult.getMethod().empty());

  // 完全なリクエストを提供
  std::string remainder = "Host: example.com\r\n\r\n";
  EXPECT_TRUE(localParser.feed(remainder.c_str(), remainder.length()));

  // 今度は有効なリクエストが返る
  HTTPRequest completeResult = localParser.createRequest();
  EXPECT_EQ(completeResult.getMethod(), "GET");
}
