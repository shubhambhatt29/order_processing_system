# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

Requires MySQL (via `mysql_config`) and C++17. On macOS, uses Homebrew paths for zstd and openssl.

```bash
make              # build main binary (order_processing)
make tests        # build all 5 test binaries
make run-tests    # build and run all tests sequentially
make clean        # remove all binaries
```

Build/run a single test:
```bash
make test_order_creation && ./test_order_creation
make test_order_status && ./test_order_status
make test_order_query && ./test_order_query
make test_concurrency && ./test_concurrency
make test_patterns && ./test_patterns   # no DB required — uses mock repository
```

## Database Setup

MySQL must be running locally. Run `schema.sql` to create the `order_processing` database with `orders` and `order_items` tables. Connection defaults: `localhost:3306`, user `root`, no password. Integration tests use database `order_processing_test`.

## Architecture

Flat-structure C++17 backend using the MySQL C API directly.

**Layered design with interfaces (DIP):**
- **Domain models** (`Order`, `OrderItem`, `OrderStatus`) — `OrderStatus` includes a state machine transition map via `isValidTransition()` enforcing valid status flows (PENDING→PROCESSING→SHIPPED→DELIVERED, with CANCELLED as a terminal state reachable from PENDING/PROCESSING).
- **Repository interface** (`IOrderRepository`) — pure virtual interface. `OrderRepository` is the MySQL implementation. `TestPatterns.cpp` provides `MockOrderRepository` for DB-free unit testing.
- **Service** (`OrderService`) — accepts `IOrderRepository` via constructor injection (DI). Enforces state machine on `updateOrderStatus()`, stricter PENDING-only rule on `cancelOrder()`. Uses `OrderFactory` for validated order creation.
- **Observer** (`IOrderObserver`) — `OrderService` notifies registered observers on every status change. Concrete: `LoggingObserver`, `NotificationObserver`.
- **Strategy** (`IPromotionStrategy`) — pluggable promotion logic. `TimeBasedPromotion` (default, 300s threshold) used by `promotePendingOrders()`.
- **Background job** (`BackgroundJob`) — timer thread calling `promotePendingOrders()` on interval.
- **Database** (`DatabaseManager`) — Meyer's singleton (thread-safe), connection pool with `acquire()`/`release()`.

**Design patterns used:** Singleton, Repository, Observer, Strategy, Factory, State (transition map), Dependency Injection.

**SQL injection protection:** `createOrder()` and `addOrderItem()` use MySQL prepared statements (`mysql_stmt_*`). Read-only queries with integer-only parameters use direct queries.

**Testing:** Custom `TestHelper` framework. `TestPatterns.cpp` tests all design patterns with a mock repository (no DB). Other test files are integration tests requiring MySQL.
