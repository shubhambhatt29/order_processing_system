#include "TestHelper.hpp"
#include "DatabaseManager.hpp"
#include "OrderService.hpp"

// Cleans test data before/after each suite
static void cleanTestData() {
  MYSQL* conn = DatabaseManager::getInstance()->getConnection();
  mysql_query(conn, "DELETE FROM order_items");
  mysql_query(conn, "DELETE FROM orders");
  mysql_query(conn, "ALTER TABLE orders AUTO_INCREMENT = 1");
  mysql_query(conn, "ALTER TABLE order_items AUTO_INCREMENT = 1");
}

static int testCreateSingleItemOrder() {
  TestHelper t("Create Single Item Order");
  OrderService service;

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Laptop", 1, 999.99));

  int orderId = service.createOrder("Alice", items);
  t.assert_true(orderId > 0, "Order ID should be positive");

  Order* order = service.getOrderById(orderId);
  t.assert_not_null(order, "Order should exist in DB");
  t.assert_equal(std::string("Alice"), order->getCustomerName(), "Customer name matches");
  t.assert_equal(std::string("PENDING"), orderStatusToString(order->getStatus()), "Status is PENDING");
  t.assert_equal(999.99, order->getTotalAmount(), "Total amount matches");
  t.assert_equal(1, (int)order->getItems().size(), "Has 1 item");
  t.assert_equal(std::string("Laptop"), order->getItems()[0].getProductName(), "Item name matches");

  delete order;
  return t.printResults();
}

static int testCreateMultiItemOrder() {
  TestHelper t("Create Multi-Item Order");
  OrderService service;

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Phone", 2, 499.99));
  items.push_back(OrderItem("Case", 2, 29.99));
  items.push_back(OrderItem("Charger", 1, 19.99));

  int orderId = service.createOrder("Bob", items);
  t.assert_true(orderId > 0, "Order ID should be positive");

  Order* order = service.getOrderById(orderId);
  t.assert_not_null(order, "Order should exist in DB");
  t.assert_equal(3, (int)order->getItems().size(), "Has 3 items");

  // 2*499.99 + 2*29.99 + 1*19.99 = 1079.95
  t.assert_equal(1079.95, order->getTotalAmount(), "Total calculated correctly");
  t.assert_equal(std::string("Bob"), order->getCustomerName(), "Customer name matches");

  delete order;
  return t.printResults();
}

static int testCreateOrderValidation() {
  TestHelper t("Create Order Validation");
  OrderService service;

  // Empty customer name
  std::vector<OrderItem> items;
  items.push_back(OrderItem("Widget", 1, 10.00));
  int id1 = service.createOrder("", items);
  t.assert_equal(-1, id1, "Reject empty customer name");

  // Empty items list
  std::vector<OrderItem> emptyItems;
  int id2 = service.createOrder("Charlie", emptyItems);
  t.assert_equal(-1, id2, "Reject empty items list");

  return t.printResults();
}

static int testRetrieveNonExistentOrder() {
  TestHelper t("Retrieve Non-Existent Order");
  OrderService service;

  Order* order = service.getOrderById(99999);
  t.assert_null(order, "Non-existent order returns nullptr");

  return t.printResults();
}

int main() {
  DatabaseManager* db = DatabaseManager::getInstance();
  if (!db->connect("localhost", "root", "", "order_processing_test")) {
    std::cerr << "Failed to connect to test database." << std::endl;
    return 1;
  }
  db->initializeSchema();

  int totalFailures = 0;

  cleanTestData();
  totalFailures += testCreateSingleItemOrder();

  cleanTestData();
  totalFailures += testCreateMultiItemOrder();

  cleanTestData();
  totalFailures += testCreateOrderValidation();

  cleanTestData();
  totalFailures += testRetrieveNonExistentOrder();

  cleanTestData();
  db->disconnect();

  std::cout << "====================================" << std::endl;
  if (totalFailures == 0) {
    std::cout << "  ALL ORDER CREATION TESTS PASSED" << std::endl;
  } else {
    std::cout << "  " << totalFailures << " TOTAL FAILURE(S)" << std::endl;
  }
  std::cout << "====================================" << std::endl;

  return totalFailures;
}
