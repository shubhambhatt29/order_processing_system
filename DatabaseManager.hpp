#pragma once

#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <mysql.h>

class DatabaseManager {
private:
  static DatabaseManager* instance;

  std::string host;
  std::string user;
  std::string password;
  std::string database;
  unsigned int port;
  int poolSize;

  std::queue<MYSQL*> pool;
  std::mutex poolMutex;
  std::condition_variable poolCondition;

  DatabaseManager();
  MYSQL* createConnection();

public:
  static DatabaseManager* getInstance();
  ~DatabaseManager();

  bool connect(std::string host, std::string user,
               std::string password, std::string database,
               unsigned int port = 3306, int poolSize = 5);
  void disconnect();

  MYSQL* acquire();
  void release(MYSQL* conn);

  bool isConnected();
  void initializeSchema();
};
