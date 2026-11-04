CXX = g++
CXXFLAGS = -O3 -DNDEBUG -march=native -std=c++23 -Wall -Wextra

SRC = engine/game.cpp
TARGET = build/game
HEADERS = $(wildcard engine/*.hpp)

$(TARGET): $(SRC) $(HEADERS)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean: 
	rm -r build

.PHONY: run clean
