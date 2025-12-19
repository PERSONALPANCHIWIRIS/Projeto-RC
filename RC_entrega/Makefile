# Compilers e flags
CXX = g++
CC  = gcc
CXXFLAGS = -Wall -Wextra -std=c++11 -g
CFLAGS   = -Wall -Wextra -std=c99 -g

# Diretórios
CLIENT_DIR = Client
SERVER_DIR = Server

# Ficheiros
CLIENT_SOURCES = $(CLIENT_DIR)/User.cpp
SERVER_SOURCES = $(SERVER_DIR)/ES.c

CLIENT_OBJS = $(CLIENT_DIR)/User.o
SERVER_OBJS = $(SERVER_DIR)/ES.o

# Executáveis no diretório atual
CLIENT_EXEC = client
SERVER_EXEC = ES

all: $(CLIENT_EXEC) $(SERVER_EXEC)

# ---- Client ----
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Client built: $@"

$(CLIENT_DIR)/User.o: $(CLIENT_DIR)/User.cpp $(CLIENT_DIR)/User.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---- Server ----
$(SERVER_EXEC): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Server built: $@"

$(SERVER_DIR)/ES.o: $(SERVER_DIR)/ES.c $(SERVER_DIR)/ES.h
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Clean ----
clean:
	rm -f $(CLIENT_DIR)/*.o $(SERVER_DIR)/*.o \
	      $(CLIENT_EXEC) $(SERVER_EXEC)

db_clean:
	rm -rf Server/USERS
	rm -rf Server/EVENTS
	mkdir Server/USERS
	mkdir Server/EVENTS
	rm -rf Client/SHOW
	mkdir Client/SHOW
