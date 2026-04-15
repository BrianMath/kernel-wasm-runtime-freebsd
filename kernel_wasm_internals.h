
#ifndef kprobes_helper_h
#define kprobes_helper_h

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/hashtable.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>

#define INVOKED_PRE "pre"
#define INVOKED_POST "post"
#define INVOKED_REPORT "report"
#define KPROBE_HEADER "kprobe:"

#define SPLIT_SIZE 32
#define SYMBOL_SIZE 64
#define MAX_PROBES 16

#define MAX_MODULES 16
static struct module_entry* module_entries[MAX_MODULES] = {0};

DEFINE_HASHTABLE(kprobe_map, 6);

// used in the kprobe_map hashtable
struct kprobe_map_entry {
    char symbol_name[SYMBOL_SIZE];
    struct kprobe kp;
    int count;
    struct hlist_node node;
};

// static struct kprobe hook;
struct probe_entry {
    int id;
    char symbol_name[SYMBOL_SIZE];
    struct module_entry* owner;
    // would it be smarter to put this in a union in the future?
    // note that you will need a flag identifier or something to make that work.
    IM3Function wasm_pre_func;
    IM3Function wasm_post_func;
};

struct module_entry {
    int id;
    char* comm;
    IM3Environment env;
    IM3Runtime runtime;
    IM3Module module;
    IM3Function wasm_report_func;
};


static struct probe_entry* probe_table[MAX_PROBES];

static int setup_wasm(void);

static int pre_handler(struct kprobe* p, struct pt_regs* regs);
static void post_handler(struct kprobe* p, struct pt_regs* regs, unsigned long flags);
static void report(void);
static int sign_hook(struct kprobe_map_entry*);

static struct kprobe_map_entry* lookup_kprobe_entry(const char* symbol);
static int add_kprobe_entry(const char* symbol);
// static int signHooks(unsigned long syscall_open, unsigned long syscall_write, long syscall_read);
// static int setPointers(unsigned long * syscall_open, unsigned long * syscall_write, unsigned long * syscall_read);

#endif