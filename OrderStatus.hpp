#pragma once

#include <string>

enum OrderStatus {
  PENDING,
  PROCESSING,
  SHIPPED,
  DELIVERED,
  CANCELLED
};

inline std::string orderStatusToString(OrderStatus status) {
  switch (status) {
    case PENDING: return "PENDING";
    case PROCESSING: return "PROCESSING";
    case SHIPPED: return "SHIPPED";
    case DELIVERED: return "DELIVERED";
    case CANCELLED: return "CANCELLED";
    default: return "UNKNOWN";
  }
}

inline OrderStatus stringToOrderStatus(const std::string& status) {
  if (status == "PENDING") return PENDING;
  if (status == "PROCESSING") return PROCESSING;
  if (status == "SHIPPED") return SHIPPED;
  if (status == "DELIVERED") return DELIVERED;
  if (status == "CANCELLED") return CANCELLED;
  return PENDING;
}
