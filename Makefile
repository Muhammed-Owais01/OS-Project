# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Iinclude
LDFLAGS = -lcjson -pthread  # Added cJSON linking
DEPFLAGS = -M

# Executable names
SERVER = server
CLIENT = client

# Source files
SERVER_SRCS = src/server.c \
              src/socket.c \
              src/connection_handler.c \
              src/message_queue.c \
              src/message_processor.c \
              src/thread_safe_data.c \
              src/lock_guard.c \
              src/http_parser.c

CLIENT_SRCS = src/client.c \
              src/socket.c

# Object files
SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

# Dependency files
SERVER_DEPS = $(SERVER_OBJS:.o=.d)
CLIENT_DEPS = $(CLIENT_OBJS:.o=.d)

# Default target
all: $(SERVER) $(CLIENT)

# Compile the server
$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)  # Added LDFLAGS

# Compile the client
$(CLIENT): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)  # Added LDFLAGS

# Generic compilation rule
%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Dependency generation
%.d: %.c
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