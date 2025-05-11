#!/bin/bash


OUT_FILENAME=fac
CONFIG="$1"

#config should be either debug or release
if [ "$CONFIG" = "" ]; then 
	CONFIG="release"
fi

BINARY_POSTFIX=""
PLATFORM=""
if [[ "$(uname)" =~ ^Linux$ ]]; then
	PLATFORM="linux"
else
	PLATFORM="windows"
	BINARY_POSTFIX=".exe"
fi

ARCH=x86_64

exit_early(){

	exit 1
}

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN_DIR="$PROJECT_DIR/bin"

EDITOR_DIR="$BIN_DIR/editor$BINARY_POSTFIX"
EXPORT_DIR="$PROJECT_DIR/export"
TEMPLATE_DIR="$BIN_DIR/godot.${PLATFORM}.template_$CONFIG.$ARCH$BINARY_POSTFIX"

if [ ! -f "$EDITOR_DIR" ]; then
	bash build_editor.sh
	status=$?
fi

if [ $status -ne 0 ]; then
	exit_early
fi

if [ ! -f "$TEMPLATE_DIR" ]; then
	bash build_template_$CONFIG.sh
	status=$?
fi

if [ $status -ne 0 ]; then
	exit_early
fi

if [ "$CONFIG" = "debug" ]; then
	OUT_FILENAME="${OUT_FILENAME}_debug"
fi

if [ "$PLATFORM" = "windows" ]; then
	CUSTOM="Windows"
	OUT_FILENAME="$OUT_FILENAME.exe"
elif [ "$PLATFORM" = "linux" ]; then
	CUSTOM="Linux"
fi

"$EDITOR_DIR" --export-$CONFIG "$CUSTOM" "$EXPORT_DIR/$OUT_FILENAME"



