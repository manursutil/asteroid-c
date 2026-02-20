# Makefile for Asteroid — supports PLATFORM_WEB (emscripten) and PLATFORM_DESKTOP
#
# Usage:
#   Desktop : make
#   Web     : make PLATFORM=PLATFORM_WEB
#
# Prerequisites for PLATFORM_WEB:
#   1. Install emscripten SDK   → https://emscripten.org/docs/getting_started/downloads.html
#   2. Compile raylib for web   → cd raylib/src && make PLATFORM=PLATFORM_WEB -B
#      (produces raylib/src/libraylib.a compiled for WASM)
#   3. Set EMSDK_PATH and RAYLIB_PATH below (or pass them on the command line)

# ---------------------------------------------------------------------------
# Paths — edit these to match your setup
# ---------------------------------------------------------------------------
EMSDK_PATH  ?= $(HOME)/emsdk
RAYLIB_PATH ?= $(HOME)/raylib

# Where the web-compiled libraylib.a lives
RAYLIB_INCLUDE_PATH ?= $(RAYLIB_PATH)/src
RAYLIB_LIB_PATH     ?= $(RAYLIB_PATH)/src

# The HTML shell provided by raylib (nice minimal layout)
SHELL_FILE ?= $(RAYLIB_PATH)/src/minshell.html

# ---------------------------------------------------------------------------
# Project settings
# ---------------------------------------------------------------------------
PROJECT_NAME   = asteroid
SOURCE_FILE    = asteroid_web.c
BUILD_DIR      = build

PLATFORM       ?= PLATFORM_DESKTOP

# ---------------------------------------------------------------------------
# Desktop build (gcc)
# ---------------------------------------------------------------------------
ifeq ($(PLATFORM),PLATFORM_DESKTOP)
    CC      = gcc
    CFLAGS  = -Wall -O2
    LDFLAGS = -lraylib -lm

    # Detect OS for library paths / linker flags
    UNAME := $(shell uname -s)
    ifeq ($(UNAME), Linux)
        LDFLAGS += -lGL -lpthread -ldl -lrt -lX11
    endif
    ifeq ($(UNAME), Darwin)
        LDFLAGS += -framework OpenGL -framework Cocoa -framework IOKit
    endif

    OUT = $(BUILD_DIR)/$(PROJECT_NAME)

all: $(OUT)

$(OUT): $(SOURCE_FILE) | $(BUILD_DIR)
	$(CC) -o $@ $< $(CFLAGS) -I$(RAYLIB_INCLUDE_PATH) -L$(RAYLIB_LIB_PATH) $(LDFLAGS)

endif

# ---------------------------------------------------------------------------
# Web build (emscripten)
# ---------------------------------------------------------------------------
ifeq ($(PLATFORM),PLATFORM_WEB)
    # Make sure emcc is in PATH.  If you sourced emsdk_env.sh this is automatic.
    CC = emcc

    CFLAGS  = -Os -Wall -DPLATFORM_WEB

    # Core emscripten linker flags
    LDFLAGS = \
        $(RAYLIB_LIB_PATH)/libraylib.a \
        -s USE_GLFW=3 \
        -s WASM=1 \
        -s GL_ENABLE_GET_PROC_ADDRESS=1 \
        -s TOTAL_MEMORY=67108864 \
        --shell-file $(SHELL_FILE)

    # Uncomment the line below ONLY if you use the while(!WindowShouldClose())
    # loop instead of emscripten_set_main_loop (adds ~20 % overhead):
    # LDFLAGS += -s ASYNCIFY

    # Uncomment to bundle a resources/ directory into a .data file:
    # LDFLAGS += --preload-file resources

    OUT_HTML = $(BUILD_DIR)/$(PROJECT_NAME).html
    OUT_DIR  = $(BUILD_DIR)

all: $(OUT_HTML)

$(OUT_HTML): $(SOURCE_FILE) | $(BUILD_DIR)
	$(CC) -o $@ $< $(CFLAGS) \
	    -I$(RAYLIB_INCLUDE_PATH) \
	    $(LDFLAGS)
	@echo ""
	@echo "Build complete!  Files generated in $(BUILD_DIR)/:"
	@echo "  $(PROJECT_NAME).html  — open this in a browser"
	@echo "  $(PROJECT_NAME).js"
	@echo "  $(PROJECT_NAME).wasm"
	@echo ""
	@echo "Serve locally with:"
	@echo "  python3 -m http.server 8080  (then open localhost:8080/$(PROJECT_NAME).html)"
	@echo "  — or —"
	@echo "  emrun $(OUT_HTML)"

endif

# ---------------------------------------------------------------------------
# Shared rules
# ---------------------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: all
ifeq ($(PLATFORM),PLATFORM_WEB)
	emrun $(BUILD_DIR)/$(PROJECT_NAME).html
else
	./$(BUILD_DIR)/$(PROJECT_NAME)
endif

.PHONY: all clean run