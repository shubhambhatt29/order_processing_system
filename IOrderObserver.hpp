#pragma once

#include <iostream>
#include <string>
#include "OrderStatus.hpp"

class IOrderObserver {
public:
  virtual ~IOrderObserver() = default;
  virtual void onStatusChanged(int orderId, OrderStatus oldStatus, OrderStatus newStatus) = 0;
};

class LoggingObserver : public IOrderObserver {
public:
  void onStatusChanged(int orderId, OrderStatus oldStatus, OrderStatus newStatus) override {
    std::cout << "[LOG] Order #" << orderId << " status changed: "
              << orderStatusToString(oldStatus) << " -> "
              << orderStatusToString(newStatus) << std::endl;
  }
};

class NotificationObserver : public IOrderObserver {
public:
  void onStatusChanged(int orderId, OrderStatus /*oldStatus*/, OrderStatus newStatus) override {
    if (newStatus == SHIPPED) {
      std::cout << "[NOTIFY] Order #" << orderId
                << ": Your order has been shipped!" << std::endl;
    } else if (newStatus == DELIVERED) {
      std::cout << "[NOTIFY] Order #" << orderId
                << ": Your order has been delivered!" << std::endl;
    }
  }
};
