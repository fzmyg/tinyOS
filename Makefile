LD := ld
LDFLAGES := -melf_i386 -Ttext 0xc0001500 -emain
CC := gcc
CFLAGES := -c -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -m32 -g -O0 -fno-stack-protector
AS := nasm
ASFLAGES := -f elf32 -g
#kernel options
KERNEL_INCLUDE_DIR := ./src/kernel/include
C_SOURCE := ${shell find ./src/kernel/ -name  *.c}
C_OBJECT := ${patsubst ./src/kernel/%.c,./src/kernel/bin/%.o,${C_SOURCE}}
S_SOURCE := ${shell find ./src/kernel/ -name  *.s}
S_OBJECT := ${patsubst ./src/kernel/%.s,./src/kernel/bin/%.o,${S_SOURCE}}

#lib options
LIB_KERNEL_DIR := ./src/lib/kernel
LIB_KERNEL_INCLUDE_DIR := ${LIB_KERNEL_DIR}/include
LIB_KERNEL_SRC_S := ${shell find ./src/lib/kernel/src -name *.s}
LIB_KERNEL_SRC_C := ${shell find ./src/lib/kernel/src -name *.c}
LIB_KERNEL_SRC_BIN_S :=${patsubst ./src/lib/kernel/src/%.s,./src/lib/kernel/bin/%.o,${LIB_KERNEL_SRC_S}}
LIB_KERNEL_SRC_BIN_C :=${patsubst ./src/lib/kernel/src/%.c,./src/lib/kernel/bin/%.o,${LIB_KERNEL_SRC_C}}
LIB_KERNEL_SRC_BIN := ${LIB_KERNEL_SRC_BIN_S} ${LIB_KERNEL_SRC_BIN_C}

LIB_USER_DIR := ./src/lib/user
LIB_USER_INCLUDE_DIR := ${LIB_USER_DIR}/include
LIB_USER_SRC_S := ${shell find ./src/lib/user/src -name *.s}
LIB_USER_SRC_C := ${shell find ./src/lib/user/src -name *.c}
LIB_USER_SRC_BIN_S :=${patsubst ./src/lib/user/src/%.s,./src/lib/user/bin/%.o,${LIB_USER_SRC_S}}
LIB_USER_SRC_BIN_C :=${patsubst ./src/lib/user/src/%.c,./src/lib/user/bin/%.o,${LIB_USER_SRC_C}}
LIB_USER_SRC_BIN := ${LIB_USER_SRC_BIN_S} ${LIB_USER_SRC_BIN_C}

INCLUDE_DIR := -I${LIB_KERNEL_INCLUDE_DIR} -I${LIB_USER_INCLUDE_DIR} -I${KERNEL_INCLUDE_DIR}
.PHONY: boot
boot:
	./bin/bochs -f bochsrc


.PHONY: install_mbr
install_mbr:
	@echo "######################################################################################################################################################################################################################"
	@echo "                                                                                     compile mbr                                   "
	@echo "######################################################################################################################################################################################################################"
	@mkdir -p ./src/bin
	${AS} ./src/boot/mbr.s -o./src/bin/mbr.bin -I./src/boot/
	dd if=./src/bin/mbr.bin of=./hd60M.img bs=512 count=1 seek=0 conv=notrunc


.PHONY: install_loader
install_loader:
	@echo "######################################################################################################################################################################################################################"
	@echo "                                                                                    compile loader                                   "
	@echo "######################################################################################################################################################################################################################"
	@mkdir -p ./src/bin
	${AS} ./src/boot/loader.s -o ./src/bin/loader.bin  -I./src/boot/
	dd if=./src/bin/loader.bin of=./hd60M.img bs=512 count=4 seek=2 conv=notrunc


.PHONY: install_kernel compile_oskernel
compile_oskernel:
	@echo "######################################################################################################################################################################################################################"
	@echo "                                                                                   compile os_kernel                                  "
	@echo "######################################################################################################################################################################################################################"
	
install_kernel: compile_oskernel os_kernel
	dd if=./src/bin/os_kernel of=./hd60M.img bs=512 count=400 seek=10 conv=notrunc
os_kernel:${C_OBJECT} ${S_OBJECT}
	${LD} src/kernel/bin/main.o src/kernel/bin/init.o src/kernel/bin/interrupt.o src/kernel/bin/kernel.o src/kernel/bin/timer.o src/kernel/bin/fork.o src/kernel/bin/memory.o ./src/kernel/bin/thread.o ./src/kernel/bin/switch.o ./src/kernel/bin/sync.o ./src/kernel/bin/console.o ./src/kernel/bin/keyboard.o ./src/kernel/bin/ioqueue.o ./src/kernel/bin/tss.o  ./src/kernel/bin/syscall_init.o ./src/kernel/bin/syscall.o  ./src/kernel/bin/process.o ./src/kernel/bin/shell.o ./src/kernel/bin/buildin.o ./src/kernel/bin/ide.o  ./src/kernel/bin/fs.o ./src/kernel/bin/inode.o ./src/kernel/bin/file.o ./src/kernel/bin/dir.o ./src/lib/user/bin/stdio.o ./src/lib/kernel/bin/stdiok.o ./src/lib/kernel/bin/print.o ./src/lib/kernel/bin/debug.o ./src/lib/kernel/bin/string.o -o ./src/bin/os_kernel ./src/lib/kernel/bin/bitmap.o ./src/lib/kernel/bin/list.o ${LDFLAGES}
./src/kernel/bin/%.o:./src/kernel/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/kernel/bin/%.o:./src/kernel/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@


.PHONY: lib compile_lib
compile_lib:
	@echo "######################################################################################################################################################################################################################"
	@echo "                                                                                   compile lib                                  "
	@echo "######################################################################################################################################################################################################################"
lib: compile_lib ${LIB_KERNEL_SRC_BIN}  ${LIB_USER_SRC_BIN}
	@echo "generate lib file :${LIB_KERNEL_SRC_BIN} ${LIB_USER_SRC_BIN}"
#compilel kernel's lib
${LIB_KERNEL_DIR}/bin/%.o:${LIB_KERNEL_DIR}/src/%.s
	@mkdir -p ${dir $@}
	${AS} ${ASFLAGES} -o$@ $<
${LIB_KERNEL_DIR}/bin/%.o:${LIB_KERNEL_DIR}/src/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES}  ${INCLUDE_DIR} -o$@  $<
#compile user's lib
${LIB_USER_DIR}/bin/%.o:${LIB_USER_DIR}/src/%.s
	@mkdir -p ${dir $@}
	${AS} ${ASFLAGES} -o$@ $<
${LIB_USER_DIR}/bin/%.o:${LIB_USER_DIR}/src/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES}  ${INCLUDE_DIR} -o$@  $<


.PHONY:all 
all: install_mbr install_loader lib install_kernel 

.PHONY: clean
clean:	
	rm -rf ./src/bin/* ./src/bin/*.o ./src/kernel/bin/*.o ${LIB_KERNEL_DIR}/bin/*.o ${LIB_USER_DIR}/bin/*.o

.PHONY:
clear_disk:
	rm hd80M.img
	cp hd80M.bk hd80M.img
