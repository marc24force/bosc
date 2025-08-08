CIRI ?= ../ciri

ifeq ($(OS),Windows_NT)
    RM = del /Q
    SEP = \\
else
    RM = rm -f
    SEP = /
endif

CXXFLAGS = -std=c++20 -Wall -Iinclude -I$(CIRI)/include -I$(CIRI)/inih/cpp 
LDFLAGS = $(CIRI)/build/libciri.a -Wl,--gc-sections
SRC = src/main.cpp src/Bosc.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = bosc

all: run

$(TARGET): ciri $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

.PHONY: ciri
ciri: 
	$(MAKE) -C $(CIRI)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET) build
	@$(eval BUILD_DIR := $(shell echo build-*))
	@./$(BUILD_DIR)$(SEP)bin$(SEP)bosc install
	@$(RM) $(OBJ) $(TARGET)

clean:
	$(RM) $(OBJ) $(TARGET)

