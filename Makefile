# Compilers e flags
CXX = g++
CC = gcc
CXXFLAGS = -Wall -Wextra -std=c++11 -g
CFLAGS = -Wall -Wextra -std=c99 -g

CLIENT_DIR = Client
SERVER_DIR = Server
BUILD_DIR  = build

# Ficheiros
CLIENT_SOURCES = $(CLIENT_DIR)/User.cpp
SERVER_SOURCES = $(SERVER_DIR)/UDP.c

CLIENT_OBJS = $(BUILD_DIR)/User.o
SERVER_OBJS = $(BUILD_DIR)/UDP.o

CLIENT_EXEC = $(BUILD_DIR)/client
SERVER_EXEC = $(BUILD_DIR)/server

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CLIENT_EXEC): $(CLIENT_OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Client built: $@"

$(SERVER_EXEC): $(SERVER_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Server built: $@"

$(BUILD_DIR)/User.o: $(CLIENT_DIR)/User.cpp $(CLIENT_DIR)/User.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/UDP.o: $(SERVER_DIR)/UDP.c $(SERVER_DIR)/UDP.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(BUILD_DIR)

# run-client: $(CLIENT_EXEC)
#	./$(CLIENT_EXEC)

# run-server: $(SERVER_EXEC)
#	./$(SERVER_EXEC)
