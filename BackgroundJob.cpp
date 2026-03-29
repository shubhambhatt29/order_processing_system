#include "BackgroundJob.hpp"
#include <iostream>

BackgroundJob::BackgroundJob(OrderService* service, int intervalSeconds)
  : orderService(service), running(false), intervalSeconds(intervalSeconds) {}

BackgroundJob::~BackgroundJob() {
  stop();
}

void BackgroundJob::start() {
  if (running) return;

  running = true;
  workerThread = std::thread(&BackgroundJob::run, this);
  std::cout << "Background job started (interval: "
            << intervalSeconds << "s)." << std::endl;
}

void BackgroundJob::stop() {
  running = false;
  if (workerThread.joinable()) {
    workerThread.join();
  }
  std::cout << "Background job stopped." << std::endl;
}

bool BackgroundJob::isRunning() const {
  return running;
}

void BackgroundJob::run() {
  while (running) {
    // Sleep in small increments so we can stop promptly
    for (int i = 0; i < intervalSeconds && running; ++i) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (running) {
      std::cout << "\n[BackgroundJob] Promoting PENDING orders to PROCESSING..."
                << std::endl;
      orderService->promotePendingOrders();
      std::cout << "[BackgroundJob] Done.\n> " << std::flush;
    }
  }
}
