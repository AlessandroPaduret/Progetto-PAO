# Makefile aggiornato
CXX      := g++

# Flag per la fase di COMPILAZIONE (Header files)
QT_INCLUDES = $(shell pkg-config --cflags Qt6Core)
CXXFLAGS := -std=c++20 -Wall -Wextra $(QT_INCLUDES) -Iinclude  # Root delle inclusioni

# Flag per la fase di LINKING (Librerie condivise .so)
# PS: Queste vengono risolte universalmente in tutte le distro Unix, 
# su NixOS sono necessarie per l'hashing dei path per Qt6 ed il cambio continuo(per questo pkg-config)
QT_LIBS = $(shell pkg-config --libs Qt6Core)
LDLIBS = $(QT_LIBS)

SRC_DIR   := src
OBJ_DIR   := build
BIN_DIR   := bin

# Trova tutti i file .cpp in src/ e nelle sue sottocartelle (come events/)
SRC       := $(shell find $(SRC_DIR) -name "*.cpp")
# Trasforma src/path/file.cpp in build/path/file.o
OBJ       := $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

TARGET    := $(BIN_DIR)/test.out

all: $(TARGET)

# FASE DI LINKING
# PS: usato $^ per l'espansione automatica di tutte le dependencies $(OBJ) e $(OBJ_DIR)/test
# PSS: Stessa cosa per il targer $@
$(TARGET): $(OBJ) $(OBJ_DIR)/test.o
	@mkdir -p $(BIN_DIR)
	$(CXX) $^ -o $@ $(LDLIBS)

# FASE DI COMPILAZIONE (Qui servono solo le includes / CXXFLAGS)
# Il flag "-c" dice "compila solo, non linkare"
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

