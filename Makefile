CC := gcc
CFLAGS := -Wall -Wextra -pedantic -std=c11
CPPFLAGS := -Isrc/dependencies/include
LDFLAGS := -Lsrc/dependencies/lib
LDLIBS := -lglew32 -lglfw3 -lopengl32 -lgdi32 -lm

SRC_DIR := src
BUILD_DIR := build

TARGET := $(BUILD_DIR)\app.exe
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR):
	if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)

clean:
	if exist $(BUILD_DIR) rmdir /s /q $(BUILD_DIR)

run: $(TARGET)
	@echo Running $(TARGET)...
	@$(TARGET)