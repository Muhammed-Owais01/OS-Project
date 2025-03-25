# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -Iinclude

# Folders
SRC_DIR = src
BUILD_DIR = build

# Executable names
SERVER = server
CLIENT = client

# Source files
SERVER_SRCS = $(SRC_DIR)/server.cpp $(SRC_DIR)/socket.cpp
CLIENT_SRCS = $(SRC_DIR)/client.cpp $(SRC_DIR)/socket.cpp

# Object files
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Default target
all: $(SERVER) $(CLIENT)

# Compile the server
$(SERVER): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(SERVER_OBJS)

# Compile the client
$(CLIENT): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(CLIENT_OBJS)

# Compile object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run the server
run_server: $(SERVER)
	./$(SERVER)

# Run the client
run_client: $(CLIENT)
	./$(CLIENT)

# Clean compiled files
clean:
	rm -f $(SERVER) $(CLIENT) $(SERVER_OBJS) $(CLIENT_OBJS)
