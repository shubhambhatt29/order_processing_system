#pragma once

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include "Order.hpp"
#include "IOrderRepository.hpp"
#include "IOrderObserver.hpp"
#include "IPromotionStrategy.hpp"

class OrderService {
private:
  std::unique_ptr<IOrderRepository> repository;
  std::unique_ptr<IPromotionStrategy> promotionStrategy;
  std::vector<IOrderObserver*> observers;

  void notifyObservers(int orderId, OrderStatus oldStatus, OrderStatus newStatus);

public:
  OrderService();
  explicit OrderService(std::unique_ptr<IOrderRepository> repo);

  void addObserver(IOrderObserver* observer);
  void removeObserver(IOrderObserver* observer);

  int createOrder(std::string customerName, std::vector<OrderItem> items);
  std::unique_ptr<Order> getOrderById(int orderId);
  std::vector<std::unique_ptr<Order>> getAllOrders();
  std::vector<std::unique_ptr<Order>> getOrdersByStatus(OrderStatus status);
  bool cancelOrder(int orderId);
  bool updateOrderStatus(int orderId, OrderStatus status);
  void promotePendingOrders();
};
