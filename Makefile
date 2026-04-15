.PHONY: all clean

SRC_C := kernel_wasm_entry.c wasm_chardev.c source/m3_parse.c source/m3_module.c source/m3_core.c source/m3_function.c source/m3_api_kernel.c source/m3_bind.c\
		source/m3_env.c source/m3_code.c source/m3_compile.c source/m3_exec.c source/m3_info.c source/m3_api_libc.c

ccflags-y := -std=gnu17 -O3 -g0 -s -Isource -Dd_m3HasWASI -lm -Wno-error 

obj-m += kernel_wasm.o 
kernel_wasm-objs := $(SRC_C:.c=.o)

PWD := $(CURDIR) 


all: 

	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules 

 

clean: 

	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

