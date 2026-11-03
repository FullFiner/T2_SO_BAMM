CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -MMD -MP
TARGET = torneo
SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:src/%.cpp=build/%.o)
DEP = $(OBJ:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(DEP)

.PHONY: all clean