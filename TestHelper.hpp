#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <vector>

class TestHelper {
private:
  int passed;
  int failed;
  std::string suiteName;

public:
  TestHelper(std::string suiteName) : passed(0), failed(0), suiteName(suiteName) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Suite: " << suiteName << std::endl;
    std::cout << "========================================" << std::endl;
  }

  void assert_true(bool condition, std::string testName) {
    if (condition) {
      std::cout << "  [PASS] " << testName << std::endl;
      passed++;
    } else {
      std::cout << "  [FAIL] " << testName << std::endl;
      failed++;
    }
  }

  void assert_false(bool condition, std::string testName) {
    assert_true(!condition, testName);
  }

  void assert_equal(int expected, int actual, std::string testName) {
    if (expected == actual) {
      std::cout << "  [PASS] " << testName << std::endl;
      passed++;
    } else {
      std::cout << "  [FAIL] " << testName
                << " (expected: " << expected << ", got: " << actual << ")"
                << std::endl;
      failed++;
    }
  }

  void assert_equal(double expected, double actual, std::string testName) {
    double diff = expected - actual;
    if (diff < 0) diff = -diff;
    if (diff < 0.01) {
      std::cout << "  [PASS] " << testName << std::endl;
      passed++;
    } else {
      std::cout << "  [FAIL] " << testName
                << " (expected: " << expected << ", got: " << actual << ")"
                << std::endl;
      failed++;
    }
  }

  void assert_equal(std::string expected, std::string actual, std::string testName) {
    if (expected == actual) {
      std::cout << "  [PASS] " << testName << std::endl;
      passed++;
    } else {
      std::cout << "  [FAIL] " << testName
                << " (expected: \"" << expected << "\", got: \"" << actual << "\")"
                << std::endl;
      failed++;
    }
  }

  void assert_not_null(void* ptr, std::string testName) {
    assert_true(ptr != nullptr, testName);
  }

  void assert_null(void* ptr, std::string testName) {
    assert_true(ptr == nullptr, testName);
  }

  int printResults() {
    std::cout << "----------------------------------------" << std::endl;
    std::cout << "  " << suiteName << " Results: "
              << passed << " passed, " << failed << " failed, "
              << (passed + failed) << " total" << std::endl;
    std::cout << "========================================\n" << std::endl;
    return failed;
  }
};
