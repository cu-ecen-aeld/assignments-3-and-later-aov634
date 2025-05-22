# Compiler setup
ifeq ($(BUILD),cross)
    $(info CROSS-COMPILATION ENABLED)
    CC = /home/aov634/Downloads/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-gcc
else
    $(info NATIVE COMPILATION: Using default GCC)
    CC = gcc
endif
$(info Compiler selected: $(CC))

.DEFAULT_GOAL := build

all: build

# Clean previous build artifacts
clean:
	rm -f aesdsocket.o aesdsocket

build: aesdsocket

CROSS_COMPILE:
	$(MAKE) build BUILD=cross

# Compile the object file from source
aesdsocket.o: aesdsocket.c
	$(CC) -Wall -Werror -c aesdsocket.c -o aesdsocket.o

# Link the object file to create the executable
aesdsocket: aesdsocket.o
	$(CC) aesdsocket.o -o aesdsocket
