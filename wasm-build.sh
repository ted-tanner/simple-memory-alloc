source .env
OUTPUT_LOC="$OUTPUT_DIR/game.wasm"
BUILD_SPECIFIC_FILES=''

$COMPILER -D__WASM__ --target=wasm32 -nostdlib -Wl,--no-ntry -Wl,--export-all $WARNINGS $FLAGS $FILES $BUILD_SPECIFIC_FILES -o $OUTPUT_LOC && $OUTPUT_LOC $@
