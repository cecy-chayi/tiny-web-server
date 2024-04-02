#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <errno.h>
#include <mysql/mysql.h>

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };

    HttpRequest() {init(); }
    ~HttpRequest() = default;

    void init();
    bool parse(Buffer &buff);

    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string getPost(const std::string &key) const;
    std::string getPost(const char* key) const;

    bool isKeepAlive() const;
private:
    bool parseRequestLine_(const std::string &line);    // processing request line
    void parseHeader_(const std::string &line);         // processing request header
    void parseBody_(const std::string &line);           // processing request body

    void parsePath_();              // processing the request url
    void parsePost_();              // processing the post request
    void parseFromUrlencoded_();    // decode the url

    static bool userVerify(const std::string &name, const std::string &pwd, bool isLogin); // verify user
    static int converHex(char ch);  // convert hexadecimal to decimal

    PARSE_STATE state_;
    std::string method_, path_, version_, body_;
    std::unordered_map<std::string, std::string> header_;
    std::unordered_map<std::string, std::string> post_;

    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
};

#endif
