.SUFFIXES:
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -Werror -Og -g -pipe -fPIC -MMD -MP
LDFLAGS   :=#

PROJECT_DIR := $(CURDIR)
MAIN_DIR    := $(PROJECT_DIR)/src
OBJECTS_DIR := $(PROJECT_DIR)/objects
BIN_DIR     := $(PROJECT_DIR)/bin


BOOST_INC :=/home/blackbox/boost_1_80_0
BOOST_LIB :=/home/blackbox/boost_1_80_0/stage/lib
LDFLAGS +=-Wl,-rpath,$(BOOST_LIB) -L$(BOOST_LIB) -lboost_python38

PYTHON_INC :=/usr/include/python3.8
#PYTHON_LIB :=/usr/lib/python3.8
PYTHON_LIB :=/usr/lib/python3.8/config-3.8-x86_64-linux-gnu
LDFLAGS    +=-Wl,-rpath,$(PYTHON_LIB) -L$(PYTHON_LIB) -lpython3.8


MAIN_TARGET_BASENAME := main
MAIN_TARGET := $(BIN_DIR)/$(MAIN_TARGET_BASENAME).e

BPMOD_TARGET_BASENAME := my_cpp_module
BPMOD_TARGET := $(BIN_DIR)/$(BPMOD_TARGET_BASENAME).so

SRC_DIRS  := $(shell find "$(MAIN_DIR)" -type d)
OBJ_DIRS  := $(patsubst $(PROJECT_DIR)/src%,$(PROJECT_DIR)/objects/src%,$(SRC_DIRS))

SOURCES_CPP :=$(shell find "$(MAIN_DIR)" -name '*.cpp')
OBJECTS_CPP :=$(patsubst $(PROJECT_DIR)/src%,$(PROJECT_DIR)/objects/src%,$(SOURCES_CPP:.cpp=.o))
DEPENDENCIES :=$(shell find "$(OBJECTS_DIR)" -name '*.d')

$(shell mkdir -p "$(BIN_DIR)" "$(OBJECTS_DIR)" $(OBJ_DIRS))

CXXFLAGS += -I$(MAIN_DIR) -I$(BOOST_INC) -I$(PYTHON_INC)

.PHONY: default all clean dinfo

default: $(MAIN_TARGET)

all: $(MAIN_TARGET) $(BPMOD_TARGET)

$(MAIN_TARGET): $(OBJECTS_CPP)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BPMOD_TARGET): $(filter-out %main.o, $(OBJECTS_CPP))
	$(CXX) -shared -export-dynamic $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJECTS_DIR)/src/%.o: $(MAIN_DIR)/%.cpp
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS) -c $< -o $@

dinfo:
	@echo "OBJECTS_DIR: $(OBJECTS_DIR)/src"
	@echo "TARGET:      $(MAIN_TARGET)"
	@echo "SRCDIRS:     $(SRC_DIRS)"
	@echo "OBJDIRS:     $(OBJ_DIRS)"
	@echo "SOURCES:     $(words $(SOURCES_CPP)) file(s)"
	@echo "DEPS:        $(DEPENDENCIES)"

clean:
	@echo "Cleaning build outputs…"
	@rm -rf "$(OBJECTS_DIR)/src" "$(MAIN_TARGET)" $(BPMOD_TARGET)

-include $(DEPENDENCIES)
