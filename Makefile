TARGET := DEBUG
PLATFORM := x64
SRCDIR := src
INCLUDEDIR := include
OBJDIR := obj
SOURCES := $(wildcard src/*.cpp) $(wildcard src/*.c) $(wildcard src/**/*.cpp) $(wildcard src/**/*.c)
HEADERS := $(wildcard include/*.hpp) $(wildcard include/*.h) $(wildcard include/**/*.hpp) $(wildcard include/**/*.h)
OBJ := $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

TEST_PROJ := test

ARGS = $(TEST_PROJ) 1 2 3

ifdef OS
    CC := msbuild
    ifdef RELEASE
    CC_ARGS := /property:Configuration=Release /property:Platform=x64 -m
    EXE := x64\Release\native-js.exe
	else
    CC_ARGS := /property:Configuration=Debug /property:Platform=x64 -m
    EXE := x64\Debug\native-js.exe
	endif
    RM := msbuild 
    RM_ARGS := $(CC_ARGS) -target:clean

else ifeq ($(shell uname), Linux)

else ifeq ($(UNAME), Darwin)
    
endif

build:
	$(CC) $(CC_ARGS)

build-test:
	cd $(TEST_PROJ) && npx tsc

build-all:
	$(MAKE) build
	$(MAKE) build-test

shaders:
	glslc resources/shaders/src/default.vert -o resources/shaders/out/vert.spv
	glslc resources/shaders/src/default.frag -o resources/shaders/out/frag.spv

run:
	$(MAKE) build-all
	$(EXE) $(ARGS)

exec:
	$(MAKE) build-test
	$(EXE) $(ARGS)

clean:
	msbuild $(CC_ARGS) -target:clean

quick-run:
	$(EXE) $(ARGS)