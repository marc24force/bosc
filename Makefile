SRC := $(wildcard src/*.cpp)
OBJ := $(patsubst src/%.cpp, make_build/%.o, $(SRC))

BRUC := .bruc

CXX := g++
CXXFLAGS := -Wall -O3 -I$(BRUC)/include -Iinclude
OUT := make_bosc.exe

all: $(OUT)

$(BRUC)/lib/libbruc.a: 
	git clone https://github.com/marc24force/bruc.git $(BRUC)
	$(MAKE) -C $(BRUC)


$(OUT): $(BRUC)/lib/libbruc.a $(OBJ) 
	$(CXX) $(CXXFLAGS) $(OBJ) -L$(BRUC)/lib -lbruc -o $@

make_build/%.o: src/%.cpp
	@mkdir -p make_build
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf make_build $(OUT)

