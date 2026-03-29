#include "DatabaseManager.hpp"
#include <iostream>

DatabaseManager* DatabaseManager::instance = nullptr;

DatabaseManager::DatabaseManager()
  : connection(nullptr), port(3306) {
  connection = mysql_init(nullptr);
}

DatabaseManager::~DatabaseManager() {
  disconnect();
}

DatabaseManager* DatabaseManager::getInstance() {
  if (instance == nullptr) {
    instance = new DatabaseManager();
  }
  return instance;
}

bool DatabaseManager::connect(std::string host, std::string user,
                              std::string password, std::string database,
                              unsigned int port) {
  this->host = host;
  this->user = user;
  this->password = password;
  this->database = database;
  this->port = port;

  if (!mysql_real_connect(connection, host.c_str(), user.c_str(),
                          password.c_str(), nullptr, port, nullptr, 0)) {
    std::cerr << "MySQL connection error: " << mysql_error(connection) << std::endl;
    return false;
  }

  // Create database if it doesn't exist
  std::string createDb = "CREATE DATABASE IF NOT EXISTS " + database;
  mysql_query(connection, createDb.c_str());

  // Select the database
  if (mysql_select_db(connection, database.c_str()) != 0) {
    std::cerr << "Failed to select database: " << mysql_error(connection) << std::endl;
    return false;
  }

  std::cout << "Connected to MySQL database: " << database << std::endl;
  return true;
}

void DatabaseManager::disconnect() {
  if (connection != nullptr) {
    mysql_close(connection);
    connection = nullptr;
  }
}

MYSQL* DatabaseManager::getConnection() {
  return connection;
}

bool DatabaseManager::isConnected() {
  return connection != nullptr && mysql_ping(connection) == 0;
}

void DatabaseManager::initializeSchema() {
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

  if (mysql_query(connection, createOrders.c_str()) != 0) {
    std::cerr << "Failed to create orders table: " << mysql_error(connection) << std::endl;
  }

  if (mysql_query(connection, createItems.c_str()) != 0) {
    std::cerr << "Failed to create order_items table: " << mysql_error(connection) << std::endl;
  }

  std::cout << "Database schema initialized." << std::endl;
}
