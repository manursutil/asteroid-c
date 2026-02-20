#!/bin/sh
set -eu

if [ ! -x ./bin/asteroid ]; then
  echo "Missing ./bin/asteroid. Build first with ./build.sh"
  exit 1
fi

./bin/asteroid
