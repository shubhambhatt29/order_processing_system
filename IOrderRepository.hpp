#pragma once

#include <vector>
#include <memory>
#include "Order.hpp"

class IOrderRepository {
public:
  virtual ~IOrderRepository() = default;

  virtual int createOrder(Order& order) = 0;
  virtual void addOrderItem(int orderId, OrderItem& item) = 0;
  virtual std::unique_ptr<Order> findById(int orderId) = 0;
  virtual std::vector<std::unique_ptr<Order>> findAll() = 0;
  virtual std::vector<std::unique_ptr<Order>> findByStatus(OrderStatus status) = 0;
  virtual bool updateStatus(int orderId, OrderStatus status) = 0;
  virtual std::vector<OrderItem> findItemsByOrderId(int orderId) = 0;
  virtual std::vector<int> findOrderIdsByStatus(OrderStatus status) = 0;
  virtual std::vector<int> findEligibleForPromotion(int minAgeSeconds) = 0;
};
