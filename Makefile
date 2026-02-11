# Makefile aggiornato
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude  # Root delle inclusioni
SRC_DIR   := src
OBJ_DIR   := build
BIN_DIR   := bin

# Trova tutti i file .cpp in src/ e nelle sue sottocartelle (come events/)
SRC       := $(shell find $(SRC_DIR) -name "*.cpp")
# Trasforma src/path/file.cpp in build/path/file.o
OBJ       := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TARGET    := $(BIN_DIR)/test.out

all: $(TARGET)

$(TARGET): $(OBJ) $(OBJ_DIR)/test.o
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ) $(OBJ_DIR)/test.o -o $(TARGET)

# La chiave dell'estensibilità: crea le sottocartelle in build al volo
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/test.o: tests/test.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

# Aggiungi questo al tuo Makefile
format:
	find $(SRC_DIR) $(INC_DIR) -name "*.cpp" -o -name "*.h" | xargs clang-format -i
	@echo "Codice formattato con successo!"