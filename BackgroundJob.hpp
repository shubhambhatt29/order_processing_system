#pragma once

#include <thread>
#include <atomic>
#include <chrono>
#include "OrderService.hpp"

class BackgroundJob {
private:
  OrderService* orderService;
  std::thread workerThread;
  std::atomic<bool> running;
  int intervalSeconds;

  void run();

public:
  BackgroundJob(OrderService* service, int intervalSeconds = 300);
  ~BackgroundJob();

  void start();
  void stop();
  bool isRunning() const;
};
