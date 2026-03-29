#include "OrderService.hpp"
#include <iostream>

OrderService::OrderService() {
  repository = new OrderRepository();
}

OrderService::~OrderService() {
  delete repository;
}

int OrderService::createOrder(std::string customerName, std::vector<OrderItem> items) {
  if (customerName.empty()) {
    std::cerr << "Customer name cannot be empty." << std::endl;
    return -1;
  }
  if (items.empty()) {
    std::cerr << "Order must have at least one item." << std::endl;
    return -1;
  }

  Order order(customerName);
  for (auto& item : items) {
    order.addItem(item);
  }

  int orderId = repository->createOrder(order);
  if (orderId > 0) {
    std::cout << "Order #" << orderId << " created successfully." << std::endl;
  }
  return orderId;
}

Order* OrderService::getOrderById(int orderId) {
  return repository->findById(orderId);
}

std::vector<Order*> OrderService::getAllOrders() {
  return repository->findAll();
}

std::vector<Order*> OrderService::getOrdersByStatus(OrderStatus status) {
  return repository->findByStatus(status);
}

bool OrderService::cancelOrder(int orderId) {
  Order* order = repository->findById(orderId);
  if (order == nullptr) {
    std::cerr << "Order #" << orderId << " not found." << std::endl;
    return false;
  }

  if (order->getStatus() != PENDING) {
    std::cerr << "Order #" << orderId << " cannot be cancelled. "
              << "Only PENDING orders can be cancelled. "
              << "Current status: " << orderStatusToString(order->getStatus())
              << std::endl;
    delete order;
    return false;
  }

  bool result = repository->updateStatus(orderId, CANCELLED);
  if (result) {
    std::cout << "Order #" << orderId << " has been cancelled." << std::endl;
  }
  delete order;
  return result;
}

bool OrderService::updateOrderStatus(int orderId, OrderStatus status) {
  return repository->updateStatus(orderId, status);
}

void OrderService::promotePendingOrders() {
  // Only promote orders that have been PENDING for at least 5 minutes (300s)
  std::vector<int> eligibleIds = repository->findEligibleForPromotion(300);
  for (int id : eligibleIds) {
    repository->updateStatus(id, PROCESSING);
    std::cout << "  Order #" << id << " promoted to PROCESSING." << std::endl;
  }
  if (eligibleIds.empty()) {
    std::cout << "  No eligible orders to promote." << std::endl;
  }
}
