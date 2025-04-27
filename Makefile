# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Iinclude
DEPFLAGS = -M

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
              src/lock_guard.cpp \
              src/http_parser.cpp

CLIENT_SRCS = src/client.cpp \
              src/socket.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Dependency files
SERVER_DEPS = $(SERVER_OBJS:.o=.d)
CLIENT_DEPS = $(CLIENT_OBJS:.o=.d)

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

# Dependency generation
%.d: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $< > $@

# Run targets
run_server: $(SERVER)
	./$(SERVER)

run_client: $(CLIENT)
	./$(CLIENT)

clean:
	rm -f $(SERVER) $(CLIENT) $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_DEPS) $(CLIENT_DEPS)

# Copy data folder
data:
	cp -r data .

# Include dependencies
-include $(SERVER_DEPS) $(CLIENT_DEPS)

.PHONY: all clean run_server run_client data