#include "httprequest.h"

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML {
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
};

const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG {
    {"/register.html", 0}, 
    {"/login.html", 1},
};

void HttpRequest::Init() {
    method_ = path_ = version_ = body_ = "";
    state_ = PARSE_STATE::REQUEST_LINE;
    header_.clear();
    post_.clear();
}

bool HttpRequest::isKeepAlive() const {
    if(header_.count("Connection")) {
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    return false;
}

bool HttpRequest::parse(Buffer &buff) {
    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0) {
        return false;
    }
    // read data
    while(buff.readableBytes() && state_ != PARSE_STATE::FINISH) {
        // split by "\r\n"
        const char* lineEnd = std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        // turn into string
        std::string line(buff.peek(), lineEnd);
        switch (state_) { // DFA
        case PARSE_STATE::REQUEST_LINE:
            if(!parseRequestLine_(line)) {
                return false;
            }
            parsePath_();
            break;
        case PARSE_STATE::HEADERS:
            parseHeader_(line);
            if(buff.readableBytes() <= 2) {
                state_ = PARSE_STATE::FINISH;
            }
            break;
        case PARSE_STATE::BODY:
            parseBody_(line);
            break;
        default:
            break;
        }
        // finished reading
        if(lineEnd == buff.beginWrite()) {
            break; 
        }
        // skip the enter and newline
        buff.retrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// decode the path
void HttpRequest::parsePath_() {
    if(path_ == "/") {
        path_ = "/index.html";
    } else {
        for(auto &item : DEFAULT_HTML) if(item == path_) {
            path_ += ".html";
            break;
        }
    }
}

bool HttpRequest::parseRequestLine_(const std::string &line) {
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    // In the matching rules, groups are divided by brackets().
    // Three brackets in total
    // [0] indicate the overall
    if(std::regex_match(line, subMatch, pattern)) {
        method_ = subMatch[1];
        method_ = subMatch[2];
        version_ = subMatch[3];
        state_ = PARSE_STATE::HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::parseHeader_(const std::string &line) {
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(std::regex_match(line, subMatch, pattern)) {
        header_[subMatch[1]] = subMatch[2];
    } else {
        state_ = PARSE_STATE::BODY;
    }
}

void HttpRequest::parseBody_(const std::string &line) {
    body_ = line;
    parsePost_();
    state_ = PARSE_STATE::FINISH;
    LOG_DEBUG("Body:%s, len: %d", line.c_str(), line.size());
}

int HttpRequest::converHex(char ch) {
    if('A' <= ch && ch <= 'F') return ch - 'A' + 10;
    if('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    return ch - '0';
}

void HttpRequest::parsePost_() {
    if(method_ == "POST" && header_["Content_Type"] == "application/x-ww-form-urlencoded") {
        parseFromUrlencoded_();
        // login/register request
        if(DEFAULT_HTML_TAG.count(path_)) {
            int tag = DEFAULT_HTML_TAG.find(path_)->second;
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);
                if(userVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } else {
                    path_ = "/error.html";
                }
            }
            
        }
    }
}

void HttpRequest::parseFromUrlencoded_() {
    if(body_.size() == 0) {
        return ;
    }
    std::string key, value;
    int num = 0, n = body_.size(), i = 0, j = 0;
    for(; i < n; i++) {
        char ch = body_[i];
        switch (ch) {
        case '=':
            key = body_.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            body_[i] = ' ';
            break;
        case '%':
            num = converHex(body_[i + 1]) * 16 + converHex(body_[i + 2]);
            body_[i + 2] = num % 10 + '0';
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;
        // k-v connector
        case '&':
            value = body_.substr(j, i - j);
            j = i + 1;
            post_[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);
    if(!post_.count(key) && j < i) {
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::userVerify(const std::string &name, const std::string &pwd, bool isLogin) {
    if(name == "" || pwd == "") {
        return false;
    }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    // sqlConnPool must initialize ?
    SqlConnRAII(&sql, SqlConnPool::instance());
    assert(sql);
    
    bool flag = false;
    char order[256]{0};
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if(!isLogin) {
        // register
        flag = true; 
    }

    snprintf(order, 256, "SELECT username, password, FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) {
        mysql_free_result(res);
    }

    res = mysql_store_result(sql);
    fields = mysql_fetch_field(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        std::string password(row[1]);
        if(isLogin) {
            if(pwd == password) {
                flag = true; 
            } else {
                flag = false;
                LOG_INFO("%s: pwd error", name);
            }
        } else {
            flag = false;
            LOG_INFO("user %s used", name);
        }
    }
    mysql_free_result(res);

    if(!isLogin && flag == true) {
        LOG_DEBUG("%s: register", name);
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if(mysql_query(sql, order)) {
            LOG_DEBUG("Insert error");
            flag = false;
        }
    }
    LOG_DEBUG("User verify success");
    Log::instance()->flush();
    return flag;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string& HttpRequest::path() {
    return path_;
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

std::string HttpRequest::getPost(const std::string &key) const {
    assert(key.size());
    if(post_.count(key)) {
        return post_.find(key)->second;
    }
    return "";
}