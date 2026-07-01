// Device driver
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>

#include <sys/socket.h>
#include <sys/mbuf.h>
#include <net/if.h>
#include <net/pfil.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include "wasm3.h"
#include "m3_env.h"
#include "add.h"
#include "fib.h"
#include "sum_mem.h"

static pfil_hook_t my_hook;
static IM3Environment env2;
static IM3Runtime runtime2;
static IM3Module module2;

static IM3Environment env3;
static IM3Runtime runtime3;
static IM3Module module3;

static pfil_return_t
my_filter(struct mbuf **mp, struct ifnet *ifp, int dir, void *arg, struct inpcb *inp) {
	// Convert an mbuf pointer to a data pointer of the specified type
	struct ip *ip = mtod(*mp, struct ip *);

	// ntohl: big endian to little endian
	int src = ntohl(ip->ip_src.s_addr);
	int dst = ntohl(ip->ip_dst.s_addr);
	printf("|pfil: src=%d.%d.%d.%d dst=%d.%d.%d.%d|\n",
		(src & 0xFF000000) >> 24,
		(src & 0x00FF0000) >> 16,
		(src & 0x0000FF00) >> 8,
		src & 0x000000FF,
		(dst & 0xFF000000) >> 24,
		(dst & 0x00FF0000) >> 16,
		(dst & 0x0000FF00) >> 8,
		dst & 0x000000FF
	);

	return PFIL_PASS;
}

struct pfil_hook_args pha = {
	.pa_version  = PFIL_VERSION,
	.pa_flags    = PFIL_IN,
	.pa_type     = PFIL_TYPE_IP4,
	.pa_mbuf_chk = my_filter,
	.pa_ruleset  = NULL,
	.pa_modname  = "my_pfil",
	.pa_rulname  = "log"
};

struct pfil_link_args pla = {
	.pa_flags    = PFIL_IN,
	.pa_headname = PFIL_INET_NAME,
	.pa_modname  = "my_pfil",
	.pa_rulname  = "log"
};

m3ApiRawFunction(host_sum) {
    m3ApiReturnType(int32_t);
    m3ApiGetArg(int32_t, a);
    m3ApiGetArg(int32_t, b);
    
    int32_t c = a + b;
    printf("a = %d, b = %d, c = %d\n", a, b, c);
    
    m3ApiReturn(c);
}

// Signature: v(i i) = void(i32 ptr, i32 len)
m3ApiRawFunction(host_log) {
    m3ApiGetArg(uint32_t, ptr);
    m3ApiGetArg(uint32_t, len);
    
    // Get runtime to access linear memory
    uint32_t mem_size;
    uint8_t* memory = m3_GetMemory(runtime2, &mem_size, 0);
    
    if (!memory || ptr + len > mem_size) {
        m3ApiTrap("memory access out of bounds");
    }
    
    // Print string
    printf("[WASM LOG] %.*s\n", len, (char*)(memory + ptr));
    
    m3ApiSuccess();
}

int loader() {
    M3Result result;
    M3Result result2;
    M3Result result3;

    /* 1. Create Wasm3 environment */
    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        printf("Error creating the environment.\n");
        return 1;
    }

    env2 = m3_NewEnvironment();
    if (!env2) {
        printf("Error creating the environment 2.\n");
        return 1;
    }

    env3 = m3_NewEnvironment();
    if (!env2) {
        printf("Error creating the environment 3.\n");
        return 1;
    }

    /* 2. Create runtime. The second argument is the stack size in bytes */
    IM3Runtime runtime = m3_NewRuntime(env, 8192, NULL);
    if (!runtime) {
        printf("Error creating the runtime.\n");
        m3_FreeEnvironment(env);
        return 1;
    }

    runtime2 = m3_NewRuntime(env2, 8192, NULL);
    if (!runtime2) {
        printf("Error creating the runtime 2.\n");
        m3_FreeEnvironment(env2);
        return 1;
    }

    runtime3 = m3_NewRuntime(env3, 8192, NULL);
    if (!runtime3) {
        printf("Error creating the runtime 3.\n");
        m3_FreeEnvironment(env3);
        return 1;
    }

    /* 3. Parse the WebAssembly binary */
    IM3Module module;
    result = m3_ParseModule(env, &module, add_wasm, add_wasm_len);
    if (result) {
        printf("Parsing error: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    result2 = m3_ParseModule(env2, &module2, sum_mem_wasm, sum_mem_wasm_len);
    if (result2) {
        printf("Parsing error: %s\n", result2);
        m3_FreeRuntime(runtime2);
        m3_FreeEnvironment(env2);
        return 1;
    }

    result3 = m3_ParseModule(env3, &module3, sum_mem_wasm, sum_mem_wasm_len);
    if (result3) {
        printf("Parsing error: %s\n", result3);
        m3_FreeRuntime(runtime3);
        m3_FreeEnvironment(env3);
        return 1;
    }

    /* 4. Load the module in runtime */
    result = m3_LoadModule(runtime, module);
    if (result) {
        printf("Error loading module: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }
    
    result2 = m3_LoadModule(runtime2, module2);
    if (result2) {
        printf("Error loading module: %s\n", result2);
        m3_FreeRuntime(runtime2);
        m3_FreeEnvironment(env2);
        return 1;
    }

    result3 = m3_LoadModule(runtime3, module3);
    if (result3) {
        printf("Error loading module: %s\n", result3);
        m3_FreeRuntime(runtime3);
        m3_FreeEnvironment(env3);
        return 1;
    }

    /* 5. Link the raw function in the module */
    result = m3_LinkRawFunction(module, "env", "sum_c", "i(ii)", &host_sum);
    if (result) {
        printf("Error linking: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }
    
    // result2 = m3_LinkRawFunction(module, "env", "log", "v(ii)", &host_log);
    // if (result2) {
    //     printf("Error linking: %s\n", result2);
    //     m3_FreeRuntime(runtime2);
    //     m3_FreeEnvironment(env2);
    //     return 1;
    // }

    /* 6. Find and call an exported module function */
    IM3Function func;
    result = m3_FindFunction(&func, runtime, "sum");
    if (!result) {
        /* The function is called. m3_CallV accepts variable arguments based on the function signature */
        result = m3_CallV(func, 25, 42);
        if (result) {
            printf("Error calling the function: %s\n", result);
        } else {
			int func_return;
			result = m3_GetResultsV(func, &func_return);
			if (!result) {
				printf("Sum = %d\n", func_return);
			}
		}
    }

    uint32_t mem_size;
    uint8_t* memory = m3_GetMemory(runtime2, &mem_size, 0);

    if (memory) {
    	printf("Memory size: %u bytes\n", mem_size);
    	*(int32_t*)(memory + 400) = 0xBebaCafe;
    }

    IM3Function func2;
    result2 = m3_FindFunction(&func2, runtime2, "sum_get");
    if (!result2) {
        /* The function is called. m3_CallV accepts variable arguments based on the function signature */
        result2 = m3_CallV(func2);
        if (result2) {
            printf("Error calling function 2: %s\n", result2);
        } else {
            int func_return;
            result2 = m3_GetResultsV(func2, &func_return);
            // printf("get result\n");
            if (!result2) {
                printf("Sum = 0x%x\n", *(int*)(memory + 400));
                printf("Get = %d\n", func_return);
            }
        }
    }

    IM3Function array_get;
    result3 = m3_FindFunction(&array_get, runtime3, "array_get");
    if (!result3) {
        /* The function is called. m3_CallV accepts variable arguments based on the function signature */
        result3 = m3_CallV(array_get);
        if (result3) {
            printf("Error calling function array_get: %s\n", result3);
        } else {
            int func_return;
            result3 = m3_GetResultsV(array_get, &func_return);
            // printf("get result\n");
            if (!result3) {
                printf("&array[0] = %d\n", func_return);
                // printf("array[0] = %d\n", *func_return);
            }
        }
    }

    /* 7. Free allocated memory.
       Note: The module is automatically freed when either the runtime or the environment are freed */
    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);

    // m3_FreeRuntime(runtime2);
    // m3_FreeEnvironment(env2);

    return 0;
}

static int
wasmodule_loader(struct module *m, int what, void *arg)
{
	int err = 0;
	int r;

	switch (what) {
	case MOD_LOAD:                // kldload
		printf("=== Wasmodule KLD loaded ===\n");
		r = loader();
		if (r) {
			printf("Runtime error\n");
		} else {
			printf("No runtime error\n");
		}

        //my_hook = pfil_add_hook(&pha);
		//pfil_link(&pla);
		//printf("my_pfil: loaded\n");

		break;
	case MOD_UNLOAD:            // kldunload
		printf("=== Wasmodule KLD unloaded ===\n");

        //pfil_remove_hook(my_hook);
		//printf("my_pfil: unloaded\n");
        
        m3_FreeRuntime(runtime2);
        m3_FreeEnvironment(env2);

        m3_FreeRuntime(runtime3);
        m3_FreeEnvironment(env3);

		break;
	default:
		err = EOPNOTSUPP;
		break;
	}
	return(err);
}

/* Declare this module to the rest of the kernel */
static moduledata_t wasmodule_mod = {
	"wasmodule",
	wasmodule_loader,
	NULL
};

DECLARE_MODULE(wasmodule, wasmodule_mod, SI_SUB_KLD, SI_ORDER_ANY);
