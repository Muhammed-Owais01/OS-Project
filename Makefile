# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Iinclude

# Executable names
SERVER = server
CLIENT = client

# Source files
SERVER_SRCS = src/server.cpp \
              src/socket.cpp \
              src/connection_handler.cpp \
              src/message_queue.cpp \
              src/message_processor.cpp \
              src/thread_safe_data.cpp \
              src/lock_guard.cpp

CLIENT_SRCS = src/client.cpp \
              src/socket.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Default target
all: $(SERVER) $(CLIENT)

# Compile the server
$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile the client
$(CLIENT): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic compilation rule
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run targets
run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT) $(SERVER_OBJS) $(CLIENT_OBJS)

.PHONY: all clean run_server run_client