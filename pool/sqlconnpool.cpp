#include "sqlconnpool.h"

SqlConnPool *SqlConnPool::instance() {
    static SqlConnPool pool;
    return &pool;
}

MYSQL *SqlConnPool::getConn() {
    MYSQL *conn = nullptr;
    bool dequeued = false;
    if(connQue_.empty()) {
        LOG_WARN("SqlConnPool busy!\n");
        return nullptr;
    }
    sem_wait(&semId_);
    dequeued = connQue_.dequeue(conn);
    if(!dequeued) {
        LOG_WARN("SqlConnPool busy!\n");
        return nullptr;
    }
    return conn;
}

void SqlConnPool::freeConn(MYSQL *conn) {
    assert(conn);
    connQue_.dequeue(conn);
    sem_post(&semId_);
}

int SqlConnPool::getFreeConnCount() {
    return connQue_.size();
}

void SqlConnPool::init(const char *host, int port,
                const char *user, const char *pwd,
                const char *dbName, int connSize) {
    assert(connSize > 0);
    for(int i = 0; i < connSize; i++) {
        MYSQL *conn = mysql_init(conn);
        if(!conn) {
            LOG_ERROR("MySql Init Error...\n");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if(!conn) {
            LOG_ERROR("Mysql Connect Error...\n");
        }
        connQue_.enqueue(conn);
    }
    MAX_CONN_ = connSize;
    sem_init(&semId_, 0, MAX_CONN_);
}

void SqlConnPool::closePool() {
    MYSQL *conn = nullptr;
    while(connQue_.dequeue(conn)) {
        mysql_close(conn);
    }
    mysql_library_end();
}