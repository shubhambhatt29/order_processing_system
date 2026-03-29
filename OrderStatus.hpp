#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

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

inline const std::unordered_map<OrderStatus, std::unordered_set<OrderStatus>>&
getAllowedTransitions() {
  static const std::unordered_map<OrderStatus, std::unordered_set<OrderStatus>> transitions = {
    { PENDING,    { PROCESSING, CANCELLED } },
    { PROCESSING, { SHIPPED, CANCELLED } },
    { SHIPPED,    { DELIVERED } },
    { DELIVERED,  { } },
    { CANCELLED,  { } }
  };
  return transitions;
}

inline bool isValidTransition(OrderStatus from, OrderStatus to) {
  auto& transitions = getAllowedTransitions();
  auto it = transitions.find(from);
  if (it == transitions.end()) return false;
  return it->second.count(to) > 0;
}
