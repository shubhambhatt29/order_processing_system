#pragma once

#include <string>
#include <vector>
#include "OrderStatus.hpp"
#include "OrderItem.hpp"

class Order {
private:
  int id;
  std::string customerName;
  OrderStatus status;
  double totalAmount;
  std::string createdAt;
  std::string updatedAt;
  std::vector<OrderItem> items;

public:
  Order(std::string customerName)
    : id(0), customerName(customerName), status(PENDING), totalAmount(0.0) {}

  Order(int id, std::string customerName, OrderStatus status,
        double totalAmount, std::string createdAt, std::string updatedAt)
    : id(id), customerName(customerName), status(status),
      totalAmount(totalAmount), createdAt(createdAt), updatedAt(updatedAt) {}

  int getId() const { return id; }
  std::string getCustomerName() const { return customerName; }
  OrderStatus getStatus() const { return status; }
  double getTotalAmount() const { return totalAmount; }
  std::string getCreatedAt() const { return createdAt; }
  std::string getUpdatedAt() const { return updatedAt; }
  std::vector<OrderItem>& getItems() { return items; }
  const std::vector<OrderItem>& getItems() const { return items; }

  void setId(int id) { this->id = id; }
  void setStatus(OrderStatus status) { this->status = status; }
  void setTotalAmount(double amount) { this->totalAmount = amount; }

  void addItem(OrderItem item);
  void calculateTotal();
};
