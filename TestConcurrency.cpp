#include "TestHelper.hpp"
#include "DatabaseManager.hpp"
#include "OrderService.hpp"
#include "BackgroundJob.hpp"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

static void cleanTestData() {
  DatabaseManager* db = DatabaseManager::getInstance();
  MYSQL* conn = db->acquire();
  mysql_query(conn, "DELETE FROM order_items");
  mysql_query(conn, "DELETE FROM orders");
  mysql_query(conn, "ALTER TABLE orders AUTO_INCREMENT = 1");
  mysql_query(conn, "ALTER TABLE order_items AUTO_INCREMENT = 1");
  db->release(conn);
}

static int testConcurrentOrderCreation() {
  TestHelper t("Concurrent Order Creation (10 threads)");

  const int numThreads = 10;
  std::vector<std::thread> threads;
  std::atomic<int> successCount(0);
  std::atomic<int> failCount(0);

  for (int i = 0; i < numThreads; ++i) {
    threads.push_back(std::thread([i, &successCount, &failCount]() {
      OrderService service;
      std::string name = "Customer_" + std::to_string(i);

      std::vector<OrderItem> items;
      items.push_back(OrderItem("Product_" + std::to_string(i), 1, 10.00 * (i + 1)));

      int orderId = service.createOrder(name, items);
      if (orderId > 0) {
        successCount++;
      } else {
        failCount++;
      }
    }));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  t.assert_equal(numThreads, (int)successCount.load(), "All 10 orders created successfully");
  t.assert_equal(0, (int)failCount.load(), "No failures");

  // Verify all orders exist in DB
  OrderService service;
  auto allOrders = service.getAllOrders();
  t.assert_equal(numThreads, (int)allOrders.size(), "All 10 orders persisted in DB");

  return t.printResults();
}

static int testConcurrentReadWrite() {
  TestHelper t("Concurrent Reads While Writing");

  // Pre-create some orders
  OrderService service;
  for (int i = 0; i < 5; ++i) {
    std::vector<OrderItem> items;
    items.push_back(OrderItem("Existing_" + std::to_string(i), 1, 50.00));
    service.createOrder("Existing_" + std::to_string(i), items);
  }

  const int numReaders = 5;
  const int numWriters = 5;
  std::vector<std::thread> threads;
  std::atomic<int> readSuccess(0);
  std::atomic<int> writeSuccess(0);

  // Readers: fetch all orders repeatedly
  for (int i = 0; i < numReaders; ++i) {
    threads.push_back(std::thread([&readSuccess]() {
      OrderService svc;
      for (int j = 0; j < 3; ++j) {
        auto orders = svc.getAllOrders();
        if (!orders.empty()) {
          readSuccess++;
        }
      }
    }));
  }

  // Writers: create new orders
  for (int i = 0; i < numWriters; ++i) {
    threads.push_back(std::thread([i, &writeSuccess]() {
      OrderService svc;
      std::vector<OrderItem> items;
      items.push_back(OrderItem("New_" + std::to_string(i), 2, 25.00));
      int id = svc.createOrder("NewCustomer_" + std::to_string(i), items);
      if (id > 0) writeSuccess++;
    }));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  t.assert_equal(numWriters, (int)writeSuccess.load(), "All concurrent writes succeeded");
  t.assert_true(readSuccess.load() > 0, "Concurrent reads succeeded during writes");

  // Verify final count: 5 pre-existing + 5 new = 10
  auto allOrders = service.getAllOrders();
  t.assert_equal(10, (int)allOrders.size(), "All 10 orders exist after concurrent ops");

  return t.printResults();
}

static int testConcurrentStatusUpdates() {
  TestHelper t("Concurrent Status Updates");

  // Create 10 orders
  OrderService service;
  std::vector<int> orderIds;
  for (int i = 0; i < 10; ++i) {
    std::vector<OrderItem> items;
    items.push_back(OrderItem("Item_" + std::to_string(i), 1, 30.00));
    int id = service.createOrder("Customer_" + std::to_string(i), items);
    orderIds.push_back(id);
  }

  // 10 threads each update a different order's status
  std::vector<std::thread> threads;
  std::atomic<int> updateSuccess(0);

  for (int i = 0; i < 10; ++i) {
    threads.push_back(std::thread([i, &orderIds, &updateSuccess]() {
      OrderService svc;
      if (svc.updateOrderStatus(orderIds[i], PROCESSING)) {
        updateSuccess++;
      }
    }));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  t.assert_equal(10, (int)updateSuccess.load(), "All 10 concurrent updates succeeded");

  // Verify all are now PROCESSING
  auto processing = service.getOrdersByStatus(PROCESSING);
  t.assert_equal(10, (int)processing.size(), "All 10 orders now PROCESSING");

  return t.printResults();
}

static int testConcurrentCancelRace() {
  TestHelper t("Concurrent Cancel Race Condition");

  // Create one PENDING order
  OrderService service;
  std::vector<OrderItem> items;
  items.push_back(OrderItem("RaceItem", 1, 100.00));
  int orderId = service.createOrder("RaceCustomer", items);

  // 5 threads all try to cancel the same order simultaneously
  std::atomic<int> cancelSuccess(0);
  std::vector<std::thread> threads;

  for (int i = 0; i < 5; ++i) {
    threads.push_back(std::thread([orderId, &cancelSuccess]() {
      OrderService svc;
      if (svc.cancelOrder(orderId)) {
        cancelSuccess++;
      }
    }));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  // At least one should succeed, and the order should be CANCELLED
  t.assert_true(cancelSuccess.load() >= 1, "At least one cancel succeeded");

  auto order = service.getOrderById(orderId);
  t.assert_true(order != nullptr, "Order still exists");
  t.assert_equal(std::string("CANCELLED"), orderStatusToString(order->getStatus()),
                 "Order is CANCELLED");

  return t.printResults();
}

static int testBackgroundJobWithConcurrentOps() {
  TestHelper t("Background Job Running With Concurrent Operations");

  OrderService service;

  // Start background job with short interval
  BackgroundJob bgJob(&service, 2);
  bgJob.start();
  t.assert_true(bgJob.isRunning(), "Background job running");

  // While background job runs, create orders from multiple threads
  std::vector<std::thread> threads;
  std::atomic<int> created(0);

  for (int i = 0; i < 5; ++i) {
    threads.push_back(std::thread([i, &created]() {
      OrderService svc;
      std::vector<OrderItem> items;
      items.push_back(OrderItem("BGItem_" + std::to_string(i), 1, 20.00));
      int id = svc.createOrder("BGCustomer_" + std::to_string(i), items);
      if (id > 0) created++;
    }));
  }

  for (auto& thread : threads) {
    thread.join();
  }

  bgJob.stop();

  t.assert_equal(5, (int)created.load(), "All orders created while bg job running");

  auto allOrders = service.getAllOrders();
  t.assert_equal(5, (int)allOrders.size(), "All 5 orders persisted");

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
  totalFailures += testConcurrentOrderCreation();

  cleanTestData();
  totalFailures += testConcurrentReadWrite();

  cleanTestData();
  totalFailures += testConcurrentStatusUpdates();

  cleanTestData();
  totalFailures += testConcurrentCancelRace();

  cleanTestData();
  totalFailures += testBackgroundJobWithConcurrentOps();

  cleanTestData();
  db->disconnect();

  std::cout << "====================================" << std::endl;
  if (totalFailures == 0) {
    std::cout << "  ALL CONCURRENCY TESTS PASSED" << std::endl;
  } else {
    std::cout << "  " << totalFailures << " TOTAL FAILURE(S)" << std::endl;
  }
  std::cout << "====================================" << std::endl;

  return totalFailures;
}
