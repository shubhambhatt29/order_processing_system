#pragma once

#include <vector>
#include <string>
#include "Order.hpp"
#include "OrderRepository.hpp"

class OrderService {
private:
  OrderRepository* repository;

public:
  OrderService();
  ~OrderService();

  int createOrder(std::string customerName, std::vector<OrderItem> items);
  Order* getOrderById(int orderId);
  std::vector<Order*> getAllOrders();
  std::vector<Order*> getOrdersByStatus(OrderStatus status);
  bool cancelOrder(int orderId);
  bool updateOrderStatus(int orderId, OrderStatus status);
  void promotePendingOrders();
};
