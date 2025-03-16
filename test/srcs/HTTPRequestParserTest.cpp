#include <gtest/gtest.h>
#include "../../srcs/HTTPRequestParser.hpp"
#include "../../srcs/HTTPRequest.hpp"
#include <string>
#include <vector>

class HTTPRequestParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser = new HTTPRequestParser();
    }

    void TearDown() override {
        delete parser;
    }

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
    EXPECT_EQ(result.getURL(), "/index.html");  // Changed from getURI() to getURL()
    EXPECT_EQ(result.getVersion(), "HTTP/1.1");
    EXPECT_EQ(result.getHeaders().size(), 1);
    EXPECT_EQ(result.getHeader("Host"), "localhost");
}

TEST_F(HTTPRequestParserTest, PostRequestWithBody) {
    std::string request = "POST /submit-form HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Type: application/x-www-form-urlencoded\r\n"
                          "Content-Length: 27\r\n\r\n"
                          "username=john&password=1234";
    
    HTTPRequest result = parseRequest(request);

    EXPECT_EQ(result.getMethod(), "POST");
    EXPECT_EQ(result.getURL(), "/submit-form");  // Changed from getURI() to getURL()
    EXPECT_EQ(result.getBody(), "username=john&password=1234");
    EXPECT_EQ(result.getHeader("Content-Type"), "application/x-www-form-urlencoded");
    EXPECT_EQ(result.getHeader("Content-Length"), "27");
}

TEST_F(HTTPRequestParserTest, PutRequest) {
    std::string request = "PUT /resource HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "Content-Length: 15\r\n\r\n"
                          "Updated content";
    
    HTTPRequest result = parseRequest(request);

    EXPECT_EQ(result.getMethod(), "PUT");
    EXPECT_EQ(result.getURL(), "/resource");  // Changed from getURI() to getURL()
    EXPECT_EQ(result.getBody(), "Updated content");
}

TEST_F(HTTPRequestParserTest, DeleteRequest) {
    std::string request = "DELETE /resource/123 HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    
    HTTPRequest result = parseRequest(request);

    EXPECT_EQ(result.getMethod(), "DELETE");
    EXPECT_EQ(result.getURL(), "/resource/123");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, MultipleHeaders) {
    std::string request = "GET /page HTTP/1.1\r\n"
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
    std::string request = "INVALID /path HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    
    EXPECT_THROW(parseRequest(request), std::runtime_error);
}

TEST_F(HTTPRequestParserTest, MalformedRequest) {
    std::string request = "GET /path\r\n"
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
    std::string request = "GET /search?q=test&page=1 HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    
    HTTPRequest result = parseRequest(request);
    EXPECT_EQ(result.getURL(), "/search?q=test&page=1");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, RequestWithFragment) {
    std::string request = "GET /page#section1 HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    
    HTTPRequest result = parseRequest(request);
    EXPECT_EQ(result.getURL(), "/page#section1");  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, LongUri) {
    std::string longPath(2000, 'a');
    std::string request = "GET /" + longPath + " HTTP/1.1\r\n"
                          "Host: example.com\r\n\r\n";
    
    HTTPRequest result = parseRequest(request);
    EXPECT_EQ(result.getURL(), "/" + longPath);  // Changed from getURI() to getURL()
}

TEST_F(HTTPRequestParserTest, HeaderContinuationLine) {
    std::string request = "GET /path HTTP/1.1\r\n"
                          "User-Agent: Mozilla/5.0\r\n"
                          " (Windows NT 10.0; Win64; x64)\r\n"
                          "Host: example.com\r\n\r\n";
    
    HTTPRequest result = parseRequest(request);
    EXPECT_EQ(result.getHeader("User-Agent"), "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
