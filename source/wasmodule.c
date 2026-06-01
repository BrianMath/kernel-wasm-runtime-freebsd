// Device driver
#include <sys/types.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/module.h>
#include <sys/kernel.h> /* types used in module initialization */

#include "wasm3.h"
#include "m3_env.h"
#include "add.h"
#include "fib.h"

m3ApiRawFunction(host_sum) {
    m3ApiReturnType(int32_t);
    m3ApiGetArg(int32_t, a);
    m3ApiGetArg(int32_t, b);
    
    int32_t c = a + b;
    printf("a = %d, b = %d, c = %d\n", a, b, c);
    
    m3ApiReturn(c);
}

int loader() {
    M3Result result;
    // M3Result result2;

    /* 1. Create Wasm3 environment */
    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        printf("Error creating the environment.\n");
        return 1;
    }

    // IM3Environment env2 = m3_NewEnvironment();
    // if (!env2) {
    //     printf("Erro ao criar o ambiente 2.\n");
    //     return 1;
    // }

    /* 2. Create runtime. The second argument is the stack size in bytes */
    IM3Runtime runtime = m3_NewRuntime(env, 8192, NULL);
    if (!runtime) {
        printf("Error creating the runtime.\n");
        m3_FreeEnvironment(env);
        return 1;
    }

    // IM3Runtime runtime2 = m3_NewRuntime(env2, 8192, NULL);
    // if (!runtime2) {
    //     printf("Erro ao criar o runtime 2.\n");
    //     m3_FreeEnvironment(env2);
    //     return 1;
    // }

    /* 3. Parse the WebAssembly binary */
    IM3Module module;

    result = m3_ParseModule(env, &module, add_wasm, add_wasm_len);
    if (result) {
        printf("Parsing error: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    // IM3Module module2;
    // result2 = m3_ParseModule(env2, &module2, fib_wasm, fib_wasm_len);
    // if (result2) {
    //     printf("Erro no parse: %s\n", result2);
    //     m3_FreeRuntime(runtime2);
    //     m3_FreeEnvironment(env2);
    //     return 1;
    // }

    /* 4. Load the module in runtime */
    result = m3_LoadModule(runtime, module);
    if (result) {
        printf("Error loading module: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    /* 5. Link the raw function in the module */
    result = m3_LinkRawFunction(module, "env", "sum_c", "i(ii)", &host_sum);
    if (result) {
        printf("Erro no link: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    // result2 = m3_LoadModule(runtime2, module2);
    // if (result2) {
    //     printf("Erro ao carregar o módulo 2: %s\n", result2);
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

    // IM3Function func2;
    // result2 = m3_FindFunction(&func2, runtime2, "fib");
    // if (!result2) {
    //     /* A função é chamada. m3_CallV aceita argumentos variáveis baseados na assinatura */
	// 	printf("chamando função 2\n");
    //     result2 = m3_CallV(func2, 10);
    //     if (result2) {
    //         printf("Erro na execução da função 2: %s\n", result2);
    //     } else {
	// 		int retorno2;
	// 		printf("pegando resultado 2\n");
	// 		result2 = m3_GetResultsV(func2, &retorno2);
	// 		if (!result2) {
	// 			printf("Fib(10) = %d\n", retorno2);
	// 		}
	// 	}
    // }

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
		break;
	case MOD_UNLOAD:            // kldunload
		printf("=== Wasmodule KLD unloaded ===\n");
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
