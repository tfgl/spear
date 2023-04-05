CC := g++
INC := -Isrc
CARGS := -std=c++20 -c
DARGS := -Og -g3 -Wall
RARGS := -O3
OBJ_DIR := target/release/object
BIN_DIR := target/release/build
SRC_FILES := $(wildcard src/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(SRC_FILES))

TOML_SRC_FILES := $(wildcard src/toml/*.cpp)
TOML_OBJ_FILES := $(patsubst src/toml/%.cpp,$(OBJ_DIR)/toml/%.o,$(TOML_SRC_FILES))

spear: mktree $(OBJ_FILES)
	$(CC) $(RARGS) -o $@ $^

$(OBJ_DIR)/toml/%.o: scr/toml/%.cpp
	$(CC) $(INC) $(CARGS) $(RARGS) -o $@ $<

$(OBJ_DIR)/%.o: ./scr/%.cpp
	$(CC) $(INC) $(CARGS) $(RARGS) -o $@ $<

mktree:
	mkdir -p $(OBJ_DIR) $(BIN_DIR)
