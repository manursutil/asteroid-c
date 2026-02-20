cd ~/raylib/src && make PLATFORM=PLATFORM_WEB -B

make PLATFORM=PLATFORM_WEB RAYLIB_PATH=~/raylib EMSDK_PATH=~/emsdk

python3 -m http.server 8080