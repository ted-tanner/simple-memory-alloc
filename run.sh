FLAGS='-O0 -g -DDEBUG_MODE'
WARNINGS='-Winline'
FILES="./src/main.c ./src/memory.c"

OUTPUT_LOC="./out/game.out"

clang $WARNINGS $FLAGS $FILES -o $OUTPUT_LOC && $OUTPUT_LOC
