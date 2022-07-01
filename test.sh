FLAGS='-O0 -g -DDEBUG_MODE'
WARNINGS='-Winline'
FILES="./src/test_main.c ./src/memory.c"

clang $WARNINGS $FLAGS $FILES -o test.out &&
./test.out


