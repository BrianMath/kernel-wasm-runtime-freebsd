// Device driver
#include <sys/types.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/module.h>
#include <sys/kernel.h> /* types used in module initialization */

#include "wasm3.h"
#include "add.h"
#include "fib.h"

int loader() {
    M3Result result;
    M3Result result2;

    /* 2. Criar o ambiente Wasm3 */
    IM3Environment env = m3_NewEnvironment();
    if (!env) {
        printf("Erro ao criar o ambiente.\n");
        return 1;
    }

    IM3Environment env2 = m3_NewEnvironment();
    if (!env2) {
        printf("Erro ao criar o ambiente 2.\n");
        return 1;
    }

    /* 3. Criar o runtime. O segundo argumento é o tamanho da stack em bytes (ex: 8192) */
    IM3Runtime runtime = m3_NewRuntime(env, 8192, NULL);
    if (!runtime) {
        printf("Erro ao criar o runtime.\n");
        m3_FreeEnvironment(env);
        return 1;
    }

    IM3Runtime runtime2 = m3_NewRuntime(env2, 8192, NULL);
    if (!runtime2) {
        printf("Erro ao criar o runtime 2.\n");
        m3_FreeEnvironment(env2);
        return 1;
    }

    /* 4. Fazer o parse do binário WebAssembly */
    IM3Module module;
    result = m3_ParseModule(env, &module, add_wasm, add_wasm_len);
    if (result) {
        printf("Erro no parse: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    IM3Module module2;
    result2 = m3_ParseModule(env2, &module2, fib_wasm, fib_wasm_len);
    if (result2) {
        printf("Erro no parse: %s\n", result2);
        m3_FreeRuntime(runtime2);
        m3_FreeEnvironment(env2);
        return 1;
    }

    /* 5. Carregar o módulo no runtime */
    result = m3_LoadModule(runtime, module);
    if (result) {
        printf("Erro ao carregar o módulo: %s\n", result);
        m3_FreeRuntime(runtime);
        m3_FreeEnvironment(env);
        return 1;
    }

    result2 = m3_LoadModule(runtime2, module2);
    if (result2) {
        printf("Erro ao carregar o módulo 2: %s\n", result2);
        m3_FreeRuntime(runtime2);
        m3_FreeEnvironment(env2);
        return 1;
    }

    /* 6. (Opcional) Localizar e executar uma função exportada pelo módulo */
    IM3Function func;
    result = m3_FindFunction(&func, runtime, "add");
    if (!result) {
        /* A função é chamada. m3_CallV aceita argumentos variáveis baseados na assinatura */
		printf("chamando função\n");
        result = m3_CallV(func, 25, 42);
        if (result) {
            printf("Erro na execução da função: %s\n", result);
        } else {
			int retorno;
			printf("pegando resultado\n");
			result = m3_GetResultsV(func, &retorno);
			if (!result) {
				printf("Soma = %d\n", retorno);
			}
		}
    }

    IM3Function func2;
    result2 = m3_FindFunction(&func2, runtime2, "fib");
    if (!result2) {
        /* A função é chamada. m3_CallV aceita argumentos variáveis baseados na assinatura */
		printf("chamando função 2\n");
        result2 = m3_CallV(func2, 10);
        if (result2) {
            printf("Erro na execução da função 2: %s\n", result2);
        } else {
			int retorno2;
			printf("pegando resultado 2\n");
			result2 = m3_GetResultsV(func2, &retorno2);
			if (!result2) {
				printf("Fib(10) = %d\n", retorno2);
			}
		}
    }

    /* 7. Liberar recursos alocados.
       Nota: O módulo é liberado automaticamente quando o runtime ou environment é liberado. */
    m3_FreeRuntime(runtime);
    m3_FreeEnvironment(env);

    m3_FreeRuntime(runtime2);
    m3_FreeEnvironment(env2);

    return 0;
}

static int
wasmodule_loader(struct module *m, int what, void *arg)
{
	int err = 0;
	int r;

	switch (what) {
	case MOD_LOAD:                /* kldload */
		uprintf("Wasmodule KLD loaded.\n");
		r = loader();
		if (r) {
			printf("erro na execução\n");
		} else {
			printf("!erro na execução\n");
		}
		break;
	case MOD_UNLOAD:
		uprintf("Wasmodule KLD unloaded.\n");
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

