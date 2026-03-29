#include "OrderRepository.hpp"
#include <iostream>
#include <sstream>

OrderRepository::OrderRepository() {
  db = DatabaseManager::getInstance();
}

int OrderRepository::createOrder(Order& order) {
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "INSERT INTO orders (customerName, status, totalAmount) VALUES ('"
        << order.getCustomerName() << "', '"
        << orderStatusToString(order.getStatus()) << "', "
        << order.getTotalAmount() << ")";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Failed to create order: " << mysql_error(conn) << std::endl;
    return -1;
  }

  int orderId = (int)mysql_insert_id(conn);
  order.setId(orderId);

  for (auto& item : order.getItems()) {
    addOrderItem(orderId, item);
  }

  return orderId;
}

void OrderRepository::addOrderItem(int orderId, OrderItem& item) {
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "INSERT INTO order_items (orderId, productName, quantity, price) VALUES ("
        << orderId << ", '"
        << item.getProductName() << "', "
        << item.getQuantity() << ", "
        << item.getPrice() << ")";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Failed to add order item: " << mysql_error(conn) << std::endl;
  }
}

Order* OrderRepository::findById(int orderId) {
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "SELECT id, customerName, status, totalAmount, createdAt, updatedAt "
        << "FROM orders WHERE id = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    return nullptr;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return nullptr;

  MYSQL_ROW row = mysql_fetch_row(result);
  if (row == nullptr) {
    mysql_free_result(result);
    return nullptr;
  }

  Order* order = new Order(
    std::stoi(row[0]),
    std::string(row[1]),
    stringToOrderStatus(std::string(row[2])),
    std::stod(row[3]),
    std::string(row[4]),
    std::string(row[5])
  );

  mysql_free_result(result);

  std::vector<OrderItem> items = findItemsByOrderId(orderId);
  for (auto& item : items) {
    order->getItems().push_back(item);
  }

  return order;
}

std::vector<Order*> OrderRepository::findAll() {
  std::vector<Order*> orders;
  MYSQL* conn = db->getConnection();

  std::string query = "SELECT id, customerName, status, totalAmount, createdAt, updatedAt FROM orders";

  if (mysql_query(conn, query.c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    return orders;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return orders;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    Order* order = new Order(
      std::stoi(row[0]),
      std::string(row[1]),
      stringToOrderStatus(std::string(row[2])),
      std::stod(row[3]),
      std::string(row[4]),
      std::string(row[5])
    );

    std::vector<OrderItem> items = findItemsByOrderId(order->getId());
    for (auto& item : items) {
      order->getItems().push_back(item);
    }

    orders.push_back(order);
  }

  mysql_free_result(result);
  return orders;
}

std::vector<Order*> OrderRepository::findByStatus(OrderStatus status) {
  std::vector<Order*> orders;
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "SELECT id, customerName, status, totalAmount, createdAt, updatedAt "
        << "FROM orders WHERE status = '" << orderStatusToString(status) << "'";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    return orders;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return orders;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    Order* order = new Order(
      std::stoi(row[0]),
      std::string(row[1]),
      stringToOrderStatus(std::string(row[2])),
      std::stod(row[3]),
      std::string(row[4]),
      std::string(row[5])
    );

    std::vector<OrderItem> items = findItemsByOrderId(order->getId());
    for (auto& item : items) {
      order->getItems().push_back(item);
    }

    orders.push_back(order);
  }

  mysql_free_result(result);
  return orders;
}

bool OrderRepository::updateStatus(int orderId, OrderStatus status) {
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "UPDATE orders SET status = '" << orderStatusToString(status)
        << "' WHERE id = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Update error: " << mysql_error(conn) << std::endl;
    return false;
  }

  return mysql_affected_rows(conn) > 0;
}

std::vector<OrderItem> OrderRepository::findItemsByOrderId(int orderId) {
  std::vector<OrderItem> items;
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "SELECT id, orderId, productName, quantity, price "
        << "FROM order_items WHERE orderId = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    return items;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return items;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    OrderItem item(
      std::stoi(row[0]),
      std::stoi(row[1]),
      std::string(row[2]),
      std::stoi(row[3]),
      std::stod(row[4])
    );
    items.push_back(item);
  }

  mysql_free_result(result);
  return items;
}

std::vector<int> OrderRepository::findOrderIdsByStatus(OrderStatus status) {
  std::vector<int> ids;
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "SELECT id FROM orders WHERE status = '"
        << orderStatusToString(status) << "'";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    return ids;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return ids;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    ids.push_back(std::stoi(row[0]));
  }

  mysql_free_result(result);
  return ids;
}

std::vector<int> OrderRepository::findEligibleForPromotion(int minAgeSeconds) {
  std::vector<int> ids;
  MYSQL* conn = db->getConnection();

  std::ostringstream query;
  query << "SELECT id FROM orders WHERE status = 'PENDING' "
        << "AND createdAt <= NOW() - INTERVAL " << minAgeSeconds << " SECOND";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    return ids;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) return ids;

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    ids.push_back(std::stoi(row[0]));
  }

  mysql_free_result(result);
  return ids;
}
