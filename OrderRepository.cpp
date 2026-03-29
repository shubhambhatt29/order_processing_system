#include "OrderRepository.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

OrderRepository::OrderRepository() {
  db = DatabaseManager::getInstance();
}

int OrderRepository::createOrder(Order& order) {
  MYSQL* conn = db->acquire();

  const char* sql = "INSERT INTO orders (customerName, status, totalAmount) VALUES (?, ?, ?)";
  MYSQL_STMT* stmt = mysql_stmt_init(conn);
  if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
    std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
    mysql_stmt_close(stmt);
    db->release(conn);
    return -1;
  }

  MYSQL_BIND bind[3];
  memset(bind, 0, sizeof(bind));

  std::string name = order.getCustomerName();
  std::string status = orderStatusToString(order.getStatus());
  double total = order.getTotalAmount();
  unsigned long nameLen = name.length();
  unsigned long statusLen = status.length();

  bind[0].buffer_type = MYSQL_TYPE_STRING;
  bind[0].buffer = (void*)name.c_str();
  bind[0].buffer_length = nameLen;
  bind[0].length = &nameLen;

  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer = (void*)status.c_str();
  bind[1].buffer_length = statusLen;
  bind[1].length = &statusLen;

  bind[2].buffer_type = MYSQL_TYPE_DOUBLE;
  bind[2].buffer = &total;

  mysql_stmt_bind_param(stmt, bind);

  if (mysql_stmt_execute(stmt) != 0) {
    std::cerr << "Failed to create order: " << mysql_stmt_error(stmt) << std::endl;
    mysql_stmt_close(stmt);
    db->release(conn);
    return -1;
  }

  int orderId = (int)mysql_stmt_insert_id(stmt);
  order.setId(orderId);
  mysql_stmt_close(stmt);

  // Insert order items using prepared statements
  const char* itemSql = "INSERT INTO order_items (orderId, productName, quantity, price) VALUES (?, ?, ?, ?)";
  for (auto& item : order.getItems()) {
    MYSQL_STMT* itemStmt = mysql_stmt_init(conn);
    if (mysql_stmt_prepare(itemStmt, itemSql, strlen(itemSql)) != 0) {
      std::cerr << "Failed to prepare item statement: " << mysql_stmt_error(itemStmt) << std::endl;
      mysql_stmt_close(itemStmt);
      continue;
    }

    MYSQL_BIND itemBind[4];
    memset(itemBind, 0, sizeof(itemBind));

    std::string productName = item.getProductName();
    int quantity = item.getQuantity();
    double price = item.getPrice();
    unsigned long productNameLen = productName.length();

    itemBind[0].buffer_type = MYSQL_TYPE_LONG;
    itemBind[0].buffer = &orderId;

    itemBind[1].buffer_type = MYSQL_TYPE_STRING;
    itemBind[1].buffer = (void*)productName.c_str();
    itemBind[1].buffer_length = productNameLen;
    itemBind[1].length = &productNameLen;

    itemBind[2].buffer_type = MYSQL_TYPE_LONG;
    itemBind[2].buffer = &quantity;

    itemBind[3].buffer_type = MYSQL_TYPE_DOUBLE;
    itemBind[3].buffer = &price;

    mysql_stmt_bind_param(itemStmt, itemBind);

    if (mysql_stmt_execute(itemStmt) != 0) {
      std::cerr << "Failed to add order item: " << mysql_stmt_error(itemStmt) << std::endl;
    }
    mysql_stmt_close(itemStmt);
  }

  db->release(conn);
  return orderId;
}

void OrderRepository::addOrderItem(int orderId, OrderItem& item) {
  MYSQL* conn = db->acquire();

  const char* sql = "INSERT INTO order_items (orderId, productName, quantity, price) VALUES (?, ?, ?, ?)";
  MYSQL_STMT* stmt = mysql_stmt_init(conn);
  if (mysql_stmt_prepare(stmt, sql, strlen(sql)) != 0) {
    std::cerr << "Failed to prepare statement: " << mysql_stmt_error(stmt) << std::endl;
    mysql_stmt_close(stmt);
    db->release(conn);
    return;
  }

  MYSQL_BIND bind[4];
  memset(bind, 0, sizeof(bind));

  std::string productName = item.getProductName();
  int quantity = item.getQuantity();
  double price = item.getPrice();
  unsigned long productNameLen = productName.length();

  bind[0].buffer_type = MYSQL_TYPE_LONG;
  bind[0].buffer = &orderId;

  bind[1].buffer_type = MYSQL_TYPE_STRING;
  bind[1].buffer = (void*)productName.c_str();
  bind[1].buffer_length = productNameLen;
  bind[1].length = &productNameLen;

  bind[2].buffer_type = MYSQL_TYPE_LONG;
  bind[2].buffer = &quantity;

  bind[3].buffer_type = MYSQL_TYPE_DOUBLE;
  bind[3].buffer = &price;

  mysql_stmt_bind_param(stmt, bind);

  if (mysql_stmt_execute(stmt) != 0) {
    std::cerr << "Failed to add order item: " << mysql_stmt_error(stmt) << std::endl;
  }

  mysql_stmt_close(stmt);
  db->release(conn);
}

std::unique_ptr<Order> OrderRepository::findById(int orderId) {
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "SELECT id, customerName, status, totalAmount, createdAt, updatedAt "
        << "FROM orders WHERE id = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return nullptr;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return nullptr;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  if (row == nullptr) {
    mysql_free_result(result);
    db->release(conn);
    return nullptr;
  }

  auto order = std::make_unique<Order>(
    std::stoi(row[0]),
    std::string(row[1]),
    stringToOrderStatus(std::string(row[2])),
    std::stod(row[3]),
    std::string(row[4]),
    std::string(row[5])
  );

  mysql_free_result(result);

  // Fetch items using the same connection
  std::ostringstream itemQuery;
  itemQuery << "SELECT id, orderId, productName, quantity, price "
            << "FROM order_items WHERE orderId = " << orderId;

  if (mysql_query(conn, itemQuery.str().c_str()) == 0) {
    MYSQL_RES* itemResult = mysql_store_result(conn);
    if (itemResult != nullptr) {
      MYSQL_ROW itemRow;
      while ((itemRow = mysql_fetch_row(itemResult)) != nullptr) {
        OrderItem item(
          std::stoi(itemRow[0]),
          std::stoi(itemRow[1]),
          std::string(itemRow[2]),
          std::stoi(itemRow[3]),
          std::stod(itemRow[4])
        );
        order->getItems().push_back(item);
      }
      mysql_free_result(itemResult);
    }
  }

  db->release(conn);
  return order;
}

std::vector<std::unique_ptr<Order>> OrderRepository::findAll() {
  std::vector<std::unique_ptr<Order>> orders;
  MYSQL* conn = db->acquire();

  std::string query = "SELECT id, customerName, status, totalAmount, createdAt, updatedAt FROM orders";

  if (mysql_query(conn, query.c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return orders;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return orders;
  }

  // Collect order data first, then release the result
  std::vector<std::tuple<int, std::string, std::string, double, std::string, std::string>> rows;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    rows.push_back({
      std::stoi(row[0]),
      std::string(row[1]),
      std::string(row[2]),
      std::stod(row[3]),
      std::string(row[4]),
      std::string(row[5])
    });
  }
  mysql_free_result(result);

  for (auto& [id, name, status, total, created, updated] : rows) {
    auto order = std::make_unique<Order>(id, name, stringToOrderStatus(status), total, created, updated);

    std::ostringstream itemQuery;
    itemQuery << "SELECT id, orderId, productName, quantity, price "
              << "FROM order_items WHERE orderId = " << id;

    if (mysql_query(conn, itemQuery.str().c_str()) == 0) {
      MYSQL_RES* itemResult = mysql_store_result(conn);
      if (itemResult != nullptr) {
        MYSQL_ROW itemRow;
        while ((itemRow = mysql_fetch_row(itemResult)) != nullptr) {
          order->getItems().push_back(OrderItem(
            std::stoi(itemRow[0]), std::stoi(itemRow[1]),
            std::string(itemRow[2]), std::stoi(itemRow[3]), std::stod(itemRow[4])
          ));
        }
        mysql_free_result(itemResult);
      }
    }

    orders.push_back(std::move(order));
  }

  db->release(conn);
  return orders;
}

std::vector<std::unique_ptr<Order>> OrderRepository::findByStatus(OrderStatus status) {
  std::vector<std::unique_ptr<Order>> orders;
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "SELECT id, customerName, status, totalAmount, createdAt, updatedAt "
        << "FROM orders WHERE status = '" << orderStatusToString(status) << "'";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return orders;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return orders;
  }

  std::vector<std::tuple<int, std::string, std::string, double, std::string, std::string>> rows;
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    rows.push_back({
      std::stoi(row[0]),
      std::string(row[1]),
      std::string(row[2]),
      std::stod(row[3]),
      std::string(row[4]),
      std::string(row[5])
    });
  }
  mysql_free_result(result);

  for (auto& [id, name, statusStr, total, created, updated] : rows) {
    auto order = std::make_unique<Order>(id, name, stringToOrderStatus(statusStr), total, created, updated);

    std::ostringstream itemQuery;
    itemQuery << "SELECT id, orderId, productName, quantity, price "
              << "FROM order_items WHERE orderId = " << id;

    if (mysql_query(conn, itemQuery.str().c_str()) == 0) {
      MYSQL_RES* itemResult = mysql_store_result(conn);
      if (itemResult != nullptr) {
        MYSQL_ROW itemRow;
        while ((itemRow = mysql_fetch_row(itemResult)) != nullptr) {
          order->getItems().push_back(OrderItem(
            std::stoi(itemRow[0]), std::stoi(itemRow[1]),
            std::string(itemRow[2]), std::stoi(itemRow[3]), std::stod(itemRow[4])
          ));
        }
        mysql_free_result(itemResult);
      }
    }

    orders.push_back(std::move(order));
  }

  db->release(conn);
  return orders;
}

bool OrderRepository::updateStatus(int orderId, OrderStatus status) {
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "UPDATE orders SET status = '" << orderStatusToString(status)
        << "' WHERE id = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Update error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return false;
  }

  bool updated = mysql_affected_rows(conn) > 0;
  db->release(conn);
  return updated;
}

std::vector<OrderItem> OrderRepository::findItemsByOrderId(int orderId) {
  std::vector<OrderItem> items;
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "SELECT id, orderId, productName, quantity, price "
        << "FROM order_items WHERE orderId = " << orderId;

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return items;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return items;
  }

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
  db->release(conn);
  return items;
}

std::vector<int> OrderRepository::findOrderIdsByStatus(OrderStatus status) {
  std::vector<int> ids;
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "SELECT id FROM orders WHERE status = '"
        << orderStatusToString(status) << "'";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    db->release(conn);
    return ids;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return ids;
  }

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    ids.push_back(std::stoi(row[0]));
  }

  mysql_free_result(result);
  db->release(conn);
  return ids;
}

std::vector<int> OrderRepository::findEligibleForPromotion(int minAgeSeconds) {
  std::vector<int> ids;
  MYSQL* conn = db->acquire();

  std::ostringstream query;
  query << "SELECT id FROM orders WHERE status = 'PENDING' "
        << "AND createdAt <= NOW() - INTERVAL " << minAgeSeconds << " SECOND";

  if (mysql_query(conn, query.str().c_str()) != 0) {
    std::cerr << "Query error: " << mysql_error(conn) << std::endl;
    db->release(conn);
    return ids;
  }

  MYSQL_RES* result = mysql_store_result(conn);
  if (result == nullptr) {
    db->release(conn);
    return ids;
  }

  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr) {
    ids.push_back(std::stoi(row[0]));
  }

  mysql_free_result(result);
  db->release(conn);
  return ids;
}
