#include "Order.hpp"

void Order::addItem(OrderItem item) {
  items.push_back(item);
  calculateTotal();
}

void Order::calculateTotal() {
  double total = 0.0;
  for (auto& item : items) {
    total += item.getSubtotal();
  }
  totalAmount = total;
}
