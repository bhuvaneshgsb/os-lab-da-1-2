# ============================================================
# Predictive Severity-Aware RTOS Patient Monitoring System
# FreeRTOS Windows Simulator + MSYS2 UCRT64
# ============================================================

CC = gcc

PROJECT = PatientMonitoringRTOS
BUILD_DIR = build

# FreeRTOS location relative to this project
FREERTOS_DIR = ../FreeRTOS/FreeRTOS

# FreeRTOS kernel
KERNEL_DIR = $(FREERTOS_DIR)/Source

# Windows/MinGW FreeRTOS port
PORT_DIR = $(KERNEL_DIR)/portable/MSVC-MingW

# FreeRTOS include directories
INCLUDES = \
	-Iinclude \
	-I$(KERNEL_DIR)/include \
	-I$(PORT_DIR) \
	-Iprediction

# Compiler flags
CFLAGS = -Wall -Wextra -g $(INCLUDES)

# FreeRTOS kernel source files
KERNEL_SOURCES = \
	$(KERNEL_DIR)/croutine.c \
	$(KERNEL_DIR)/event_groups.c \
	$(KERNEL_DIR)/list.c \
	$(KERNEL_DIR)/queue.c \
	$(KERNEL_DIR)/stream_buffer.c \
	$(KERNEL_DIR)/tasks.c \
	$(KERNEL_DIR)/timers.c

# Windows port
PORT_SOURCES = \
	$(PORT_DIR)/port.c

# Memory management
HEAP_SOURCES = \
	$(KERNEL_DIR)/portable/MemMang/heap_4.c

# Our project source files
PROJECT_SOURCES = \
	src/main.c \
	src/severity.c \
	src/database.c \
	prediction/prediction.c

# All source files
SOURCES = \
	$(PROJECT_SOURCES) \
	$(KERNEL_SOURCES) \
	$(PORT_SOURCES) \
	$(HEAP_SOURCES)

# Convert .c files to build/*.o
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

TARGET = $(BUILD_DIR)/$(PROJECT).exe


# ============================================================
# Default target
# ============================================================

all: $(TARGET)


# ============================================================
# Link
# ============================================================

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJECTS) -o $@ -lwinmm -lws2_32 -lsqlite3


# ============================================================
# Compile
# ============================================================

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@


# ============================================================
# Clean
# ============================================================

clean:
	rm -rf $(BUILD_DIR)


# ============================================================
# Run
# ============================================================

run: $(TARGET)
	./$(TARGET)


.PHONY: all clean run