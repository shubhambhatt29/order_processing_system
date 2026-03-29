#include "TestHelper.hpp"
#include "DatabaseManager.hpp"
#include "OrderService.hpp"
#include "BackgroundJob.hpp"
#include <thread>
#include <chrono>

static void cleanTestData() {
  MYSQL* conn = DatabaseManager::getInstance()->getConnection();
  mysql_query(conn, "DELETE FROM order_items");
  mysql_query(conn, "DELETE FROM orders");
  mysql_query(conn, "ALTER TABLE orders AUTO_INCREMENT = 1");
  mysql_query(conn, "ALTER TABLE order_items AUTO_INCREMENT = 1");
}

static int createTestOrder(OrderService* service, std::string name) {
  std::vector<OrderItem> items;
  items.push_back(OrderItem("TestProduct", 1, 50.00));
  return service->createOrder(name, items);
}

static int testCancelPendingOrder() {
  TestHelper t("Cancel Pending Order");
  OrderService service;

  int orderId = createTestOrder(&service, "Alice");
  t.assert_true(orderId > 0, "Order created");

  bool cancelled = service.cancelOrder(orderId);
  t.assert_true(cancelled, "Cancel returns true for PENDING order");

  Order* order = service.getOrderById(orderId);
  t.assert_not_null(order, "Order still exists");
  t.assert_equal(std::string("CANCELLED"), orderStatusToString(order->getStatus()),
                 "Status is CANCELLED");

  delete order;
  return t.printResults();
}

static int testCancelNonPendingOrder() {
  TestHelper t("Cancel Non-Pending Order");
  OrderService service;

  int orderId = createTestOrder(&service, "Bob");
  service.updateOrderStatus(orderId, PROCESSING);

  bool cancelled = service.cancelOrder(orderId);
  t.assert_false(cancelled, "Cannot cancel PROCESSING order");

  Order* order = service.getOrderById(orderId);
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(order->getStatus()),
                 "Status unchanged");

  delete order;
  return t.printResults();
}

static int testCancelShippedOrder() {
  TestHelper t("Cancel Shipped Order");
  OrderService service;

  int orderId = createTestOrder(&service, "Charlie");
  service.updateOrderStatus(orderId, PROCESSING);
  service.updateOrderStatus(orderId, SHIPPED);

  bool cancelled = service.cancelOrder(orderId);
  t.assert_false(cancelled, "Cannot cancel SHIPPED order");

  Order* order = service.getOrderById(orderId);
  t.assert_equal(std::string("SHIPPED"), orderStatusToString(order->getStatus()),
                 "Status unchanged");

  delete order;
  return t.printResults();
}

static int testStatusTransitions() {
  TestHelper t("Status Transitions");
  OrderService service;

  int orderId = createTestOrder(&service, "Dave");

  // PENDING -> PROCESSING
  bool updated = service.updateOrderStatus(orderId, PROCESSING);
  t.assert_true(updated, "Update to PROCESSING succeeds");
  Order* o1 = service.getOrderById(orderId);
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(o1->getStatus()),
                 "Status is PROCESSING");
  delete o1;

  // PROCESSING -> SHIPPED
  updated = service.updateOrderStatus(orderId, SHIPPED);
  t.assert_true(updated, "Update to SHIPPED succeeds");
  Order* o2 = service.getOrderById(orderId);
  t.assert_equal(std::string("SHIPPED"), orderStatusToString(o2->getStatus()),
                 "Status is SHIPPED");
  delete o2;

  // SHIPPED -> DELIVERED
  updated = service.updateOrderStatus(orderId, DELIVERED);
  t.assert_true(updated, "Update to DELIVERED succeeds");
  Order* o3 = service.getOrderById(orderId);
  t.assert_equal(std::string("DELIVERED"), orderStatusToString(o3->getStatus()),
                 "Status is DELIVERED");
  delete o3;

  return t.printResults();
}

static int testCancelNonExistentOrder() {
  TestHelper t("Cancel Non-Existent Order");
  OrderService service;

  bool cancelled = service.cancelOrder(99999);
  t.assert_false(cancelled, "Cannot cancel non-existent order");

  return t.printResults();
}

static int testBackgroundJobPromotion() {
  TestHelper t("Background Job Promotion (age-based)");
  OrderService service;

  int orderId = createTestOrder(&service, "Eve");

  // Manually backdate the order's createdAt by 6 minutes so it qualifies
  MYSQL* conn = DatabaseManager::getInstance()->getConnection();
  std::string backdate = "UPDATE orders SET createdAt = NOW() - INTERVAL 6 MINUTE WHERE id = "
                         + std::to_string(orderId);
  mysql_query(conn, backdate.c_str());

  // Create a fresh order that should NOT be promoted (just created)
  int freshId = createTestOrder(&service, "Frank");

  // Run promotion
  service.promotePendingOrders();

  // Old order should be promoted
  Order* old = service.getOrderById(orderId);
  t.assert_not_null(old, "Backdated order exists");
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(old->getStatus()),
                 "Backdated order promoted to PROCESSING");
  delete old;

  // Fresh order should still be PENDING
  Order* fresh = service.getOrderById(freshId);
  t.assert_not_null(fresh, "Fresh order exists");
  t.assert_equal(std::string("PENDING"), orderStatusToString(fresh->getStatus()),
                 "Fresh order still PENDING (not yet 5 min old)");
  delete fresh;

  return t.printResults();
}

static int testBackgroundJobStartStop() {
  TestHelper t("Background Job Start/Stop");
  OrderService service;

  BackgroundJob bgJob(&service, 2);

  t.assert_false(bgJob.isRunning(), "Not running before start");

  bgJob.start();
  t.assert_true(bgJob.isRunning(), "Running after start");

  bgJob.stop();
  t.assert_false(bgJob.isRunning(), "Not running after stop");

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
  totalFailures += testCancelPendingOrder();

  cleanTestData();
  totalFailures += testCancelNonPendingOrder();

  cleanTestData();
  totalFailures += testCancelShippedOrder();

  cleanTestData();
  totalFailures += testStatusTransitions();

  cleanTestData();
  totalFailures += testCancelNonExistentOrder();

  cleanTestData();
  totalFailures += testBackgroundJobPromotion();

  cleanTestData();
  totalFailures += testBackgroundJobStartStop();

  cleanTestData();
  db->disconnect();

  std::cout << "====================================" << std::endl;
  if (totalFailures == 0) {
    std::cout << "  ALL ORDER STATUS TESTS PASSED" << std::endl;
  } else {
    std::cout << "  " << totalFailures << " TOTAL FAILURE(S)" << std::endl;
  }
  std::cout << "====================================" << std::endl;

  return totalFailures;
}
