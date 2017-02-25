BUILD_DIR=build
SRC_DIR=src
CC=clang-3.9
CXX=clang++-3.9
CC_FLAGS=
CXX_FLAGS=-ferror-limit=5 -std=c++1z -fno-rtti -fno-exceptions \
          -fno-use-cxa-atexit -fno-threadsafe-statics \
          -Weverything -Werror -pedantic \
          -Wno-exit-time-destructors -Wno-missing-prototypes -Wno-missing-variable-declarations \
          -Wno-c++98-c++11-c++14-compat -Wno-c++98-compat -Wno-c++98-compat-pedantic \
          -g -O1
LD_FLAGS_L=-ldl -lm -lpthread
LD_FLAGS_W=-L/usr/lib/gcc/x86_64-w64-mingw32/6.2-win32 -mwindows -lgdi32

CXX_FILES:=$(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/lib/*.cpp)
OBJ_FILES_L:=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.l.o,$(CXX_FILES))
OBJ_FILES_W:=$(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.w.o,$(CXX_FILES))

all: test

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(BUILD_DIR)/lib:
	mkdir -p $(BUILD_DIR)/lib

$(BUILD_DIR)/%.l.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXX_FLAGS)
$(BUILD_DIR)/%.w.o: $(SRC_DIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXX_FLAGS) -target x86_64-w64-mingw32 -DUNICODE -I/usr/x86_64-w64-mingw32/include

test: $(BUILD_DIR) $(BUILD_DIR)/lib $(OBJ_FILES_L) $(OBJ_FILES_W)
	$(CXX) $(LD_FLAGS_L) $(OBJ_FILES_L) -o $(BUILD_DIR)/test
	$(CXX) $(LD_FLAGS_W) $(OBJ_FILES_W) -o $(BUILD_DIR)/test.exe -target x86_64-w64-mingw32

run: test
	./$(BUILD_DIR)/test

# sudo apt install mingw-w64-x86-64-dev g++-mingw-w64-x86-64

