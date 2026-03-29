#pragma once

#include <vector>
#include <string>
#include "Order.hpp"
#include "DatabaseManager.hpp"

class OrderRepository {
private:
  DatabaseManager* db;

public:
  OrderRepository();

  int createOrder(Order& order);
  void addOrderItem(int orderId, OrderItem& item);
  Order* findById(int orderId);
  std::vector<Order*> findAll();
  std::vector<Order*> findByStatus(OrderStatus status);
  bool updateStatus(int orderId, OrderStatus status);
  std::vector<OrderItem> findItemsByOrderId(int orderId);
  std::vector<int> findOrderIdsByStatus(OrderStatus status);
  std::vector<int> findEligibleForPromotion(int minAgeSeconds);
};
