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


# ── installation dir ─────────────────────────────────────────────────────────
PREFIX  = $(HOME)/.local
BINDIR  = $(PREFIX)/bin
LIBDIR 	= $(PREFIX)/lib
DATADIR = $(PREFIX)/share/repman

BIN=repman

# ── Source files ─────────────────────────────────────────────────────────────
#
# $(wildcard ...)  expands a glob at make-time
# $(patsubst ...)  does pattern substitution:
#   patsubst src/%.c, build/%.o, <list>  →  replaces src/X.c with build/X.o

SRCS    = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(SRCS)
OBJS    = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# ── Output ───────────────────────────────────────────────────────────────────

LIB     = $(BUILD_DIR)/librepman.so

# ── Default target ───────────────────────────────────────────────────────────

.PHONY: all
all: $(LIB)

# Link all object files into a shared library (.so)
$(LIB): $(OBJS) | $(BUILD_DIR)
	$(CC) -shared -o $@ $^ -lcurl -lcjson
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

TEST_FILES = $(wildcard $(TEST_DIR)/*.c)

.PHONY: test
test: $(TEST_FILES) $(TEST_SRCS) | $(BUILD_DIR)
	@for t in $(TEST_FILES); do \
	    name=$$(basename $$t .c); \
	    echo "Building test: $$name"; \
	    $(CC) $(CFLAGS) -DTESTING $(TEST_SRCS) $$t -o $(BUILD_DIR)/$$name -lcurl -lcjson; \
	    echo "Running test: $$name"; \
	    $(BUILD_DIR)/$$name; \
	done

# ── Python CLI tests ──────────────────────────────────────────────────────────

.PHONY: test-cli
test-cli: $(LIB)
	python3 $(TEST_DIR)/test_cli.py

# ── Clean ─────────────────────────────────────────────────────────────────────

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned."


# –– Install –––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

install: $(BIN)
	mkdir -p $(BINDIR)
	mkdir -p $(LIBDIR)
	mkdir -p $(DATADIR)
	chown -R $(USER):$(USER) $(PREFIX)
	
#	copy binaries, lib files, and data
#	initially, must be a ci.pub key in ./sig/
#	run ensure_dirs()
	cp $(BIN) $(BINDIR)/$(BIN)
	chmod +x $(BINDIR)/$(BIN)

	cp -r cli $(DATADIR)/
	cp -r sig $(DATADIR)/
	cp ci.pub $(DATADIR)/sig/
	cp -r build $(DATADIR)/
	cp config.env.example $(DATADIR)/config.env

#	create venv
	python3 -m venv $(DATADIR)/cli/venv
# 	"$(DATADIR)/cli/venv/bin/pip3" install -q python-dotenv
	@if [ $? -ne 0 ]; then \
		@echo "Failed to create venv" \
		exit 1; \
	fi
# 	source $(DATADIR)/cli/venv/bin/activate 
# 	pip3 install -q python-dotenv
# 	deactivate $(DATADIR)/cli/venv/bin/activate

#	ensure correct PATH variable

#	ensure needed project direcetories


