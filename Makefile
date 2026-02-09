# Compilatore e flag
CXX      := g++
CXXFLAGS := -Wall -Wextra -Iinclude
LDFLAGS  := 

# Cartelle
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
OBJ_DIR   := build
BIN_DIR   := bin

# File
# Trova tutti i .cpp in src/ e tests/
SRC       := $(wildcard $(SRC_DIR)/*.cpp)
OBJ       := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET    := $(BIN_DIR)/test.out

# Regola principale
all: $(TARGET)

# Linker: crea l'eseguibile finale
$(TARGET): $(OBJ) $(OBJ_DIR)/test.o
	@mkdir -p $(BIN_DIR)
	$(CXX) $(OBJ) $(OBJ_DIR)/test.o -o $(TARGET) $(LDFLAGS)
	@echo "Costruito con successo: $(TARGET)"

# Compilazione dei file oggetto da src/
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compilazione del file di test
$(OBJ_DIR)/test.o: $(TEST_DIR)/test.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Pulizia completata."

.PHONY: all clean