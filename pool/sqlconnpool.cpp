#include "sqlconnpool.h"

SqlConnPool *SqlConnPool::instance() {
    static SqlConnPool pool;
    return &pool;
}

MYSQL *SqlConnPool::getConn() {

}

void SqlConnPool::freeConn(MYSQL *conn) {

}

int SqlConnPool::getFreeConnCount() {

}

void SqlConnPool::init(const char *host, int port,
                const char *user, const char *pwd,
                const char *dbName, int connSize) {

}

void SqlConnPool::closePool() {
    
}