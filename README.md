# Asteroid game in C

A classic Asteroids-style arcade game built in C with raylib, with native and web (WebAssembly) builds.

![Preview](/preview.gif)

## Native build (desktop)

### Prerequisites

1. Install **raylib**: https://github.com/raysan5/raylib#build-and-installation
2. Clone this repository

### Linux/macOS (pkg-config raylib setup)

```bash
./build.sh
./run.sh
```

### Other platforms / setups (desktop)

If this does not work (for example on Windows, or with a raylib installation that does not provide pkg-config), you will need to build the project manually using your preferred toolchain (e.g. CMake or an IDE).

See the official raylib repository for platform-specific installation and build instructions:
https://github.com/raysan5/raylib#build-and-installation

## Web build (Emscripten / WebAssembly)

### Prerequisites

1. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
2. Compile raylib for web:

```bash
cd ~/raylib/src
make PLATFORM=PLATFORM_WEB -B
```

3. From this repo, build the web version:

```bash
make PLATFORM=PLATFORM_WEB RAYLIB_PATH=~/raylib EMSDK_PATH=~/emsdk SOURCE_FILE=asteroid-web.c
```

Generated files are placed in `build/`:
- `asteroid.html`
- `asteroid.js`
- `asteroid.wasm`

Run a local server and open the game in your browser:

```bash
python3 -m http.server 8080
```

Then open:
- `http://localhost:8080/build/asteroid.html`

## Gameplay

- **Move**: Arrow keys (↑ ↓ ← →)
- **Shoot**: Space bar
- **Restart**: R (after game over/win)

Destroy all asteroids to win! Larger asteroids require multiple hits and split into smaller pieces. Avoid colliding with any asteroid or it's game over.
