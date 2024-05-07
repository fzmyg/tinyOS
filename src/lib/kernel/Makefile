SRC_S := ${shell find ./src/ -name *.s}
SRC_C := ${shell find ./src/ -name *.c}
S_SRC_BIN :=${patsubst ./src/%.s,./bin/%.o,${SRC_S}}
C_SRC_BIN :=${patsubst ./src/%.c,./bin/%.o,${SRC_C}}
SRC_BIN := ${S_SRC_BIN} ${C_SRC_BIN}
.PHONY: compile
compile: ${SRC_BIN}
	@echo "generate file :${SRC_BIN}"
./bin/%.o:./src/%.s
	nasm -felf32 -o$@ $<
./bin/%.o:./src/%.c
	gcc -c -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -m32 -I./include  -I../../kernel/include -o$@  $<
.PHONY: clean 
clean:
	rm -rf ${SRC_BIN}
