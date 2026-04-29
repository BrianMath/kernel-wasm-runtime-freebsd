KMOD = wasmodule
SRCS = wasmodule.c m3_api_tracer.c m3_compile.c \
	   m3_code.c m3_core.c m3_env.c m3_exec.c \
	   m3_function.c m3_info.c m3_module.c \
	   m3_parse.c m3_bind.c m3_api_libc.c \
	   m3_api_kernel.c m3_api_meta_wasi.c \
	   m3_api_uvwasi.c m3_api_wasi.c

CWARNFLAGS += -Wno-cast-qual

clean:
	rm -f *.o

.include <bsd.kmod.mk>
