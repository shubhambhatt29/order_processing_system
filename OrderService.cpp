#include "OrderService.hpp"
#include "OrderRepository.hpp"
#include "OrderFactory.hpp"
#include <iostream>

OrderService::OrderService()
  : repository(std::make_unique<OrderRepository>()),
    promotionStrategy(std::make_unique<TimeBasedPromotion>()) {}

OrderService::OrderService(std::unique_ptr<IOrderRepository> repo)
  : repository(std::move(repo)),
    promotionStrategy(std::make_unique<TimeBasedPromotion>()) {}

void OrderService::addObserver(IOrderObserver* observer) {
  observers.push_back(observer);
}

void OrderService::removeObserver(IOrderObserver* observer) {
  observers.erase(
    std::remove(observers.begin(), observers.end(), observer),
    observers.end()
  );
}

void OrderService::notifyObservers(int orderId, OrderStatus oldStatus, OrderStatus newStatus) {
  for (auto* observer : observers) {
    observer->onStatusChanged(orderId, oldStatus, newStatus);
  }
}

int OrderService::createOrder(std::string customerName, std::vector<OrderItem> items) {
  try {
    Order order = OrderFactory::createOrder(customerName, items);

    int orderId = repository->createOrder(order);
    if (orderId > 0) {
      std::cout << "Order #" << orderId << " created successfully." << std::endl;
    }
    return orderId;
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << std::endl;
    return -1;
  }
}

std::unique_ptr<Order> OrderService::getOrderById(int orderId) {
  return repository->findById(orderId);
}

std::vector<std::unique_ptr<Order>> OrderService::getAllOrders() {
  return repository->findAll();
}

std::vector<std::unique_ptr<Order>> OrderService::getOrdersByStatus(OrderStatus status) {
  return repository->findByStatus(status);
}

bool OrderService::cancelOrder(int orderId) {
  auto order = repository->findById(orderId);
  if (order == nullptr) {
    std::cerr << "Order #" << orderId << " not found." << std::endl;
    return false;
  }

  if (order->getStatus() != PENDING) {
    std::cerr << "Order #" << orderId << " cannot be cancelled. "
              << "Only PENDING orders can be cancelled. "
              << "Current status: " << orderStatusToString(order->getStatus())
              << std::endl;
    return false;
  }

  bool result = repository->updateStatus(orderId, CANCELLED);
  if (result) {
    std::cout << "Order #" << orderId << " has been cancelled." << std::endl;
    notifyObservers(orderId, PENDING, CANCELLED);
  }
  return result;
}

bool OrderService::updateOrderStatus(int orderId, OrderStatus newStatus) {
  auto order = repository->findById(orderId);
  if (order == nullptr) {
    std::cerr << "Order #" << orderId << " not found." << std::endl;
    return false;
  }

  OrderStatus currentStatus = order->getStatus();

  if (!isValidTransition(currentStatus, newStatus)) {
    std::cerr << "Invalid status transition: "
              << orderStatusToString(currentStatus) << " -> "
              << orderStatusToString(newStatus) << std::endl;
    return false;
  }

  bool result = repository->updateStatus(orderId, newStatus);
  if (result) {
    notifyObservers(orderId, currentStatus, newStatus);
  }
  return result;
}

void OrderService::promotePendingOrders() {
  auto eligibleIds = promotionStrategy->findEligibleOrders(*repository);
  OrderStatus target = promotionStrategy->targetStatus();
  for (int id : eligibleIds) {
    if (repository->updateStatus(id, target)) {
      notifyObservers(id, PENDING, target);
      std::cout << "  Order #" << id << " promoted to "
                << orderStatusToString(target) << "." << std::endl;
    }
  }
  if (eligibleIds.empty()) {
    std::cout << "  No eligible orders to promote." << std::endl;
  }
}
