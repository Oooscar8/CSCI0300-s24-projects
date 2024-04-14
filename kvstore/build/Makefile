CC = clang++

PROJECT_SRC = ..
CPPFLAGS += -std=c++20 -Wall -Wextra -pthread -I$(PROJECT_SRC) -g -Werror -Wno-error=unused-parameter -Wno-unused-parameter

ASANFLAG = -fsanitize=address
TSANFLAG = -fsanitize=thread

# Add sanitizers if provided
ifeq ($(ASAN),1)
CPPFLAGS += $(ASANFLAG)
endif
ifeq ($(TSAN),1)
CPPFLAGS += $(TSANFLAG)
endif

CLIENT_SRC = ../client
CLIENT_CMD_SRC = $(CLIENT_SRC)/cmd
COMMON_SRC = ../common
KVSTORE_SRC = ../kvstore
NET_SRC = ../net
REPL_SRC = ../repl
SERVER_SRC = ../server
SERVER_CMD_SRC = $(SERVER_SRC)/cmd
SHARDCONTROLLER_SRC = ../shardcontroller
SHARDCONTROLLER_CMD_SRC = $(SHARDCONTROLLER_SRC)/cmd

CLIENT_OBJ = ./client_dir
CLIENT_CMD_OBJ = $(CLIENT_OBJ)/cmd
COMMON_OBJ = ./common_dir
KVSTORE_OBJ = ./kvstore_dir
NET_OBJ = ./net_dir
REPL_OBJ = ./repl_dir
SERVER_OBJ = ./server_dir
SERVER_CMD_OBJ = $(SERVER_OBJ)/cmd
SHARDCONTROLLER_OBJ = ./shardcontroller_dir
SHARDCONTROLLER_CMD_OBJ = $(SHARDCONTROLLER_OBJ)/cmd

CLIENT_SRCS = $(wildcard $(CLIENT_SRC)/*.cpp)
CLIENT_CMD_SRCS = $(wildcard $(CLIENT_CMD_SRC)/*.cpp)
COMMON_SRCS = $(wildcard $(COMMON_SRC)/*.cpp)
KVSTORE_SRCS = $(wildcard $(KVSTORE_SRC)/*.cpp)
NET_SRCS = $(wildcard $(NET_SRC)/*.cpp)
REPL_SRCS = $(wildcard $(REPL_SRC)/*.cpp)
SERVER_SRCS = $(wildcard $(SERVER_SRC)/*.cpp)
SERVER_CMD_SRCS = $(wildcard $(SERVER_CMD_SRC)/*.cpp)
SHARDCONTROLLER_SRCS = $(wildcard $(SHARDCONTROLLER_SRC)/*.cpp)
SHARDCONTROLLER_CMD_SRCS = $(wildcard $(SHARDCONTROLLER_CMD_SRC)/*.cpp)

CLIENT_OBJS = $(patsubst $(CLIENT_SRC)/%.cpp,$(CLIENT_OBJ)/%.o,$(CLIENT_SRCS))
CLIENT_CMD_OBJS = $(patsubst $(CLIENT_CMD_SRC)/%.cpp,$(CLIENT_CMD_OBJ)/%.o,$(CLIENT_CMD_SRCS))
COMMON_OBJS = $(patsubst $(COMMON_SRC)/%.cpp,$(COMMON_OBJ)/%.o,$(COMMON_SRCS))
KVSTORE_OBJS = $(patsubst $(KVSTORE_SRC)/%.cpp,$(KVSTORE_OBJ)/%.o,$(KVSTORE_SRCS))
NET_OBJS = $(patsubst $(NET_SRC)/%.cpp,$(NET_OBJ)/%.o,$(NET_SRCS))
REPL_OBJS = $(patsubst $(REPL_SRC)/%.cpp,$(REPL_OBJ)/%.o,$(REPL_SRCS))
SERVER_OBJS = $(patsubst $(SERVER_SRC)/%.cpp,$(SERVER_OBJ)/%.o,$(SERVER_SRCS))
SERVER_CMD_OBJS = $(patsubst $(SERVER_CMD_SRC)/%.cpp,$(SERVER_CMD_OBJ)/%.o,$(SERVER_CMD_SRCS))
SHARDCONTROLLER_OBJS = $(patsubst $(SHARDCONTROLLER_SRC)/%.cpp,$(SHARDCONTROLLER_OBJ)/%.o,$(SHARDCONTROLLER_SRCS))
SHARDCONTROLLER_CMD_OBJS = $(patsubst $(SHARDCONTROLLER_CMD_SRC)/%.cpp,$(SHARDCONTROLLER_CMD_OBJ)/%.o,$(SHARDCONTROLLER_CMD_SRCS))

# ====== Testing stuff
TEST_UTILS_OBJ = ./test_utils
TEST_UTILS_SRC = ../test_utils
TEST_UTILS_SRCS = $(wildcard $(TEST_UTILS_SRC)/*.cpp)
TEST_UTILS_OBJS = $(patsubst $(TEST_UTILS_SRC)/%.cpp,$(TEST_UTILS_OBJ)/%.o,$(TEST_UTILS_SRCS))

TEST_DEPENDENCIES = $(TEST_UTILS_OBJS) $(COMMON_OBJS) $(NET_OBJS) $(SHARDCONTROLLER_OBJS) $(SERVER_OBJS) $(KVSTORE_OBJS) $(CLIENT_OBJS)

# All objects, for cleanup
OBJS = $(CLIENT_OBJS) $(COMMON_OBJS) $(KVSTORE_OBJS) $(NET_OBJS) $(REPL_OBJS) $(SERVER_OBJS) $(SHARDCONTROLLER_OBJS) $(CLIENT_CMD_OBJS) 
OBJS += $(SERVER_CMD_OBJS) $(SHARDCONTROLLER_CMD_OBJS) $(TEST_UTILS_OBJS) 

# make all directories
OBJ_DIRS = $(CLIENT_OBJ) $(COMMON_OBJ) $(KVSTORE_OBJ) $(NET_OBJ) $(REPL_OBJ) $(SERVER_OBJ) $(SHARDCONTROLLER_OBJ) $(CLIENT_CMD_OBJ)
OBJ_DIRS += $(SERVER_CMD_OBJ) $(SHARDCONTROLLER_CMD_OBJ) $(TEST_UTILS_OBJ)

EXEC_DIR = ../cmd
EXECS = simple_client client server shardcontroller

all: check-in-container $(OBJ_DIRS) $(EXECS)

# New target to check if you are in the container
check-in-container:
	@if [ ! -e /usr/bin/cs300-docker-version ]; then \
		echo "ERROR: NOT INSIDE COURSE CONTAINER"; \
		exit 1; \
	fi

# Make object directories if they don't exist
$(OBJ_DIRS):
	mkdir -p $@

simple_client: $(COMMON_OBJS) $(NET_OBJS) $(REPL_OBJS) $(CLIENT_OBJS) $(CLIENT_CMD_OBJS) $(EXEC_DIR)/client.cpp
	$(CC) $(CPPFLAGS) -DSIMPLE_CLIENT $^ -o $@

client: $(COMMON_OBJS) $(NET_OBJS) $(REPL_OBJS) $(CLIENT_OBJS) $(CLIENT_CMD_OBJS) $(EXEC_DIR)/client.cpp
	$(CC) $(CPPFLAGS) $^ -o $@

server: $(COMMON_OBJS) $(NET_OBJS) $(REPL_OBJS) $(KVSTORE_OBJS) $(SERVER_OBJS) $(SERVER_CMD_OBJS) $(EXEC_DIR)/server.cpp
	$(CC) $(CPPFLAGS) $^ -o $@

shardcontroller: $(COMMON_OBJS) $(NET_OBJS) $(REPL_OBJS) $(SHARDCONTROLLER_OBJS) $(SHARDCONTROLLER_CMD_OBJS) $(EXEC_DIR)/shardcontroller.cpp
	$(CC) $(CPPFLAGS) $^ -o $@

# Formatting entire directory: https://stackoverflow.com/a/36046965
format:
	find ../ -iname '*.hpp' -o -iname '*.cpp' | xargs clang-format -i -style='{BasedOnStyle: google, DerivePointerAlignment: false, PointerAlignment: Left, AllowShortFunctionsOnASingleLine: None}'


# See here for more: https://stackoverflow.com/a/14061796. See if I like this.
# If the first argument is "check",
ifeq (check,$(firstword $(MAKECMDGOALS)))
  # use the rest as arguments for "check", and turn into do-nothing targets
  CHECK_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
  $(eval $(CHECK_ARGS):;@:)
endif

check: check-in-container $(TEST_DEPENDENCIES)
	./run_tests $(CHECK_ARGS)

ifeq (perf,$(firstword $(MAKECMDGOALS)))
  CPPFLAGS += -O3
endif

perf: clean $(TEST_DEPENDENCIES)
	@rm -f performance-runtime.csv
	@echo "title,time,tput" >> performance-runtime.csv
	@./run_tests A5
	@python3 plot_performance.py
	@make clean

# For the `|` symbol: https://stackoverflow.com/q/12299369
$(CLIENT_OBJ)/%.o: $(CLIENT_SRC)/%.cpp $(CLIENT_SRC)/simple_client.hpp $(CLIENT_SRC)/shardkv_client.hpp | $(CLIENT_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(COMMON_OBJ)/%.o: $(COMMON_SRC)/%.cpp $(COMMON_SRC)/%.hpp | $(COMMON_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(KVSTORE_OBJ)/%.o: $(KVSTORE_SRC)/%.cpp $(KVSTORE_SRC)/%.hpp | $(KVSTORE_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(NET_OBJ)/%.o: $(NET_SRC)/%.cpp $(NET_SRC)/%.hpp | $(NET_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(REPL_OBJ)/%.o: $(REPL_SRC)/%.cpp $(REPL_SRC)/%.hpp | $(REPL_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(SERVER_OBJ)/%.o: $(SERVER_SRC)/%.cpp $(SERVER_SRC)/server.hpp $(KVSTORE_SRC)/simple_kvstore.hpp $(KVSTORE_SRC)/concurrent_kvstore.hpp | $(SERVER_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

$(SHARDCONTROLLER_OBJ)/%.o: $(SHARDCONTROLLER_SRC)/%.cpp $(SHARDCONTROLLER_SRC)/shardcontroller.hpp | $(SHARDCONTROLLER_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@


# ===== Testing stuff

$(TEST_UTILS_OBJ)/%.o: $(TEST_UTILS_SRC)/%.cpp $(TEST_UTILS_SRC)/%.hpp $(SHARDCONTROLLER_SRC)/shardcontroller.hpp $(KVSTORE_SRC)/simple_kvstore.hpp $(KVSTORE_SRC)/concurrent_kvstore.hpp | $(TEST_UTILS_OBJ)
	$(CC) $(CPPFLAGS) -c $< -o $@

TESTS :=
# Method that finds the files for each test suite and 
#  - creates a rule to compile that test
#  - adds the executable to the TESTS variable
# $(1) below should be the path to a test suite directory (e.g. "../tests/server_tests")
define add_test_suite
SRCS_PATH := $$(wildcard $(1)/*.cpp)
TEST_SUITE_TESTS := $$(patsubst $(1)/%.cpp, %, $$(SRCS_PATH))
$$(TEST_SUITE_TESTS): %: $$(TEST_DEPENDENCIES) $(1)/%.cpp
	$$(CC) $$(CPPFLAGS) $$^ -o $$@
TESTS += $$(TEST_SUITE_TESTS)
endef

# Performs the above method for each test suite folder
TEST_DIRS = $(wildcard $(PROJECT_SRC)/tests/*_tests)
$(foreach dir, $(TEST_DIRS), \
  $(eval $(call add_test_suite,$(dir))))

clean:
	rm -f $(EXECS) $(OBJS) $(TESTS)

.PHONY = all clean check format check-in-container
