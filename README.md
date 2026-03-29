# E-commerce Order Processing System

## Objective
The goal of this assignment is to assess your ability to design, implement, and test a backend system in Java (or any language you prefer) that handles order processing efficiently.

## Scenario
You have been hired to build the backend for an E-commerce Order Processing System. The system should allow customers to place orders, track their status, and support basic order operations.

## Requirements

### 1. Core Features
- **Create an order**: Customers should be able to place an order with multiple items.
- **Retrieve order details**: The system should allow fetching order details by order ID.
- **Update order status**: 
  - Manage statuses: `PENDING`, `PROCESSING`, `SHIPPED`, and `DELIVERED`.
  - A background job should automatically update `PENDING` orders to `PROCESSING` every 5 minutes.
- **List all orders**: Retrieve all orders, optionally filtered by status.
- **Cancel an order**: Customers should be able to cancel an order, but ONLY if it is still in `PENDING` status.

## Coding Assignment Rules
Extensive use of AI assisted coding tools (e.g., Cursor AI, ChatGPT) for every aspect of the coding assignment is heavily encouraged. 

However, you need to provide an explanation regarding:
- What you used the AI tools for.
- What issues were found during the process.
- How you corrected those issues.

## Getting Started
1. Cut a new branch from `master` to begin your implementation.
2. Structure your codebase following best practices for production-ready backend systems.
3. Keep track of your AI usage and write up your explanation accordingly (you can define it in a separate markdown report or append it to the project's documentation).
