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
	    $(CC) $(CFLAGS) -DTESTING -I$(TEST_DIR) $(TEST_SRCS) $$t -o $(BUILD_DIR)/$$name -lcurl -lcjson; \
	    echo "Running test: $$name"; \
	    $(BUILD_DIR)/$$name; \
	done

# ── ASAN / UBSan tests ────────────────────────────────────────────────────────
# Run all C unit tests compiled with AddressSanitizer + UBSan.
# Usage:  make test-asan

ASAN_FLAGS = -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: test-asan
test-asan: $(TEST_FILES) $(TEST_SRCS) | $(BUILD_DIR)
	@for t in $(TEST_FILES); do \
	    name=$$(basename $$t .c); \
	    echo "Building ASAN test: $$name"; \
	    $(CC) $(CFLAGS) $(ASAN_FLAGS) -DTESTING -I$(TEST_DIR) $(TEST_SRCS) $$t -o $(BUILD_DIR)/$$name-asan -lcurl -lcjson; \
	    echo "Running ASAN test: $$name"; \
	    $(BUILD_DIR)/$$name-asan; \
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

.PHONY: install
install: $(LIB)
	@echo "Installing repman to $(PREFIX)..."

	# Create installation directories (mirrors repman_ensure_dirs)
	mkdir -p $(BINDIR)
	mkdir -p $(DATADIR)/index
	mkdir -p $(DATADIR)/sig/index
	mkdir -p $(DATADIR)/packages
	mkdir -p $(DATADIR)/tmp
	mkdir -p $(DATADIR)/cache
	mkdir -p $(DATADIR)/cli

	# Copy the shared library and CLI sources
	cp -r $(BUILD_DIR) $(DATADIR)/
	cp -r cli $(DATADIR)/

	# Copy config.env only if it does not already exist (preserve user edits)
	@if [ ! -f $(DATADIR)/config.env ]; then \
		cp config.env.example $(DATADIR)/config.env; \
		echo "Config written to $(DATADIR)/config.env"; \
	else \
		echo "Keeping existing $(DATADIR)/config.env"; \
	fi

	# Copy public key if available locally; otherwise warn
	@if [ -f sig/ci.pub ]; then \
		cp sig/ci.pub $(DATADIR)/sig/ci.pub; \
		echo "Public key installed from sig/ci.pub"; \
	elif [ -f ci.pub ]; then \
		cp ci.pub $(DATADIR)/sig/ci.pub; \
		echo "Public key installed from ci.pub"; \
	else \
		echo "WARNING: no ci.pub found — run 'repman fetch-key' after install"; \
	fi

	# Create venv and install Python dependencies
	python3 -m venv --clear $(DATADIR)/cli/venv
	$(DATADIR)/cli/venv/bin/pip3 install -q python-dotenv "textual>=0.80.0"

	# Write the repman wrapper script
	@printf '#!/bin/sh\nexec "$(DATADIR)/cli/venv/bin/python3" "$(DATADIR)/cli/repcli.py" "$$@"\n' \
		> $(BINDIR)/$(BIN)
	chmod +x $(BINDIR)/$(BIN)

	# Register repman source version in installed.json (uses repman_update_installed, which
	# creates the file if absent and skips the write if repman is already registered)
	@python3 -c "\
	import sys; \
	sys.path.insert(0, '$(DATADIR)/cli'); \
	import repman; \
	rc = repman.update_installed('$(DATADIR)/index/installed.json', 'repman', '0.0.0'); \
	print('repman 0.0.0 registered (source install)' if rc == 0 else 'repman already registered, keeping existing version' if rc == 1 else 'error registering repman: rc=' + str(rc)); \
	sys.exit(0 if rc >= 0 else 1)"

	# Warn if BINDIR is not in PATH
	@if ! printf '%s' "$$PATH" | tr ':' '\n' | grep -qx '$(BINDIR)'; then \
		echo ""; \
		echo "NOTE: $(BINDIR) is not in PATH."; \
		echo "Add this line to your shell profile (~/.bashrc, ~/.profile, etc.):"; \
		echo "  export PATH=\"$(BINDIR):\$$PATH\""; \
	fi

	@echo ""
	@echo "repman installed. Run 'repman --help' to get started."

