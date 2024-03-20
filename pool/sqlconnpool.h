#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include<mysql/mysql.h>
#include<string>
#include<mutex>
#include<thread>
#include<semaphore.h>
#include "../log/log.h"
#include "safequeue.h"

class SqlConnPool {
public:
    static SqlConnPool *instance();
    MYSQL *getConn();
    void freeConn(MYSQL* conn);
    int getFreeConnCount();
    void init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);
    void closePool();
private:
    SqlConnPool() = default;
    ~SqlConnPool() {closePool(); }

    int MAX_CONN_;

    SafeQueue<MYSQL *> connQue_;
    sem_t semId_;
};

class SqlConnRAII {
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool) : connpool_(connpool) {
        assert(connpool_);
        *sql = connpool_->getConn();
        sql_ = *sql;
    }
    ~SqlConnRAII() {
        if(sql_) {
            connpool_->freeConn(sql_);
        }
    }
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif