
#!/bin/bash

TARGET="$1"
OUT_FILENAME="fac"
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
GODOT_SOURCE_DIR="$(cd "$PROJECT_DIR/../godot" && pwd)"

MODULE_ROOT_DIR="$PROJECT_DIR/modules"


CUSTOM_MODULE_DIRS=()
for dir in $MODULE_ROOT_DIR/*/; do
	[ -d "$dir" ] && CUSTOM_MODULE_DIRS+=("$dir")
done


echo ""
echo "========================================="
echo "--- Building Mythmakers -----------------"
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
echo "-----------------------------------------"
echo "--- Pushing godot source dir ------------"
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

CONFIGURATION=""
GODOT_VERSION="4.4.2rc"
ARCH=x86_64

if [ "$TARGET" = "editor" ]; then 
	OUT_FILENAME="editor"; 
else 
	if [ "$TARGET" = "template_release" ]; then
		CONFIGURATION="release"
	elif [ "$TARGET" = "template_debug" ]; then
		CONFIGURATION="debug"
	fi
	OUT_FILENAME="${PLATFORM}_${CONFIGURATION}.$ARCH"
fi

if [ "$PLATFORM" = "linux" ]; then 
	PLATFORM="linuxbsd"
fi

mkdir -p bin
echo "-----------------------------------------"

if [ -e "$GODOT_SOURCE_DIR/bin/godot.$PLATFORM.$TARGET.$ARCH" ]; then
	echo "- Writing result to: $PROJECT_DIR/$OUT_FILENAME"
	cp -f "$GODOT_SOURCE_DIR/bin/godot.$PLATFORM.$TARGET.$ARCH" "$PROJECT_DIR/$OUT_FILENAME"
else
	echo "- Error: No output binary was created to copy into the project directory."
fi

if [ $status -ne 0 ]; then
	echo "-----------------------------------------"
	echo "- Error building, aborting."
	end_cosmetic_underline
	exit 1
fi
end_cosmetic_underline
