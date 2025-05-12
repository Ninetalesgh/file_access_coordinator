#!/bin/bash


PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

MAIN_PATH="$PROJECT_DIR/modules/ssh_native/main.cpp"

OUT_PATH="$PROJECT_DIR/export/fac_cli"

PLATFORM=""
if [[ "$(uname)" =~ ^Linux$ ]]; then
	PLATFORM="linux"
else
	PLATFORM="windows"
fi

INCLUDE_AND_LIBS=""

if [ "$PLATFORM" = "windows" ]; then
	OUT_PATH="$OUT_PATH.exe"
	INCLUDE_AND_LIBS="-I/c/src/include -L/c/src/lib/libssh -lssh"
elif [ "$PLATFORM" = "linux" ]; then
	INCLUDE_AND_LIBS="-I/usr/include -lssh"
fi

g++ "$MAIN_PATH" -o "$OUT_PATH" -I/usr/include -lssh
