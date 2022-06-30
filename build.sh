FLAGS='-O0 -g -DDEBUG_MODE'
WARNINGS='-Winline'
FILES="./src/main.c ./src/memory.c"

clang $WARNINGS $FLAGS $FILES
