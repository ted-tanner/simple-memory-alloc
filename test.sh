FLAGS='-O0 -g -DDEBUG_MODE'
WARNINGS='-Winline'
FILES="./src/test_main.c ./src/memory.c"

OUTPUT_LOC="./out/test.out"

clang $WARNINGS $FLAGS $FILES -o $OUTPUT_LOC && $OUTPUT_LOC


