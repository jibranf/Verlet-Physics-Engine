CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11
CPPFLAGS := -Isrc/dependencies/include
LDFLAGS := -Lsrc/dependencies/lib
LDLIBS := -lglfw3 -lopengl32 -lgdi32

SRC_DIR := src
BUILD_DIR := build

TARGET := $(BUILD_DIR)\app.exe
SRC := $(SRC_DIR)\app.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

run: $(TARGET)
	@echo Running $(TARGET)...
	@$(TARGET)