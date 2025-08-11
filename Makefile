ifdef BOSC_ROOT
	BOSC_DIR := $(BOSC_ROOT)
else ifdef XDG_DATA_HOME
	BOSC_DIR := $(XDG_DATA_HOME)/bosc
else 
	BOSC_DIR := $(HOME)/.local/bosc
endif

CIRI = $(BOSC_DIR)/ciri

ifeq ($(OS),Windows_NT)
    RM = del /Q
    SEP = \\
else
    RM = rm -f
    SEP = /
endif


CXXFLAGS =-O3 -std=c++20 -Wall -Iinclude -I$(CIRI)/include -I$(CIRI)/inih/cpp
LDFLAGS = $(CIRI)/build/libciri.a -Wl,--gc-sections
SRC = src/main.cpp src/Bosc.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = bosc

all: run

$(TARGET): ciri $(OBJ)
	@echo " - Building bosc"
	@$(CXX) $(OBJ) -o $@ $(LDFLAGS)

.PHONY: ciri
ciri: 
	@echo "[Makefile]"
	@echo " - Downloading ciri"
	@[ -d "$(CIRI)/.git" ] || git clone --quiet --recursive https://github.com/marc24force/ciri.git $(CIRI)
	@echo " - Building ciri"
	@$(MAKE) --no-print-directory -s -C $(CIRI) 

%.o: %.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	@echo "Build finished! Rebuilding with bosc\n"
	@./$(TARGET) install
	@echo "\n[Makefile]"
	@echo " - Cleaning non-bosc ciri build"
	@$(MAKE) --no-print-directory -s -C $(CIRI) clean
	@echo " - Cleaning non-bosc bosc build"
	@$(RM) $(OBJ) $(TARGET)

clean:
	$(RM) $(OBJ) $(TARGET)

