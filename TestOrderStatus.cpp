#include "TestHelper.hpp"
#include "DatabaseManager.hpp"
#include "OrderService.hpp"
#include "BackgroundJob.hpp"
#include <thread>
#include <chrono>

static void cleanTestData() {
  DatabaseManager* db = DatabaseManager::getInstance();
  MYSQL* conn = db->acquire();
  mysql_query(conn, "DELETE FROM order_items");
  mysql_query(conn, "DELETE FROM orders");
  mysql_query(conn, "ALTER TABLE orders AUTO_INCREMENT = 1");
  mysql_query(conn, "ALTER TABLE order_items AUTO_INCREMENT = 1");
  db->release(conn);
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

  auto order = service.getOrderById(orderId);
  t.assert_true(order != nullptr, "Order still exists");
  t.assert_equal(std::string("CANCELLED"), orderStatusToString(order->getStatus()),
                 "Status is CANCELLED");

  return t.printResults();
}

static int testCancelNonPendingOrder() {
  TestHelper t("Cancel Non-Pending Order");
  OrderService service;

  int orderId = createTestOrder(&service, "Bob");
  service.updateOrderStatus(orderId, PROCESSING);

  bool cancelled = service.cancelOrder(orderId);
  t.assert_false(cancelled, "Cannot cancel PROCESSING order via cancelOrder");

  auto order = service.getOrderById(orderId);
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(order->getStatus()),
                 "Status unchanged");

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

  auto order = service.getOrderById(orderId);
  t.assert_equal(std::string("SHIPPED"), orderStatusToString(order->getStatus()),
                 "Status unchanged");

  return t.printResults();
}

static int testStatusTransitions() {
  TestHelper t("Status Transitions");
  OrderService service;

  int orderId = createTestOrder(&service, "Dave");

  // PENDING -> PROCESSING
  bool updated = service.updateOrderStatus(orderId, PROCESSING);
  t.assert_true(updated, "Update to PROCESSING succeeds");
  auto o1 = service.getOrderById(orderId);
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(o1->getStatus()),
                 "Status is PROCESSING");

  // PROCESSING -> SHIPPED
  updated = service.updateOrderStatus(orderId, SHIPPED);
  t.assert_true(updated, "Update to SHIPPED succeeds");
  auto o2 = service.getOrderById(orderId);
  t.assert_equal(std::string("SHIPPED"), orderStatusToString(o2->getStatus()),
                 "Status is SHIPPED");

  // SHIPPED -> DELIVERED
  updated = service.updateOrderStatus(orderId, DELIVERED);
  t.assert_true(updated, "Update to DELIVERED succeeds");
  auto o3 = service.getOrderById(orderId);
  t.assert_equal(std::string("DELIVERED"), orderStatusToString(o3->getStatus()),
                 "Status is DELIVERED");

  return t.printResults();
}

static int testInvalidTransitions() {
  TestHelper t("Invalid Status Transitions (State Machine)");
  OrderService service;

  // DELIVERED -> PENDING (invalid)
  int id1 = createTestOrder(&service, "TestA");
  service.updateOrderStatus(id1, PROCESSING);
  service.updateOrderStatus(id1, SHIPPED);
  service.updateOrderStatus(id1, DELIVERED);
  bool result = service.updateOrderStatus(id1, PENDING);
  t.assert_false(result, "DELIVERED -> PENDING rejected");

  // CANCELLED -> PROCESSING (invalid)
  int id2 = createTestOrder(&service, "TestB");
  service.cancelOrder(id2);
  result = service.updateOrderStatus(id2, PROCESSING);
  t.assert_false(result, "CANCELLED -> PROCESSING rejected");

  // SHIPPED -> PENDING (invalid)
  int id3 = createTestOrder(&service, "TestC");
  service.updateOrderStatus(id3, PROCESSING);
  service.updateOrderStatus(id3, SHIPPED);
  result = service.updateOrderStatus(id3, PENDING);
  t.assert_false(result, "SHIPPED -> PENDING rejected");

  // PENDING -> SHIPPED (skipping PROCESSING, invalid)
  int id4 = createTestOrder(&service, "TestD");
  result = service.updateOrderStatus(id4, SHIPPED);
  t.assert_false(result, "PENDING -> SHIPPED rejected (must go through PROCESSING)");

  // PENDING -> DELIVERED (skipping all, invalid)
  int id5 = createTestOrder(&service, "TestE");
  result = service.updateOrderStatus(id5, DELIVERED);
  t.assert_false(result, "PENDING -> DELIVERED rejected");

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
  DatabaseManager* dbMgr = DatabaseManager::getInstance();
  MYSQL* conn = dbMgr->acquire();
  std::string backdate = "UPDATE orders SET createdAt = NOW() - INTERVAL 6 MINUTE WHERE id = "
                         + std::to_string(orderId);
  mysql_query(conn, backdate.c_str());
  dbMgr->release(conn);

  // Create a fresh order that should NOT be promoted (just created)
  int freshId = createTestOrder(&service, "Frank");

  // Run promotion
  service.promotePendingOrders();

  // Old order should be promoted
  auto old = service.getOrderById(orderId);
  t.assert_true(old != nullptr, "Backdated order exists");
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(old->getStatus()),
                 "Backdated order promoted to PROCESSING");

  // Fresh order should still be PENDING
  auto fresh = service.getOrderById(freshId);
  t.assert_true(fresh != nullptr, "Fresh order exists");
  t.assert_equal(std::string("PENDING"), orderStatusToString(fresh->getStatus()),
                 "Fresh order still PENDING (not yet 5 min old)");

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
  totalFailures += testInvalidTransitions();

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
