
#!/bin/bash

TARGET="$1"
DEBUG_SYMBOLS="no"
OPTIMIZE="auto"

if [ "$TARGET" = "" ]; then 
	TARGET="editor"
elif [ "$TARGET" = "template_release" ]; then
	OPTIMIZE="size"
#elif [ "$TARGET" = "template_debug" ]; then	
fi

PLATFORM=""
if [[ "$(uname)" =~ ^Linux$ ]]; then
	PLATFORM="linux"
else
	PLATFORM="windows"
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GODOT_SOURCE_DIR="$(cd "$PROJECT_DIR/../godot.4.4.2" && pwd)"

MODULE_ROOT_DIR="$PROJECT_DIR/modules"
BIN_DIR="$PROJECT_DIR/bin"

CUSTOM_MODULE_DIRS=()
for dir in $MODULE_ROOT_DIR/*/; do
	[ -d "$dir" ] && CUSTOM_MODULE_DIRS+=("$dir")
done


echo ""
echo "========================================="
echo "--- Building File Access Coordinator ----"
echo "-----------------------------------------"
echo "- Expected Project Source: $PROJECT_DIR"
echo "- Expected Godot Source: $GODOT_SOURCE_DIR"
echo "-----------------------------------------"
echo "- Collecting custom modules.."
echo $CUSTOM_MODULE_DIRS
echo "-----------------------------------------"
echo "--- Running scons with custom_modules ---"
echo "-----------------------------------------"
echo "- Platform: $PLATFORM" 
echo "- Target: $TARGET" 
echo "- Debug Symbols: $DEBUG_SYMBOLS" 
echo "- Optimize: $OPTIMIZE" 
pushd $GODOT_SOURCE_DIR
echo "-----------------------------------------"
end_cosmetic_underline(){
echo "-----------------------------------------"
echo "========================================="
echo ""
}


echo "External modules expected at: " $MODULE_ROOT_DIR
scons platform=$PLATFORM target=$TARGET debug_symbols=$DEBUG_SYMBOLS custom_modules=$CUSTOM_MODULE_DIRS optimize=$OPTIMIZE
status=$?
popd

if [ $status -ne 0 ]; then
	echo "-----------------------------------------"
	echo "- Build Errors, exiting."
	end_cosmetic_underline
	exit 1
fi

CONFIGURATION=""
ARCH=x86_64

OUT_FILENAME=""
if [ "$TARGET" = "editor" ]; then 
	OUT_FILENAME="editor"; 
else 
	OUT_FILENAME="godot.${PLATFORM}.${TARGET}.$ARCH"
fi

PLATFORM_EXTRA="$PLATFORM"
if [ "$PLATFORM_EXTRA" = "linux" ]; then 
	PLATFORM_EXTRA="linuxbsd"
elif [ "$PLATFORM_EXTRA" = "windows" ]; then
	OUT_FILENAME="$OUT_FILENAME.exe"
fi

mkdir -p bin
echo "-----------------------------------------"

if [ -e "$GODOT_SOURCE_DIR/bin/godot.$PLATFORM_EXTRA.$TARGET.$ARCH" ]; then
	echo "- Result: $BIN_DIR/$OUT_FILENAME"
	cp -f "$GODOT_SOURCE_DIR/bin/godot.$PLATFORM_EXTRA.$TARGET.$ARCH" "$BIN_DIR/$OUT_FILENAME"
else
	echo "- Error: No output binary was created to copy into the project directory."
fi

end_cosmetic_underline
