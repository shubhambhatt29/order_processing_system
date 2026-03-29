#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include "Order.hpp"
#include "OrderItem.hpp"

class OrderFactory {
public:
  static Order createOrder(const std::string& customerName,
                           const std::vector<OrderItem>& items) {
    if (customerName.empty()) {
      throw std::invalid_argument("Customer name cannot be empty");
    }
    if (items.empty()) {
      throw std::invalid_argument("Order must have at least one item");
    }

    Order order(customerName);
    for (const auto& item : items) {
      order.addItem(item);
    }
    return order;
  }
};
