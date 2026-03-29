CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
MYSQL_CFLAGS = $(shell mysql_config --cflags)
MYSQL_LIBS = $(shell mysql_config --libs) -L/opt/homebrew/opt/zstd/lib -L/opt/homebrew/opt/openssl@3/lib

LIB_SRCS = Order.cpp OrderRepository.cpp OrderService.cpp \
           DatabaseManager.cpp BackgroundJob.cpp

TARGET = order_processing

# Test binaries
TEST_CREATION = test_order_creation
TEST_STATUS = test_order_status
TEST_QUERY = test_order_query

all: $(TARGET)

$(TARGET): main.cpp $(LIB_SRCS)
	$(CXX) $(CXXFLAGS) $(MYSQL_CFLAGS) -o $(TARGET) main.cpp $(LIB_SRCS) $(MYSQL_LIBS)

$(TEST_CREATION): TestOrderCreation.cpp $(LIB_SRCS)
	$(CXX) $(CXXFLAGS) $(MYSQL_CFLAGS) -o $(TEST_CREATION) TestOrderCreation.cpp $(LIB_SRCS) $(MYSQL_LIBS)

$(TEST_STATUS): TestOrderStatus.cpp $(LIB_SRCS)
	$(CXX) $(CXXFLAGS) $(MYSQL_CFLAGS) -o $(TEST_STATUS) TestOrderStatus.cpp $(LIB_SRCS) $(MYSQL_LIBS)

$(TEST_QUERY): TestOrderQuery.cpp $(LIB_SRCS)
	$(CXX) $(CXXFLAGS) $(MYSQL_CFLAGS) -o $(TEST_QUERY) TestOrderQuery.cpp $(LIB_SRCS) $(MYSQL_LIBS)

tests: $(TEST_CREATION) $(TEST_STATUS) $(TEST_QUERY)

run-tests: tests
	@echo ""
	@echo "Running Order Creation Tests..."
	@./$(TEST_CREATION)
	@echo "Running Order Status Tests..."
	@./$(TEST_STATUS)
	@echo "Running Order Query Tests..."
	@./$(TEST_QUERY)

clean:
	rm -f $(TARGET) $(TEST_CREATION) $(TEST_STATUS) $(TEST_QUERY)

.PHONY: all tests run-tests clean
