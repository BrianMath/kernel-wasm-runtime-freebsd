#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wasm3.h"
#include "m3_env.h"

#include "sum.h"

int main()
{
    M3Result result;

    /* 1. Create Wasm3 environment */
    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        printf("Error creating the environment.\n");
        return 1;
    }

    /* 2. Create runtime. The second argument is the stack size in bytes */
    IM3Runtime runtime = m3_NewRuntime(env, 8192, NULL);
    if (!runtime) {
        printf("Error creating the runtime.\n");
        m3_FreeEnvironment(env);
        return 1;
    }

    /* 3. Parse the WebAssembly binary */
    IM3Module module;

    result = m3_ParseModule(env, &module, sum_wasm, sum_wasm_len);
    if (result) {
        printf("Parsing error: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
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

    /* 5. Link the raw function in the module */
    //result = m3_LinkRawFunction(module, "env", "sum_c", "i(ii)", &host_sum);
    //if (result) {
    //    printf("Erro no link: %s\n", result);
    //    m3_FreeRuntime(runtime);
    //    m3_FreeEnvironment(env);
    //    return 1;
    //}

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
    uint8_t* memory = m3_GetMemory(runtime, &mem_size, 0);

    if (memory) {
    	printf("Memory size: %u bytes\n", mem_size);
    	*(int32_t*)(memory + 400) = 0x41424344;
    }

    printf("Calling get\n");
    IM3Function myget;
    result = m3_FindFunction(&myget, runtime, "get");
    if (!result) {
        printf("Works!\n");

        /* The function is called. m3_CallV accepts variable arguments based on the function signature */
        result = m3_CallV(myget);
        if (result) {
            printf("Error calling the function: %s\n", result);
        } else {
            int func_return;
            result = m3_GetResultsV(myget, &func_return);
            printf("get result\n");
            if (!result) {
                printf("Get = %d\n", func_return);
                printf("Value: %d\n", *(int*)(memory + func_return));
            }
        }
    }

	    /* 7. Free allocated memory.
       Note: The module is automatically freed when either the runtime or the environment are freed */
    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);

    return 0;
}
