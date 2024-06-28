LD := ld
LDFLAGES := -melf_i386 -Ttext 0xc0001500 -emain
CC := gcc
CFLAGES := -c -Wall -fno-builtin -W -Wstrict-prototypes -Wmissing-prototypes -m32 -g -O0 -fno-stack-protector
AS := nasm
ASFLAGES := -f elf32 -g
#kernel options
KERNEL_INCLUDE_DIR := ./src/kernel
C_KERNEL_SOURCE := ${shell find ./src/kernel/ -name  *.c}
C_KERNEL_OBJECT := ${patsubst ./src/kernel/%.c,./src/build/%.o,${C_KERNEL_SOURCE}}
S_KERNEL_SOURCE := ${shell find ./src/kernel/ -name  *.s}
S_KERNEL_OBJECT := ${patsubst ./src/kernel/%.s,./src/build/%.o,${S_KERNEL_SOURCE}}

#device options
DEVICE_INCLUDE_DIR := ./src/device
C_DEVICE_SOURCE := ${shell find ./src/device/ -name  *.c}
C_DEVICE_OBJECT := ${patsubst ./src/device/%.c,./src/build/%.o,${C_DEVICE_SOURCE}}
S_DEVICE_SOURCE := ${shell find ./src/device/ -name  *.s}
S_DEVICE_OBJECT := ${patsubst ./src/device/%.s,./src/build/%.o,${S_DEVICE_SOURCE}}

#fs options
FS_INCLUDE_DIR := ./src/fs
C_FS_SOURCE := ${shell find ./src/fs/ -name  *.c}
C_FS_OBJECT := ${patsubst ./src/fs/%.c,./src/build/%.o,${C_FS_SOURCE}}
S_FS_SOURCE := ${shell find ./src/fs/ -name  *.s}
S_FS_OBJECT := ${patsubst ./src/fs/%.s,./src/build/%.o,${S_FS_SOURCE}}

#mm options
MM_INCLUDE_DIR := ./src/mm
C_MM_SOURCE := ${shell find ./src/mm/ -name  *.c}
C_MM_OBJECT := ${patsubst ./src/mm/%.c,./src/build/%.o,${C_MM_SOURCE}}
S_MM_SOURCE := ${shell find ./src/mm/ -name  *.s}
S_MM_OBJECT := ${patsubst ./src/mm/%.s,./src/build/%.o,${S_MM_SOURCE}}

#proc options
PROC_INCLUDE_DIR := ./src/proc
C_PROC_SOURCE := ${shell find ./src/proc/ -name  *.c}
C_PROC_OBJECT := ${patsubst ./src/proc/%.c,./src/build/%.o,${C_PROC_SOURCE}}
S_PROC_SOURCE := ${shell find ./src/proc/ -name  *.s}
S_PROC_OBJECT := ${patsubst ./src/proc/%.s,./src/build/%.o,${S_PROC_SOURCE}}

#shell options
SHELL_INCLUDE_DIR := ./src/shell
C_SHELL_SOURCE := ${shell find ./src/shell/ -name  *.c}
C_SHELL_OBJECT := ${patsubst ./src/shell/%.c,./src/build/%.o,${C_SHELL_SOURCE}}
S_SHELL_SOURCE := ${shell find ./src/shell/ -name  *.s}
S_SHELL_OBJECT := ${patsubst ./src/shell/%.s,./src/build/%.o,${S_SHELL_SOURCE}}

#thread options
THREAD_INCLUDE_DIR := ./src/thread
C_THREAD_SOURCE := ${shell find ./src/thread/ -name  *.c}
C_THREAD_OBJECT := ${patsubst ./src/thread/%.c,./src/build/%.o,${C_THREAD_SOURCE}}
S_THREAD_SOURCE := ${shell find ./src/thread/ -name  *.s}
S_THREAD_OBJECT := ${patsubst ./src/thread/%.s,./src/build/%.o,${S_THREAD_SOURCE}}

#total kernel obj file
C_OBJECT := ${C_KERNEL_OBJECT} ${C_DEVICE_OBJECT} ${C_FS_OBJECT} ${C_MM_OBJECT} ${C_THREAD_OBJECT} ${C_PROC_OBJECT} ${C_SHELL_OBJECT}
S_OBJECT := ${S_KERNEL_OBJECT} ${S_DEVICE_OBJECT} ${S_FS_OBJECT} ${S_MM_OBJECT} ${S_THREAD_OBJECT} ${S_PROC_OBJECT} ${S_SHELL_OBJECT}

#lib options
LIB_KERNEL_DIR := ./src/lib/kernel
LIB_KERNEL_INCLUDE_DIR := ${LIB_KERNEL_DIR}/include
LIB_KERNEL_SRC_S := ${shell find ./src/lib/kernel/src -name *.s}
LIB_KERNEL_SRC_C := ${shell find ./src/lib/kernel/src -name *.c}
LIB_KERNEL_SRC_BIN_S :=${patsubst ./src/lib/kernel/src/%.s,./src/build/%.o,${LIB_KERNEL_SRC_S}}
LIB_KERNEL_SRC_BIN_C :=${patsubst ./src/lib/kernel/src/%.c,./src/build/%.o,${LIB_KERNEL_SRC_C}}
LIB_KERNEL_SRC_BIN := ${LIB_KERNEL_SRC_BIN_S} ${LIB_KERNEL_SRC_BIN_C}

LIB_USER_DIR := ./src/lib/user
LIB_USER_INCLUDE_DIR := ${LIB_USER_DIR}/include
LIB_USER_SRC_S := ${shell find ./src/lib/user/src -name *.s}
LIB_USER_SRC_C := ${shell find ./src/lib/user/src -name *.c}
LIB_USER_SRC_BIN_S :=${patsubst ./src/lib/user/src/%.s,./src/build/%.o,${LIB_USER_SRC_S}}
LIB_USER_SRC_BIN_C :=${patsubst ./src/lib/user/src/%.c,./src/build/%.o,${LIB_USER_SRC_C}}
LIB_USER_SRC_BIN := ${LIB_USER_SRC_BIN_S} ${LIB_USER_SRC_BIN_C}

INCLUDE_DIR := -I${LIB_KERNEL_INCLUDE_DIR} -I${LIB_USER_INCLUDE_DIR} -I${KERNEL_INCLUDE_DIR} -I${DEVICE_INCLUDE_DIR} -I${FS_INCLUDE_DIR} -I${MM_INCLUDE_DIR} -I${PROC_INCLUDE_DIR} -I${SHELL_INCLUDE_DIR} -I${THREAD_INCLUDE_DIR}
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
	${LD} src/build/main.o src/build/init.o src/build/interrupt.o src/build/kernel.o src/build/timer.o src/build/fork.o src/build/exec.o src/build/wait_exit.o src/build/memory.o ./src/build/thread.o ./src/build/switch.o ./src/build/sync.o ./src/build/console.o ./src/build/keyboard.o ./src/build/ioqueue.o ./src/build/tss.o  ./src/build/syscall_init.o ./src/build/init_proc.o ./src/build/syscall.o  ./src/build/process.o ./src/build/shell.o ./src/build/buildin.o ./src/build/ide.o  ./src/build/fs.o ./src/build/inode.o ./src/build/file.o ./src/build/dir.o ./src/build/stdio.o ./src/build/stdiok.o ./src/build/print.o ./src/build/debug.o ./src/build/string.o  ./src/build/bitmap.o ./src/build/list.o  -o ./src/bin/os_kernel ${LDFLAGES}
#kernel
./src/build/%.o:./src/kernel/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/kernel/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#device
./src/build/%.o:./src/device/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/device/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#fs
./src/build/%.o:./src/fs/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/fs/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#mm
./src/build/%.o:./src/mm/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/mm/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#proc
./src/build/%.o:./src/proc/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/proc/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#shell
./src/build/%.o:./src/shell/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/shell/%.s
	@mkdir -p ${dir $@}
	${AS}  ${ASFLAGES}  $< -o $@
#thread
./src/build/%.o:./src/thread/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES} ${INCLUDE_DIR}   $< -o $@ 
./src/build/%.o:./src/thread/%.s
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
./src/build/%.o:${LIB_KERNEL_DIR}/src/%.s
	@mkdir -p ${dir $@}
	${AS} ${ASFLAGES} -o$@ $<
./src/build/%.o:${LIB_KERNEL_DIR}/src/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES}  ${INCLUDE_DIR} -o$@  $<
#compile user's lib
./src/build/%.o:${LIB_USER_DIR}/src/%.s
	@mkdir -p ${dir $@}
	${AS} ${ASFLAGES} -o$@ $<
./src/build/%.o:${LIB_USER_DIR}/src/%.c
	@mkdir -p ${dir $@}
	${CC} ${CFLAGES}  ${INCLUDE_DIR} -o$@  $<


.PHONY:all 
all: install_mbr install_loader lib install_kernel 

.PHONY: clean
clean:	
	rm -rf ./src/build/* ./src/bin/*

.PHONY:clear_disk
clear_disk:
	rm hd80M.img
	cp hd80M.bk hd80M.img

.PHONY:usrp
USER_INCLUDE_DIR := -I./src/lib/user/include -I./src/lib/kernel/include -I./src/kernel
usrp:./src/usrp/crt.s
	@mkdir -p ./src/usrp/build
	${AS} ./src/usrp/crt.s  -o ./src/usrp/build/crt.o ${ASFLAGES}
	${CC} ./src/usrp/vim.c -o ./src/usrp/build/vim.o ${USER_INCLUDE_DIR} ${CFLAGES}
	${LD} -e _start  -Ttext 0x8048080 -o ./src/usrp/build/vim ./src/usrp/build/crt.o ./src/usrp/build/vim.o ./src/build/string.o ./src/build/stdio.o ./src/build/syscall.o ./src/build/assert.o ./src/build/array.o
	dd if=./src/usrp/build/vim of=./hd60M.img bs=512 count=58 seek=400 conv=notrunc


