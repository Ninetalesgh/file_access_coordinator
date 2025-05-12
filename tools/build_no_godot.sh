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

if [ "$PLATFORM" = "windows" ]; then
	OUT_PATH="$OUT_PATH.exe"
elif [ "$PLATFORM" = "linux" ]; then
	OUT_PATH="$OUT_PATH"
fi

g++ "$MAIN_PATH" -o "$OUT_PATH" -I/usr/include -lssh
