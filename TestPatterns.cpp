#include "TestHelper.hpp"
#include "IOrderRepository.hpp"
#include "IOrderObserver.hpp"
#include "IPromotionStrategy.hpp"
#include "OrderFactory.hpp"
#include "OrderService.hpp"
#include <map>
#include <memory>

// ============================================================
// Mock Repository — in-memory, no database needed
// ============================================================
class MockOrderRepository : public IOrderRepository {
  std::map<int, Order> orders;
  std::map<int, std::vector<OrderItem>> itemStore;
  int nextOrderId = 1;
  int nextItemId = 1;

public:
  int createOrder(Order& order) override {
    int id = nextOrderId++;
    order.setId(id);
    orders.insert({id, order});
    for (auto& item : order.getItems()) {
      item.setId(nextItemId++);
      item.setOrderId(id);
      itemStore[id].push_back(item);
    }
    return id;
  }

  void addOrderItem(int orderId, OrderItem& item) override {
    item.setId(nextItemId++);
    item.setOrderId(orderId);
    itemStore[orderId].push_back(item);
  }

  std::unique_ptr<Order> findById(int orderId) override {
    auto it = orders.find(orderId);
    if (it == orders.end()) return nullptr;
    auto order = std::make_unique<Order>(
      it->second.getId(),
      it->second.getCustomerName(),
      it->second.getStatus(),
      it->second.getTotalAmount(),
      it->second.getCreatedAt(),
      it->second.getUpdatedAt()
    );
    if (itemStore.count(orderId)) {
      for (auto& item : itemStore[orderId]) {
        order->getItems().push_back(item);
      }
    }
    return order;
  }

  std::vector<std::unique_ptr<Order>> findAll() override {
    std::vector<std::unique_ptr<Order>> result;
    for (auto& [id, o] : orders) {
      auto order = findById(id);
      if (order) result.push_back(std::move(order));
    }
    return result;
  }

  std::vector<std::unique_ptr<Order>> findByStatus(OrderStatus status) override {
    std::vector<std::unique_ptr<Order>> result;
    for (auto& [id, o] : orders) {
      if (o.getStatus() == status) {
        auto order = findById(id);
        if (order) result.push_back(std::move(order));
      }
    }
    return result;
  }

  bool updateStatus(int orderId, OrderStatus status) override {
    auto it = orders.find(orderId);
    if (it == orders.end()) return false;
    it->second.setStatus(status);
    return true;
  }

  std::vector<OrderItem> findItemsByOrderId(int orderId) override {
    if (itemStore.count(orderId)) return itemStore[orderId];
    return {};
  }

  std::vector<int> findOrderIdsByStatus(OrderStatus status) override {
    std::vector<int> ids;
    for (auto& [id, o] : orders) {
      if (o.getStatus() == status) ids.push_back(id);
    }
    return ids;
  }

  std::vector<int> findEligibleForPromotion(int /*minAgeSeconds*/) override {
    // In mock, return all PENDING orders (no time check)
    return findOrderIdsByStatus(PENDING);
  }
};

// ============================================================
// Test Observer — records calls for verification
// ============================================================
class TestObserver : public IOrderObserver {
public:
  struct Event {
    int orderId;
    OrderStatus oldStatus;
    OrderStatus newStatus;
  };
  std::vector<Event> events;

  void onStatusChanged(int orderId, OrderStatus oldStatus, OrderStatus newStatus) override {
    events.push_back({orderId, oldStatus, newStatus});
  }
};

// ============================================================
// Test Promotion Strategy — returns a fixed list
// ============================================================
class FixedPromotionStrategy : public IPromotionStrategy {
  std::vector<int> ids;
public:
  explicit FixedPromotionStrategy(std::vector<int> ids) : ids(std::move(ids)) {}

  std::vector<int> findEligibleOrders(IOrderRepository& /*repo*/) override {
    return ids;
  }
};

// ============================================================
// Tests
// ============================================================

static int testStateMachineValidTransitions() {
  TestHelper t("State Machine - Valid Transitions");

  t.assert_true(isValidTransition(PENDING, PROCESSING), "PENDING -> PROCESSING valid");
  t.assert_true(isValidTransition(PENDING, CANCELLED), "PENDING -> CANCELLED valid");
  t.assert_true(isValidTransition(PROCESSING, SHIPPED), "PROCESSING -> SHIPPED valid");
  t.assert_true(isValidTransition(PROCESSING, CANCELLED), "PROCESSING -> CANCELLED valid");
  t.assert_true(isValidTransition(SHIPPED, DELIVERED), "SHIPPED -> DELIVERED valid");

  return t.printResults();
}

static int testStateMachineInvalidTransitions() {
  TestHelper t("State Machine - Invalid Transitions");

  t.assert_false(isValidTransition(DELIVERED, PENDING), "DELIVERED -> PENDING invalid");
  t.assert_false(isValidTransition(CANCELLED, PROCESSING), "CANCELLED -> PROCESSING invalid");
  t.assert_false(isValidTransition(SHIPPED, PENDING), "SHIPPED -> PENDING invalid");
  t.assert_false(isValidTransition(PENDING, SHIPPED), "PENDING -> SHIPPED invalid (skip)");
  t.assert_false(isValidTransition(PENDING, DELIVERED), "PENDING -> DELIVERED invalid (skip)");
  t.assert_false(isValidTransition(DELIVERED, DELIVERED), "DELIVERED -> DELIVERED invalid (self)");
  t.assert_false(isValidTransition(CANCELLED, CANCELLED), "CANCELLED -> CANCELLED invalid (self)");

  return t.printResults();
}

static int testDependencyInjectionWithMock() {
  TestHelper t("Dependency Injection - Mock Repository");

  auto mockRepo = std::make_unique<MockOrderRepository>();
  OrderService service(std::move(mockRepo));

  // Create order
  std::vector<OrderItem> items;
  items.push_back(OrderItem("Widget", 3, 15.00));
  int orderId = service.createOrder("TestUser", items);
  t.assert_true(orderId > 0, "Order created via mock");

  // Retrieve
  auto order = service.getOrderById(orderId);
  t.assert_true(order != nullptr, "Order retrievable from mock");
  t.assert_equal(std::string("TestUser"), order->getCustomerName(), "Customer name correct");
  t.assert_equal(1, (int)order->getItems().size(), "Has 1 item");

  // List all
  auto all = service.getAllOrders();
  t.assert_equal(1, (int)all.size(), "1 order total");

  return t.printResults();
}

static int testObserverNotification() {
  TestHelper t("Observer - Status Change Notification");

  auto mockRepo = std::make_unique<MockOrderRepository>();
  OrderService service(std::move(mockRepo));

  TestObserver observer;
  service.addObserver(&observer);

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Item", 1, 10.00));
  int orderId = service.createOrder("Alice", items);

  // Update status -> observer should be notified
  service.updateOrderStatus(orderId, PROCESSING);
  t.assert_equal(1, (int)observer.events.size(), "Observer notified once");
  t.assert_equal(orderId, observer.events[0].orderId, "Correct order ID");
  t.assert_equal(std::string("PENDING"), orderStatusToString(observer.events[0].oldStatus), "Old status PENDING");
  t.assert_equal(std::string("PROCESSING"), orderStatusToString(observer.events[0].newStatus), "New status PROCESSING");

  return t.printResults();
}

static int testMultipleObservers() {
  TestHelper t("Observer - Multiple Observers");

  auto mockRepo = std::make_unique<MockOrderRepository>();
  OrderService service(std::move(mockRepo));

  TestObserver obs1, obs2;
  service.addObserver(&obs1);
  service.addObserver(&obs2);

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Item", 1, 10.00));
  int orderId = service.createOrder("Bob", items);
  service.updateOrderStatus(orderId, PROCESSING);

  t.assert_equal(1, (int)obs1.events.size(), "Observer 1 notified");
  t.assert_equal(1, (int)obs2.events.size(), "Observer 2 notified");

  // Remove observer 1, update again
  service.removeObserver(&obs1);
  service.updateOrderStatus(orderId, SHIPPED);

  t.assert_equal(1, (int)obs1.events.size(), "Observer 1 NOT notified after removal");
  t.assert_equal(2, (int)obs2.events.size(), "Observer 2 notified again");

  return t.printResults();
}

static int testObserverOnCancel() {
  TestHelper t("Observer - Cancel Notification");

  auto mockRepo = std::make_unique<MockOrderRepository>();
  OrderService service(std::move(mockRepo));

  TestObserver observer;
  service.addObserver(&observer);

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Item", 1, 10.00));
  int orderId = service.createOrder("Charlie", items);

  service.cancelOrder(orderId);
  t.assert_equal(1, (int)observer.events.size(), "Observer notified on cancel");
  t.assert_equal(std::string("CANCELLED"), orderStatusToString(observer.events[0].newStatus),
                 "New status is CANCELLED");

  return t.printResults();
}

static int testFactoryValidation() {
  TestHelper t("Factory - Input Validation");

  // Empty customer name
  bool threw = false;
  try {
    std::vector<OrderItem> items;
    items.push_back(OrderItem("Widget", 1, 10.00));
    OrderFactory::createOrder("", items);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  t.assert_true(threw, "Factory throws on empty customer name");

  // Empty items
  threw = false;
  try {
    std::vector<OrderItem> empty;
    OrderFactory::createOrder("Alice", empty);
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  t.assert_true(threw, "Factory throws on empty items");

  // Valid input
  threw = false;
  try {
    std::vector<OrderItem> items;
    items.push_back(OrderItem("Widget", 2, 15.00));
    Order order = OrderFactory::createOrder("Alice", items);
    t.assert_equal(std::string("Alice"), order.getCustomerName(), "Factory creates order correctly");
    t.assert_equal(30.00, order.getTotalAmount(), "Factory calculates total");
  } catch (...) {
    threw = true;
  }
  t.assert_false(threw, "Factory does not throw on valid input");

  return t.printResults();
}

static int testStateEnforcementViaService() {
  TestHelper t("State Machine - Enforcement Via Service");

  auto mockRepo = std::make_unique<MockOrderRepository>();
  OrderService service(std::move(mockRepo));

  std::vector<OrderItem> items;
  items.push_back(OrderItem("Item", 1, 10.00));
  int orderId = service.createOrder("Dave", items);

  // Valid: PENDING -> PROCESSING -> SHIPPED -> DELIVERED
  t.assert_true(service.updateOrderStatus(orderId, PROCESSING), "PENDING -> PROCESSING");
  t.assert_true(service.updateOrderStatus(orderId, SHIPPED), "PROCESSING -> SHIPPED");
  t.assert_true(service.updateOrderStatus(orderId, DELIVERED), "SHIPPED -> DELIVERED");

  // Invalid: DELIVERED is terminal
  t.assert_false(service.updateOrderStatus(orderId, PENDING), "DELIVERED -> PENDING rejected");
  t.assert_false(service.updateOrderStatus(orderId, PROCESSING), "DELIVERED -> PROCESSING rejected");

  return t.printResults();
}

int main() {
  int totalFailures = 0;

  totalFailures += testStateMachineValidTransitions();
  totalFailures += testStateMachineInvalidTransitions();
  totalFailures += testDependencyInjectionWithMock();
  totalFailures += testObserverNotification();
  totalFailures += testMultipleObservers();
  totalFailures += testObserverOnCancel();
  totalFailures += testFactoryValidation();
  totalFailures += testStateEnforcementViaService();

  std::cout << "====================================" << std::endl;
  if (totalFailures == 0) {
    std::cout << "  ALL DESIGN PATTERN TESTS PASSED" << std::endl;
  } else {
    std::cout << "  " << totalFailures << " TOTAL FAILURE(S)" << std::endl;
  }
  std::cout << "====================================" << std::endl;

  return totalFailures;
}
