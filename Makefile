# repman/Makefile
#
# Targets:
#   make              — build librepman.so  (default)
#   make test         — build and run tests/test_util
#   make clean        — remove all build outputs

# ── Compiler settings ────────────────────────────────────────────────────────

CC      = gcc

CFLAGS  = -Wall -Wextra -Wpedantic   # Turn on almost all warnings — learn from them
CFLAGS += -std=c11                   # Use C11, a modern and stable standard
CFLAGS += -fPIC                      # Required for shared libraries (Position-Independent Code)
CFLAGS += -Isrc                      # Let #include "util.h" work from any src file
CFLAGS += -g                         # Include debug symbols (for gdb / valgrind)

# Add -DDEBUG to enable any debug-only code paths
# CFLAGS += -DDEBUG

# ── Directories ──────────────────────────────────────────────────────────────

SRC_DIR   = src
BUILD_DIR = build
TEST_DIR  = tests

# ── Source files ─────────────────────────────────────────────────────────────
#
# $(wildcard ...)  expands a glob at make-time
# $(patsubst ...)  does pattern substitution:
#   patsubst src/%.c, build/%.o, <list>  →  replaces src/X.c with build/X.o

SRCS    = $(wildcard $(SRC_DIR)/*.c)
OBJS    = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ── Output ───────────────────────────────────────────────────────────────────

LIB     = $(BUILD_DIR)/librepman.so

# ── Default target ───────────────────────────────────────────────────────────

.PHONY: all
all: $(LIB)

# Link all object files into a shared library (.so)
$(LIB): $(OBJS) | $(BUILD_DIR)
	$(CC) -shared -o $@ $^
	@echo "Built $@"

# ── Compile each .c → .o ─────────────────────────────────────────────────────
#
# $<  = the first prerequisite  (the .c file)
# $@  = the target              (the .o file)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ── Ensure build directory exists ────────────────────────────────────────────

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ── Tests ─────────────────────────────────────────────────────────────────────
#
# Each test file in tests/ is compiled against the same sources.
# Run with:  make test

TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)

.PHONY: test
test: $(TEST_SRCS) $(SRCS) | $(BUILD_DIR)
	@for t in $(TEST_SRCS); do \
	    name=$$(basename $$t .c); \
	    echo "Building test: $$name"; \
	    $(CC) $(CFLAGS) $(SRCS) $$t -o $(BUILD_DIR)/$$name -lcurl; \
	    echo "Running test: $$name"; \
	    $(BUILD_DIR)/$$name; \
	done

# ── Clean ─────────────────────────────────────────────────────────────────────

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned."