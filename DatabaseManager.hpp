#pragma once

#include <string>
#include <mysql.h>

class DatabaseManager {
private:
  static DatabaseManager* instance;
  MYSQL* connection;
  std::string host;
  std::string user;
  std::string password;
  std::string database;
  unsigned int port;

  DatabaseManager();

public:
  static DatabaseManager* getInstance();
  ~DatabaseManager();

  bool connect(std::string host, std::string user,
               std::string password, std::string database,
               unsigned int port = 3306);
  void disconnect();
  MYSQL* getConnection();
  bool isConnected();
  void initializeSchema();
};
