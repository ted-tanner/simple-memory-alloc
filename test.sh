FLAGS='-O0 -g -DDEBUG_MODE -DTEST_MODE'
WARNINGS='-Winline'
FILES="./src/test_main.c ./src/memory.c ./src/test.c"

OUTPUT_LOC="./out/test.out"

clang $WARNINGS $FLAGS $FILES -o $OUTPUT_LOC && $OUTPUT_LOC


