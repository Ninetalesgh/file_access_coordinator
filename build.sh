
#!/bin/bash

TARGET="$1"
OUT_FILENAME="fac"
DEBUG_SYMBOLS="no"
OPTIMIZE="auto"

if [ ! "$TARGET" = "editor" ]; then 
	TARGET="template_release"
	OPTIMIZE="size"
fi

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"
GODOT_SOURCE_DIR="$SCRIPT_DIR/../godot"

PLATFORM=""
if [[ "$(uname)" =~ ^Linux$ ]]; then
	PLATFORM="linux"
else
	PLATFORM="windows"
fi

MODULE_ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/modules"
CUSTOM_MODULE_DIRS=()

for dir in $MODULE_ROOT_DIR/*/; do
	[ -d "$dir" ] && CUSTOM_MODULE_DIRS+=("$dir")
done


echo ""
echo "========================================="
echo "--- Building Mythmakers -----------------"
echo "-----------------------------------------"
echo "- Godot Source: $GODOT_SOURCE_DIR"
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


echo "External modules expected at: " $SCRIPT_DIR
scons platform=$PLATFORM target=$TARGET debug_symbols=$DEBUG_SYMBOLS custom_modules=$CUSTOM_MODULE_DIRS optimize=$OPTIMIZE
status=$?
popd

ARCH=x86_64
if [ "$PLATFORM" = "linux" ]; then PLATFORM="linuxbsd"; fi
if [ "$TARGET" = "editor" ]; then OUT_FILENAME="${OUT_FILENAME}_editor"; fi

mkdir -p bin
cp -f "$GODOT_SOURCE_DIR/bin/godot.$PLATFORM.$TARGET.$ARCH" "bin/$OUT_FILENAME"

if [ $status -ne 0 ]; then
	echo "-----------------------------------------"
	echo "- Error building, aborting."
	end_cosmetic_underline
	exit 1
fi
end_cosmetic_underline

# for this script to work, please find 
# SConscript("modules/SCsub") 
# in the godot source SConstruct file and add this right below it:

# external_module = ARGUMENTS.get("external_module", "")
# if external_module:
#   if os.path.isdir(external_module):
#     SConscript(os.path.join(external_module, "SCsub"))

