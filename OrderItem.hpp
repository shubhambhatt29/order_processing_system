#pragma once

#include <string>

class OrderItem {
private:
  int id;
  int orderId;
  std::string productName;
  int quantity;
  double price;

public:
  OrderItem(std::string productName, int quantity, double price)
    : id(0), orderId(0), productName(productName), quantity(quantity), price(price) {}

  OrderItem(int id, int orderId, std::string productName, int quantity, double price)
    : id(id), orderId(orderId), productName(productName), quantity(quantity), price(price) {}

  int getId() const { return id; }
  int getOrderId() const { return orderId; }
  std::string getProductName() const { return productName; }
  int getQuantity() const { return quantity; }
  double getPrice() const { return price; }
  double getSubtotal() const { return price * quantity; }

  void setId(int id) { this->id = id; }
  void setOrderId(int orderId) { this->orderId = orderId; }
};
