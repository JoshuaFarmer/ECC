# Embedded C Compiler
mkdir -p bin
gcc src/main.c -o bin/ec -Os -Wall -Wextra -Wno-implicit-int
./bin/ec -s example.c -o bin/output.s -m LINUX
nasm bin/output.s -o bin/output.o -f elf32
ld bin/output.o -o bin/output -m elf_i386
./bin/output
echo $?