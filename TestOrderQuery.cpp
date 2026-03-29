#include "TestHelper.hpp"
#include "DatabaseManager.hpp"
#include "OrderService.hpp"

static void cleanTestData() {
  MYSQL* conn = DatabaseManager::getInstance()->getConnection();
  mysql_query(conn, "DELETE FROM order_items");
  mysql_query(conn, "DELETE FROM orders");
  mysql_query(conn, "ALTER TABLE orders AUTO_INCREMENT = 1");
  mysql_query(conn, "ALTER TABLE order_items AUTO_INCREMENT = 1");
}

static int createTestOrder(OrderService* service, std::string name,
                           OrderStatus targetStatus = PENDING) {
  std::vector<OrderItem> items;
  items.push_back(OrderItem("Product", 1, 25.00));
  int id = service->createOrder(name, items);
  if (targetStatus != PENDING) {
    service->updateOrderStatus(id, targetStatus);
  }
  return id;
}

static int testListAllOrders() {
  TestHelper t("List All Orders");
  OrderService service;

  createTestOrder(&service, "Alice");
  createTestOrder(&service, "Bob");
  createTestOrder(&service, "Charlie");

  std::vector<Order*> orders = service.getAllOrders();
  t.assert_equal(3, (int)orders.size(), "Returns 3 orders");

  for (auto* o : orders) delete o;
  return t.printResults();
}

static int testListEmptyOrders() {
  TestHelper t("List Empty Orders");
  OrderService service;

  std::vector<Order*> orders = service.getAllOrders();
  t.assert_equal(0, (int)orders.size(), "Returns 0 orders when DB is empty");

  return t.printResults();
}

static int testFilterByPendingStatus() {
  TestHelper t("Filter By PENDING Status");
  OrderService service;

  createTestOrder(&service, "Alice");
  createTestOrder(&service, "Bob");
  createTestOrder(&service, "Charlie", PROCESSING);

  std::vector<Order*> pending = service.getOrdersByStatus(PENDING);
  t.assert_equal(2, (int)pending.size(), "2 orders are PENDING");

  for (auto* o : pending) delete o;
  return t.printResults();
}

static int testFilterByProcessingStatus() {
  TestHelper t("Filter By PROCESSING Status");
  OrderService service;

  createTestOrder(&service, "Alice");
  createTestOrder(&service, "Bob", PROCESSING);
  createTestOrder(&service, "Charlie", PROCESSING);

  std::vector<Order*> processing = service.getOrdersByStatus(PROCESSING);
  t.assert_equal(2, (int)processing.size(), "2 orders are PROCESSING");

  for (auto* o : processing) delete o;
  return t.printResults();
}

static int testFilterByShippedStatus() {
  TestHelper t("Filter By SHIPPED Status");
  OrderService service;

  createTestOrder(&service, "Alice", SHIPPED);
  createTestOrder(&service, "Bob");
  createTestOrder(&service, "Charlie", PROCESSING);

  std::vector<Order*> shipped = service.getOrdersByStatus(SHIPPED);
  t.assert_equal(1, (int)shipped.size(), "1 order is SHIPPED");
  t.assert_equal(std::string("Alice"), shipped[0]->getCustomerName(),
                 "Shipped order belongs to Alice");

  for (auto* o : shipped) delete o;
  return t.printResults();
}

static int testFilterReturnsEmptyForNoMatch() {
  TestHelper t("Filter Returns Empty For No Match");
  OrderService service;

  createTestOrder(&service, "Alice");
  createTestOrder(&service, "Bob");

  std::vector<Order*> delivered = service.getOrdersByStatus(DELIVERED);
  t.assert_equal(0, (int)delivered.size(), "No DELIVERED orders");

  for (auto* o : delivered) delete o;
  return t.printResults();
}

static int testGetOrderByIdWithItems() {
  TestHelper t("Get Order By ID With Items");
  OrderService service;

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Keyboard", 1, 75.00));
  items.push_back(OrderItem("Mouse", 2, 25.00));
  items.push_back(OrderItem("Monitor", 1, 300.00));

  int orderId = service.createOrder("Dave", items);

  Order* order = service.getOrderById(orderId);
  t.assert_not_null(order, "Order exists");
  t.assert_equal(3, (int)order->getItems().size(), "Has 3 items");
  t.assert_equal(425.00, order->getTotalAmount(), "Total = 75 + 50 + 300");

  // Verify each item
  bool hasKeyboard = false, hasMouse = false, hasMonitor = false;
  for (auto& item : order->getItems()) {
    if (item.getProductName() == "Keyboard") hasKeyboard = true;
    if (item.getProductName() == "Mouse") hasMouse = true;
    if (item.getProductName() == "Monitor") hasMonitor = true;
  }
  t.assert_true(hasKeyboard, "Contains Keyboard item");
  t.assert_true(hasMouse, "Contains Mouse item");
  t.assert_true(hasMonitor, "Contains Monitor item");

  delete order;
  return t.printResults();
}

static int testMultipleOrdersIndependent() {
  TestHelper t("Multiple Orders Are Independent");
  OrderService service;

  std::vector<OrderItem> items1;
  items1.push_back(OrderItem("ItemA", 1, 100.00));
  int id1 = service.createOrder("Alice", items1);

  std::vector<OrderItem> items2;
  items2.push_back(OrderItem("ItemB", 3, 50.00));
  int id2 = service.createOrder("Bob", items2);

  t.assert_true(id1 != id2, "Different order IDs");

  Order* o1 = service.getOrderById(id1);
  Order* o2 = service.getOrderById(id2);

  t.assert_equal(100.00, o1->getTotalAmount(), "Order 1 total correct");
  t.assert_equal(150.00, o2->getTotalAmount(), "Order 2 total correct");
  t.assert_equal(1, (int)o1->getItems().size(), "Order 1 has 1 item");
  t.assert_equal(1, (int)o2->getItems().size(), "Order 2 has 1 item");

  // Cancel order 1, order 2 should be unaffected
  service.cancelOrder(id1);
  Order* o1After = service.getOrderById(id1);
  Order* o2After = service.getOrderById(id2);

  t.assert_equal(std::string("CANCELLED"), orderStatusToString(o1After->getStatus()),
                 "Order 1 cancelled");
  t.assert_equal(std::string("PENDING"), orderStatusToString(o2After->getStatus()),
                 "Order 2 still PENDING");

  delete o1;
  delete o2;
  delete o1After;
  delete o2After;
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
  totalFailures += testListAllOrders();

  cleanTestData();
  totalFailures += testListEmptyOrders();

  cleanTestData();
  totalFailures += testFilterByPendingStatus();

  cleanTestData();
  totalFailures += testFilterByProcessingStatus();

  cleanTestData();
  totalFailures += testFilterByShippedStatus();

  cleanTestData();
  totalFailures += testFilterReturnsEmptyForNoMatch();

  cleanTestData();
  totalFailures += testGetOrderByIdWithItems();

  cleanTestData();
  totalFailures += testMultipleOrdersIndependent();

  cleanTestData();
  db->disconnect();

  std::cout << "====================================" << std::endl;
  if (totalFailures == 0) {
    std::cout << "  ALL ORDER QUERY TESTS PASSED" << std::endl;
  } else {
    std::cout << "  " << totalFailures << " TOTAL FAILURE(S)" << std::endl;
  }
  std::cout << "====================================" << std::endl;

  return totalFailures;
}
