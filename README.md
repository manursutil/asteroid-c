# Asteroid game in C

**Work in progress. Still many things missing and broken**

Small Asteroids game written in C using the Raylib library

## Installation and usage 

1. Install the **Raylib** library by following the official documentation: https://github.com/raysan5/raylib?tab=readme-ov-file#build-and-installation 

2. Clone the repository

### Linux or macOS

On Linux or macOS **systems where raylib is available via `pkg-config`**, you can build and run the project using the provided helper script:

```bash
./build-and-run.sh
```

### Other platforms / setups

If this does not work (for example on Windows, or with a raylib installation that does not provide pkg-config), you will need to build the project manually using your preferred toolchain (e.g. CMake or an IDE).

See the official raylib repository for platform-specific installation and build instructions:
https://github.com/raysan5/raylib#build-and-installation