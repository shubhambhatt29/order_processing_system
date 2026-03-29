#include "DatabaseManager.hpp"
#include <iostream>

DatabaseManager* DatabaseManager::instance = nullptr;

DatabaseManager::DatabaseManager()
  : port(3306), poolSize(5) {}

DatabaseManager::~DatabaseManager() {
  disconnect();
}

DatabaseManager* DatabaseManager::getInstance() {
  if (instance == nullptr) {
    instance = new DatabaseManager();
  }
  return instance;
}

MYSQL* DatabaseManager::createConnection() {
  MYSQL* conn = mysql_init(nullptr);
  if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
                          password.c_str(), database.c_str(),
                          port, nullptr, 0)) {
    std::cerr << "MySQL connection error: " << mysql_error(conn) << std::endl;
    mysql_close(conn);
    return nullptr;
  }
  return conn;
}

bool DatabaseManager::connect(std::string host, std::string user,
                              std::string password, std::string database,
                              unsigned int port, int poolSize) {
  this->host = host;
  this->user = user;
  this->password = password;
  this->database = database;
  this->port = port;
  this->poolSize = poolSize;

  // Create database using a temporary connection
  MYSQL* tempConn = mysql_init(nullptr);
  if (!mysql_real_connect(tempConn, host.c_str(), user.c_str(),
                          password.c_str(), nullptr, port, nullptr, 0)) {
    std::cerr << "MySQL connection error: " << mysql_error(tempConn) << std::endl;
    mysql_close(tempConn);
    return false;
  }

  std::string createDb = "CREATE DATABASE IF NOT EXISTS " + database;
  mysql_query(tempConn, createDb.c_str());
  mysql_close(tempConn);

  // Fill the pool
  for (int i = 0; i < poolSize; ++i) {
    MYSQL* conn = createConnection();
    if (conn == nullptr) {
      std::cerr << "Failed to create pool connection " << (i + 1) << std::endl;
      disconnect();
      return false;
    }
    pool.push(conn);
  }

  std::cout << "Connection pool created with " << poolSize
            << " connections to database: " << database << std::endl;
  return true;
}

void DatabaseManager::disconnect() {
  std::lock_guard<std::mutex> lock(poolMutex);
  while (!pool.empty()) {
    MYSQL* conn = pool.front();
    pool.pop();
    mysql_close(conn);
  }
}

MYSQL* DatabaseManager::acquire() {
  std::unique_lock<std::mutex> lock(poolMutex);
  poolCondition.wait(lock, [this] { return !pool.empty(); });

  MYSQL* conn = pool.front();
  pool.pop();

  // Reconnect if the connection went stale
  if (mysql_ping(conn) != 0) {
    mysql_close(conn);
    conn = createConnection();
  }

  return conn;
}

void DatabaseManager::release(MYSQL* conn) {
  std::lock_guard<std::mutex> lock(poolMutex);
  pool.push(conn);
  poolCondition.notify_one();
}

bool DatabaseManager::isConnected() {
  std::lock_guard<std::mutex> lock(poolMutex);
  return !pool.empty();
}

void DatabaseManager::initializeSchema() {
  MYSQL* conn = acquire();

  std::string createOrders =
    "CREATE TABLE IF NOT EXISTS orders ("
    "  id INT AUTO_INCREMENT PRIMARY KEY,"
    "  customerName VARCHAR(255) NOT NULL,"
    "  status VARCHAR(20) NOT NULL DEFAULT 'PENDING',"
    "  totalAmount DOUBLE NOT NULL DEFAULT 0.0,"
    "  createdAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "  updatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"
    ")";

  std::string createItems =
    "CREATE TABLE IF NOT EXISTS order_items ("
    "  id INT AUTO_INCREMENT PRIMARY KEY,"
    "  orderId INT NOT NULL,"
    "  productName VARCHAR(255) NOT NULL,"
    "  quantity INT NOT NULL DEFAULT 1,"
    "  price DOUBLE NOT NULL,"
    "  FOREIGN KEY (orderId) REFERENCES orders(id) ON DELETE CASCADE"
    ")";

  if (mysql_query(conn, createOrders.c_str()) != 0) {
    std::cerr << "Failed to create orders table: " << mysql_error(conn) << std::endl;
  }

  if (mysql_query(conn, createItems.c_str()) != 0) {
    std::cerr << "Failed to create order_items table: " << mysql_error(conn) << std::endl;
  }

  release(conn);
  std::cout << "Database schema initialized." << std::endl;
}
