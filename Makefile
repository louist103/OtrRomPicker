CC=gcc
CXX=g++

#SRC_DIR=src
BUILD_DIR=build

C_SOURCES=$(wildcard *.c)
CXX_SOURCES=$(wildcard *.cpp)

C_OBJECTS=$(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CXX_OBJECTS=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES))

EXE=extract.elf

.PHONY: all clean

all: $(EXE)

$(shell mkdir -p build)

$(EXE): $(C_OBJECTS) $(CXX_OBJECTS)
	$(CXX) $(C_OBJECTS) $(CXX_OBJECTS) -o $@ -lSDL2

$(BUILD_DIR)/%.o: %.c
	$(CC) -c $< -o $@ -msse4.2 -O2

$(BUILD_DIR)/%.o: %.cpp
	$(CXX) -c $< -o $@ -std=c++20 -O2

clean:
	rm -f $(BUILD_DIR)/*.o $(EXE)
