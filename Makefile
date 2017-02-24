BUILD_DIR=build
SRC_DIR=src
CC=clang-3.9
CXX=clang++-3.9
CC_FLAGS=
CXX_FLAGS=-ferror-limit=5 -std=c++1z -fno-rtti -fno-exceptions \
		  -fno-use-cxa-atexit -fno-threadsafe-statics \
		  -Weverything -Werror -pedantic \
-Wno-exit-time-destructors \
		  -Wno-missing-prototypes -Wno-missing-variable-declarations \
		  -Wno-c++98-compat -Wno-c++98-compat-pedantic \
		  -Wno-c++98-c++11-c++14-compat \
		  -g -O1 -Wno-unused-parameter -Wno-unused-variable
LD_FLAGS=-ldl -lm -lpthread

CXX_FILES:=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/lib/*.cpp)
OBJ_FILES:=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CXX_FILES))

all: test

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/lib:
	mkdir -p $(BUILD_DIR)/lib

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXX_FLAGS)

test: $(BUILD_DIR) $(BUILD_DIR)/lib $(OBJ_FILES)
	$(CXX) $(LD_FLAGS) -o $(BUILD_DIR)/test $(OBJ_FILES)

run: test
	./$(BUILD_DIR)/test

