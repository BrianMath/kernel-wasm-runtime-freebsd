// Device driver
#include <sys/types.h>
#include <sys/systm.h>  /* uprintf */
#include <sys/errno.h>
#include <sys/param.h>  /* defines used in kernel.h */
#include <sys/module.h>
#include <sys/kernel.h> /* types used in module initialization */

static int
wasmodule_loader(struct module *m, int what, void *arg)
{
	int err = 0;

	switch (what) {
	case MOD_LOAD:                /* kldload */
		uprintf("Wasmodule KLD loaded.\n");
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

