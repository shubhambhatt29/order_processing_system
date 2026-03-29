#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#include "DatabaseManager.hpp"
#include "OrderService.hpp"
#include "BackgroundJob.hpp"

class Demo {
public:
  static void printOrder(Order* order) {
    std::cout << "------------------------------" << std::endl;
    std::cout << "Order #" << order->getId() << std::endl;
    std::cout << "  Customer: " << order->getCustomerName() << std::endl;
    std::cout << "  Status:   " << orderStatusToString(order->getStatus()) << std::endl;
    std::cout << "  Total:    $" << std::fixed << std::setprecision(2)
              << order->getTotalAmount() << std::endl;
    std::cout << "  Created:  " << order->getCreatedAt() << std::endl;
    std::cout << "  Updated:  " << order->getUpdatedAt() << std::endl;
    std::cout << "  Items:" << std::endl;
    for (auto& item : order->getItems()) {
      std::cout << "    - " << item.getProductName()
                << " (x" << item.getQuantity() << ")"
                << " @ $" << std::fixed << std::setprecision(2) << item.getPrice()
                << " = $" << std::fixed << std::setprecision(2) << item.getSubtotal()
                << std::endl;
    }
    std::cout << "------------------------------" << std::endl;
  }

  static void printMenu() {
    std::cout << "\n=== Order Processing System ===" << std::endl;
    std::cout << "1. Create Order" << std::endl;
    std::cout << "2. Get Order by ID" << std::endl;
    std::cout << "3. List All Orders" << std::endl;
    std::cout << "4. List Orders by Status" << std::endl;
    std::cout << "5. Update Order Status" << std::endl;
    std::cout << "6. Cancel Order" << std::endl;
    std::cout << "7. Exit" << std::endl;
    std::cout << "> ";
  }

  static void handleCreateOrder(OrderService* service) {
    std::string customerName;
    std::cout << "Enter customer name: ";
    std::getline(std::cin, customerName);

    std::vector<OrderItem> items;
    std::string addMore = "y";

    while (addMore == "y" || addMore == "Y") {
      std::string productName;
      int quantity;
      double price;

      std::cout << "Enter product name: ";
      std::getline(std::cin, productName);

      std::cout << "Enter quantity: ";
      std::cin >> quantity;

      std::cout << "Enter price: ";
      std::cin >> price;
      std::cin.ignore();

      items.push_back(OrderItem(productName, quantity, price));

      std::cout << "Add another item? (y/n): ";
      std::getline(std::cin, addMore);
    }

    service->createOrder(customerName, items);
  }

  static void handleGetOrder(OrderService* service) {
    int orderId;
    std::cout << "Enter order ID: ";
    std::cin >> orderId;
    std::cin.ignore();

    Order* order = service->getOrderById(orderId);
    if (order != nullptr) {
      printOrder(order);
      delete order;
    } else {
      std::cout << "Order not found." << std::endl;
    }
  }

  static void handleListAll(OrderService* service) {
    std::vector<Order*> orders = service->getAllOrders();
    if (orders.empty()) {
      std::cout << "No orders found." << std::endl;
      return;
    }

    std::cout << "\nFound " << orders.size() << " order(s):" << std::endl;
    for (auto* order : orders) {
      printOrder(order);
      delete order;
    }
  }

  static void handleListByStatus(OrderService* service) {
    std::cout << "Select status:" << std::endl;
    std::cout << "  1. PENDING" << std::endl;
    std::cout << "  2. PROCESSING" << std::endl;
    std::cout << "  3. SHIPPED" << std::endl;
    std::cout << "  4. DELIVERED" << std::endl;
    std::cout << "  5. CANCELLED" << std::endl;
    std::cout << "> ";

    int choice;
    std::cin >> choice;
    std::cin.ignore();

    OrderStatus status;
    switch (choice) {
      case 1: status = PENDING; break;
      case 2: status = PROCESSING; break;
      case 3: status = SHIPPED; break;
      case 4: status = DELIVERED; break;
      case 5: status = CANCELLED; break;
      default:
        std::cout << "Invalid choice." << std::endl;
        return;
    }

    std::vector<Order*> orders = service->getOrdersByStatus(status);
    if (orders.empty()) {
      std::cout << "No orders found with status "
                << orderStatusToString(status) << "." << std::endl;
      return;
    }

    std::cout << "\nFound " << orders.size() << " order(s):" << std::endl;
    for (auto* order : orders) {
      printOrder(order);
      delete order;
    }
  }

  static void handleUpdateStatus(OrderService* service) {
    int orderId;
    std::cout << "Enter order ID: ";
    std::cin >> orderId;
    std::cin.ignore();

    std::cout << "Select new status:" << std::endl;
    std::cout << "  1. PROCESSING" << std::endl;
    std::cout << "  2. SHIPPED" << std::endl;
    std::cout << "  3. DELIVERED" << std::endl;
    std::cout << "> ";

    int choice;
    std::cin >> choice;
    std::cin.ignore();

    OrderStatus status;
    switch (choice) {
      case 1: status = PROCESSING; break;
      case 2: status = SHIPPED; break;
      case 3: status = DELIVERED; break;
      default:
        std::cout << "Invalid choice." << std::endl;
        return;
    }

    if (service->updateOrderStatus(orderId, status)) {
      std::cout << "Order #" << orderId << " updated to "
                << orderStatusToString(status) << "." << std::endl;
    } else {
      std::cout << "Failed to update order status." << std::endl;
    }
  }

  static void handleCancelOrder(OrderService* service) {
    int orderId;
    std::cout << "Enter order ID to cancel: ";
    std::cin >> orderId;
    std::cin.ignore();

    service->cancelOrder(orderId);
  }

  static void run() {
    // Database connection
    DatabaseManager* db = DatabaseManager::getInstance();
    if (!db->connect("localhost", "root", "", "order_processing")) {
      std::cerr << "Failed to connect to database. Exiting." << std::endl;
      return;
    }
    db->initializeSchema();

    // Services
    OrderService* service = new OrderService();

    // Background job: sweeps every 60s, but only promotes orders aged >= 5 min
    BackgroundJob* bgJob = new BackgroundJob(service, 60);
    bgJob->start();

    // Interactive menu loop
    int choice = 0;
    while (choice != 7) {
      printMenu();
      std::cin >> choice;
      std::cin.ignore();

      switch (choice) {
        case 1: handleCreateOrder(service); break;
        case 2: handleGetOrder(service); break;
        case 3: handleListAll(service); break;
        case 4: handleListByStatus(service); break;
        case 5: handleUpdateStatus(service); break;
        case 6: handleCancelOrder(service); break;
        case 7:
          std::cout << "Shutting down..." << std::endl;
          break;
        default:
          std::cout << "Invalid option. Try again." << std::endl;
      }
    }

    // Cleanup
    bgJob->stop();
    delete bgJob;
    delete service;
    db->disconnect();
  }
};

int main() {
  Demo::run();
  return 0;
}
