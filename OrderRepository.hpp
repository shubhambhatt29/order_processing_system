#pragma once

#include <vector>
#include <memory>
#include <string>
#include "IOrderRepository.hpp"
#include "DatabaseManager.hpp"

class OrderRepository : public IOrderRepository {
private:
  DatabaseManager* db;

public:
  OrderRepository();

  int createOrder(Order& order) override;
  void addOrderItem(int orderId, OrderItem& item) override;
  std::unique_ptr<Order> findById(int orderId) override;
  std::vector<std::unique_ptr<Order>> findAll() override;
  std::vector<std::unique_ptr<Order>> findByStatus(OrderStatus status) override;
  bool updateStatus(int orderId, OrderStatus status) override;
  std::vector<OrderItem> findItemsByOrderId(int orderId) override;
  std::vector<int> findOrderIdsByStatus(OrderStatus status) override;
  std::vector<int> findEligibleForPromotion(int minAgeSeconds) override;
};
