#pragma once

#include <vector>
#include "IOrderRepository.hpp"

class IPromotionStrategy {
public:
  virtual ~IPromotionStrategy() = default;
  virtual std::vector<int> findEligibleOrders(IOrderRepository& repo) = 0;
  virtual OrderStatus targetStatus() const { return PROCESSING; }
};

class TimeBasedPromotion : public IPromotionStrategy {
  int minAgeSeconds;

public:
  explicit TimeBasedPromotion(int seconds = 300) : minAgeSeconds(seconds) {}

  std::vector<int> findEligibleOrders(IOrderRepository& repo) override {
    return repo.findEligibleForPromotion(minAgeSeconds);
  }
};
