#include "HTTPRequestParser.hpp"

#include <strings.h>  // strcasecmpのために追加

#include <algorithm>
#include <cctype>

#include "HTTPRequest.hpp"

HTTPRequestParser::HTTPRequestParser()
    : state(RequestMethodStart),
      headersParsed(false),
      requestComplete(false),
      contentLength(0),
      chunked(false),
      chunkSize(0),
      parsingError(false) {}

HTTPRequestParser::~HTTPRequestParser() {
  // 動的メモリ割り当てがないので、特別なクリーンアップは不要
}

// テスト用のパブリックインターフェースを実装
HTTPRequest HTTPRequestParser::parseRequest(const std::string& buffer) {
  reset();

  // リクエストデータを解析
  feed(buffer.c_str(), buffer.length());

  if (hasError()) {
    throw std::runtime_error(getErrorMessage());
  }

  // リクエストラインが正しく解析されたか確認
  if (method.empty() || url.empty() || version.empty()) {
    throw std::runtime_error("リクエストラインが不完全または不正です");
  }

  // メソッドの検証
  if (method != "GET" && method != "POST" && method != "DELETE" &&
      method != "PUT" && method != "HEAD" && method != "OPTIONS" &&
      method != "TRACE" && method != "CONNECT") {
    throw std::runtime_error("サポートされていないHTTPメソッド: " + method);
  }

  if (!isComplete()) {
    throw std::runtime_error("リクエストが不完全です");
  }

  return createRequest();
}

bool HTTPRequestParser::feed(const char* data, size_t length) {
  if (isComplete() || hasError()) return isComplete();

  // 新しいデータをバッファに追加
  rawBuffer.append(data, length);

  // 文字単位での解析を実装
  ParseResult result = parse(rawBuffer);

  if (result == ParsingError) {
    parsingError = true;
    return false;
  }

  return (result == ParsingCompleted);
}

// 残りのコードはそのまま、ただし戻り値型を修正
HTTPRequestParser::ParseResult HTTPRequestParser::parse(std::string& buffer) {
  const char* begin = buffer.data();
  const char* end = begin + buffer.length();
  const char* currentPos = begin;

  while (currentPos != end) {
    char input = *currentPos++;

    switch (state) {
      case RequestMethodStart:
        if (!isChar(input) || isControl(input) || isSpecial(input)) {
          errorMessage = "無効なリクエストメソッド開始文字";
          return ParsingError;
        } else {
          state = RequestMethod;
          method.push_back(input);
        }
        break;

      case RequestMethod:
        if (input == ' ') {
          state = RequestUriStart;
        } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
          errorMessage = "無効なリクエストメソッド文字";
          return ParsingError;
        } else {
          method.push_back(input);
        }
        break;

      case RequestUriStart:
        if (isControl(input)) {
          errorMessage = "無効なURI開始文字";
          return ParsingError;
        } else {
          state = RequestUri;
          url.push_back(input);
        }
        break;

      case RequestUri:
        if (input == ' ') {
          state = RequestHttpVersion_h;
        } else if (input == '\r') {
          // HTTP/0.9 simple requestではなくエラーとして扱う
          errorMessage = "HTTPバージョンがありません";
          return ParsingError;
        } else if (isControl(input)) {
          errorMessage = "無効なURI文字";
          return ParsingError;
        } else {
          url.push_back(input);
        }
        break;

      case RequestHttpVersion_h:
        if (input == 'H') {
          state = RequestHttpVersion_ht;
        } else {
          errorMessage = "無効なHTTPバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_ht:
        if (input == 'T') {
          state = RequestHttpVersion_htt;
        } else {
          errorMessage = "無効なHTTPバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_htt:
        if (input == 'T') {
          state = RequestHttpVersion_http;
        } else {
          errorMessage = "無効なHTTPバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_http:
        if (input == 'P') {
          state = RequestHttpVersion_slash;
        } else {
          errorMessage = "無効なHTTPバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_slash:
        if (input == '/') {
          state = RequestHttpVersion_majorStart;
          version = "HTTP/";
        } else {
          errorMessage = "無効なHTTPバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_majorStart:
        if (isDigit(input)) {
          state = RequestHttpVersion_major;
          version.push_back(input);
        } else {
          errorMessage = "無効なHTTPメジャーバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_major:
        if (input == '.') {
          state = RequestHttpVersion_minorStart;
          version.push_back(input);
        } else if (isDigit(input)) {
          version.push_back(input);
        } else {
          errorMessage = "無効なHTTPメジャーバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_minorStart:
        if (isDigit(input)) {
          state = RequestHttpVersion_minor;
          version.push_back(input);
        } else {
          errorMessage = "無効なHTTPマイナーバージョン";
          return ParsingError;
        }
        break;

      case RequestHttpVersion_minor:
        if (input == '\r') {
          state = ExpectingNewline_1;
        } else if (isDigit(input)) {
          version.push_back(input);
        } else {
          errorMessage = "無効なHTTPマイナーバージョン";
          return ParsingError;
        }
        break;

      case ExpectingNewline_1:
        if (input == '\n') {
          state = HeaderLineStart;
        } else {
          errorMessage = "改行文字が必要";
          return ParsingError;
        }
        break;

      case HeaderLineStart:
        if (input == '\r') {
          state = ExpectingNewline_3;
        } else if (!headers.empty() && (input == ' ' || input == '\t')) {
          state = HeaderLws;
        } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
          errorMessage = "無効なヘッダー名の開始";
          return ParsingError;
        } else {
          currentHeaderName.clear();
          currentHeaderValue.clear();
          currentHeaderName.push_back(input);
          state = HeaderName;
        }
        break;

      case HeaderLws:
        if (input == '\r') {
          state = ExpectingNewline_2;
        } else if (input == ' ' || input == '\t') {
          // スペースをスキップ
        } else if (isControl(input)) {
          errorMessage = "無効なヘッダー値";
          return ParsingError;
        } else {
          state = HeaderValue;
          // 継続行の場合、スペースを追加
          if (!headers[currentHeaderName].empty()) {
            headers[currentHeaderName] += ' ';
          }
          headers[currentHeaderName] += input;
        }
        break;

      case HeaderName:
        if (input == ':') {
          state = SpaceBeforeHeaderValue;
          currentHeaderName = trimString(currentHeaderName);
        } else if (!isChar(input) || isControl(input) || isSpecial(input)) {
          errorMessage = "無効なヘッダー名";
          return ParsingError;
        } else {
          currentHeaderName.push_back(input);
        }
        break;

      case SpaceBeforeHeaderValue:
        if (input == ' ' || input == '\t') {
          // スペースをスキップ
        } else if (input == '\r') {
          // 空のヘッダー値
          headers[currentHeaderName] = "";
          state = ExpectingNewline_2;
        } else if (isControl(input)) {
          errorMessage = "無効なヘッダー値";
          return ParsingError;
        } else {
          headers[currentHeaderName] = input;
          state = HeaderValue;
        }
        break;

      case HeaderValue:
        if (input == '\r') {
          state = ExpectingNewline_2;
        } else if (isControl(input)) {
          errorMessage = "無効なヘッダー値";
          return ParsingError;
        } else {
          headers[currentHeaderName] += input;
        }
        break;

      case ExpectingNewline_2:
        if (input == '\n') {
          state = HeaderLineStart;
        } else {
          errorMessage = "改行文字が必要";
          return ParsingError;
        }
        break;

      case ExpectingNewline_3:
        if (input == '\n') {
          headersParsed = true;

          // Content-Lengthヘッダーをチェック
          std::string contentLengthHeader = getHeader("Content-Length");
          if (!contentLengthHeader.empty()) {
            std::istringstream ss(contentLengthHeader);
            ss >> contentLength;
            if (ss.fail()) {
              errorMessage = "無効なContent-Length値";
              return ParsingError;
            }
          }

          // Transfer-Encoding: chunkedをチェック
          std::string transferEncoding = getHeader("Transfer-Encoding");
          if (transferEncoding == "chunked") {
            chunked = true;
            state = ChunkSize;
          } else if (contentLength > 0) {
            state = MessageBody;
          } else {
            // ボディなし
            size_t processed = currentPos - begin;
            buffer.erase(0, processed);
            requestComplete = true;
            return ParsingCompleted;
          }
        } else {
          errorMessage = "改行文字が必要";
          return ParsingError;
        }
        break;

      case MessageBody:
        body.push_back(input);
        if (body.length() >= contentLength) {
          size_t processed = currentPos - begin;
          buffer.erase(0, processed);
          requestComplete = true;
          return ParsingCompleted;
        }
        break;

      case ChunkSize:
        if (isHexDigit(input)) {
          chunkSizeStr.push_back(input);
        } else if (input == ';') {
          state = ChunkExtension;
        } else if (input == '\r') {
          if (chunkSizeStr.empty()) {
            errorMessage = "チャンクサイズがありません";
            return ParsingError;
          }
          chunkSize = strtol(chunkSizeStr.c_str(), NULL, 16);
          chunkSizeStr.clear();
          state = ChunkSizeNewline;
        } else {
          errorMessage = "無効なチャンクサイズ";
          return ParsingError;
        }
        break;

      case ChunkExtension:
        if (input == '\r') {
          state = ChunkSizeNewline;
        }
        break;

      case ChunkSizeNewline:
        if (input == '\n') {
          if (chunkSize == 0) {
            state = ChunkTrailerStart;
          } else {
            state = ChunkData;
          }
        } else {
          errorMessage = "改行文字が必要";
          return ParsingError;
        }
        break;

      case ChunkData:
        body.push_back(input);
        chunkSize--;
        if (chunkSize == 0) {
          state = ChunkDataNewline_1;
        }
        break;

      case ChunkDataNewline_1:
        if (input == '\r') {
          state = ChunkDataNewline_2;
        } else {
          errorMessage = "チャンク後にCRが必要";
          return ParsingError;
        }
        break;

      case ChunkDataNewline_2:
        if (input == '\n') {
          state = ChunkSize;
        } else {
          errorMessage = "チャンク後にLFが必要";
          return ParsingError;
        }
        break;

      case ChunkTrailerStart:
        if (input == '\r') {
          state = ChunkTrailerNewline;
        } else {
          state = ChunkTrailer;
        }
        break;

      case ChunkTrailer:
        if (input == '\r') {
          state = ChunkTrailerNewline;
        }
        break;

      case ChunkTrailerNewline:
        if (input == '\n') {
          size_t processed = currentPos - begin;
          buffer.erase(0, processed);
          requestComplete = true;
          return ParsingCompleted;
        } else {
          errorMessage = "改行文字が必要";
          return ParsingError;
        }
        break;

      default:
        errorMessage = "未知の解析状態";
        return ParsingError;
    }
  }

  // バッファを消費した分だけ削除
  buffer.clear();
  return ParsingIncompleted;
}

bool HTTPRequestParser::parseRequestLine(const std::string& line) {
  std::istringstream ss(line);
  ss >> method >> url >> version;

  if (ss.fail()) {
    parsingError = true;
    errorMessage = "無効なリクエスト行フォーマット";
    return false;
  }

  // メソッドを検証
  if (method != "GET" && method != "POST" && method != "DELETE" &&
      method != "PUT" && method != "HEAD" && method != "OPTIONS" &&
      method != "TRACE" && method != "CONNECT") {
    parsingError = true;
    errorMessage = "サポートされていないHTTPメソッド";
    return false;
  }

  // HTTPバージョンを検証
  if (version != "HTTP/1.0" && version != "HTTP/1.1") {
    parsingError = true;
    errorMessage = "サポートされていないHTTPバージョン";
    return false;
  }

  return true;
}

bool HTTPRequestParser::parseHeader(const std::string& line) {
  std::string::size_type colonPos = line.find(':');
  if (colonPos == std::string::npos) {
    parsingError = true;
    errorMessage = "無効なヘッダーフォーマット: コロンがありません";
    return false;
  }

  std::string key = trimString(line.substr(0, colonPos));
  std::string value = trimString(line.substr(colonPos + 1));

  if (key.empty()) {
    parsingError = true;
    errorMessage = "空のヘッダーキー";
    return false;
  }

  headers[key] = value;
  return true;
}

std::string HTTPRequestParser::trimString(const std::string& str) const {
  std::string result = str;
  // 先頭の空白を削除
  while (!result.empty() && isspace(result[0])) result.erase(0, 1);

  // 末尾の空白を削除
  while (!result.empty() && isspace(result[result.length() - 1]))
    result.erase(result.length() - 1);

  return result;
}

bool HTTPRequestParser::isComplete() const { return requestComplete; }

bool HTTPRequestParser::hasError() const { return parsingError; }

std::string HTTPRequestParser::getErrorMessage() const { return errorMessage; }

std::string HTTPRequestParser::getMethod() const { return method; }

std::string HTTPRequestParser::getURL() const { return url; }

std::string HTTPRequestParser::getVersion() const { return version; }

std::string HTTPRequestParser::getHeader(const std::string& key) const {
  std::map<std::string, std::string>::const_iterator it = headers.find(key);
  if (it != headers.end()) return it->second;
  return "";
}

const std::map<std::string, std::string>& HTTPRequestParser::getHeaders()
    const {
  return headers;
}

std::string HTTPRequestParser::getBody() const { return body; }

void HTTPRequestParser::reset() {
  rawBuffer.clear();
  headersParsed = false;
  requestComplete = false;
  contentLength = 0;
  chunked = false;
  method.clear();
  url.clear();
  version.clear();
  headers.clear();
  body.clear();
  parsingError = false;
  errorMessage.clear();
  state = RequestMethodStart;
  chunkSize = 0;
  chunkSizeStr.clear();
  currentHeaderName.clear();
  currentHeaderValue.clear();
}

HTTPRequest HTTPRequestParser::createRequest() const {
  if (!isComplete() || hasError()) {
    return HTTPRequest();
  }

  bool keepAlive = false;

  // Keep-Aliveの判定
  std::map<std::string, std::string>::const_iterator it =
      headers.find("Connection");
  if (it != headers.end()) {
    if (strcasecmp(it->second.c_str(), "keep-alive") == 0) {
      keepAlive = true;
    }
  } else {
    // HTTP/1.1ではデフォルトでKeep-Alive
    if (version == "HTTP/1.1") {
      keepAlive = true;
    }
  }

  return HTTPRequest(method, url, version, headers, body, keepAlive);
}

// 追加のヘルパーメソッド
bool HTTPRequestParser::isChar(int c) const { return c >= 0 && c <= 127; }

bool HTTPRequestParser::isControl(int c) const {
  return (c >= 0 && c <= 31) || (c == 127);
}

bool HTTPRequestParser::isSpecial(int c) const {
  switch (c) {
    case '(':
    case ')':
    case '<':
    case '>':
    case '@':
    case ',':
    case ';':
    case ':':
    case '\\':
    case '"':
    case '/':
    case '[':
    case ']':
    case '?':
    case '=':
    case '{':
    case '}':
    case ' ':
    case '\t':
      return true;
    default:
      return false;
  }
}

bool HTTPRequestParser::isDigit(int c) const { return c >= '0' && c <= '9'; }

bool HTTPRequestParser::isHexDigit(int c) const {
  return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
