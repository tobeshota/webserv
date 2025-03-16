#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP

#include "HTTPRequest.hpp"
#include <string>
#include <map>
#include <sstream>

// Forward declaration
class HTTPRequest;

/**
 * @class HTTPRequestParser
 * @brief HTTPリクエストを解析するクラス
 *
 * 依存関係:
 * - HTTPRequest: 解析されたデータを格納するクラス
 * - HTTPServer: 着信接続を処理するためにこのパーサーを使用
 *
 * エラー処理戦略:
 * - 例外ではなくエラーフラグとメッセージを使用
 * - 呼び出し側のコードは解析後にhasError()をチェックする必要がある
 */
class HTTPRequestParser {
public:
    HTTPRequestParser();
    ~HTTPRequestParser();

    // パブリックインターフェース - テストからアクセスするためのメソッド
    HTTPRequest parseRequest(const std::string& buffer);

    /**
     * @brief データをパーサーに供給する
     * @param data 解析する生データ
     * @param length データの長さ
     * @return 解析が完了したらtrue、さらにデータが必要な場合はfalse
     */
    bool feed(const char* data, size_t length);
    
    /**
     * @brief リクエスト解析が完了したかチェック
     * @return 完了した場合はtrue、それ以外はfalse
     */
    bool isComplete() const;
    
    /**
     * @brief 解析中にエラーが発生したかチェック
     * @return エラーがある場合はtrue、それ以外はfalse
     */
    bool hasError() const;
    
    /**
     * @brief エラーが発生した場合のエラーメッセージを取得
     * @return エラーメッセージ文字列
     */
    std::string getErrorMessage() const;
    
    // 解析されたデータのアクセサ
    std::string getMethod() const;
    std::string getURL() const;
    std::string getVersion() const;
    std::string getHeader(const std::string& key) const;
    const std::map<std::string, std::string>& getHeaders() const;
    std::string getBody() const;
    
    /**
     * @brief 解析されたデータからHTTPRequestを作成
     * @return HTTPRequestオブジェクト
     */
    HTTPRequest createRequest() const;
    
    /**
     * @brief 新しいリクエストを解析するためにパーサーの状態をリセット
     */
    void reset();

private:
    // パース結果の列挙型
    enum ParseResult {
        ParsingCompleted,
        ParsingIncompleted,
        ParsingError
    };

    // パーサーの状態
    enum State {
        RequestMethodStart,
        RequestMethod,
        RequestUriStart,
        RequestUri,
        RequestHttpVersion_h,
        RequestHttpVersion_ht,
        RequestHttpVersion_htt,
        RequestHttpVersion_http,
        RequestHttpVersion_slash,
        RequestHttpVersion_majorStart,
        RequestHttpVersion_major,
        RequestHttpVersion_minorStart,
        RequestHttpVersion_minor,
        ExpectingNewline_1,
        HeaderLineStart,
        HeaderLws,
        HeaderName,
        SpaceBeforeHeaderValue,
        HeaderValue,
        ExpectingNewline_2,
        ExpectingNewline_3,
        MessageBody,
        ChunkSize,
        ChunkExtension,
        ChunkSizeNewline,
        ChunkData,
        ChunkDataNewline_1,
        ChunkDataNewline_2,
        ChunkTrailerStart,
        ChunkTrailer,
        ChunkTrailerNewline
    } state;

    // 内部状態
    std::string rawBuffer;
    bool headersParsed;
    bool requestComplete;
    size_t contentLength;
    bool chunked;
    size_t chunkSize;
    std::string chunkSizeStr;
    std::string currentHeaderName;
    std::string currentHeaderValue;
    
    // 解析されたデータ
    std::string method;
    std::string url;
    std::string version;
    std::map<std::string, std::string> headers;
    std::string body;
    
    // エラー処理
    bool parsingError;
    std::string errorMessage;
    
    // コピー防止
    HTTPRequestParser(const HTTPRequestParser&);
    HTTPRequestParser& operator=(const HTTPRequestParser&);
    
    // ヘルパーメソッド
    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    std::string trimString(const std::string& str) const;
    ParseResult parse(std::string& buffer);
    
    // 文字検証ヘルパーメソッド
    bool isChar(int c) const;
    bool isControl(int c) const;
    bool isSpecial(int c) const;
    bool isDigit(int c) const;
    bool isHexDigit(int c) const;
};

#endif // HTTP_REQUEST_PARSER_HPP
