__attribute__((used)) char buffer[256];
__attribute__((used)) char comm[] = "fork_test";
__attribute__((used)) char report_buffer[256];
__attribute__((used)) unsigned int buffer_size = sizeof(buffer);

int counter = 0;

__attribute__((export_name("kprobe:__do_sys_vfork:pre")))
void pre() {
    counter++;
}

__attribute__((export_name("kprobe:__do_sys_vfork:post")))
void post() {}

void format_report(char *buf, int count) {
    // Write the static prefix
    const char *prefix = "fork was called ";
    const char *suffix = " times\n";

    char *ptr = buf;

    // Copy prefix
    while (*prefix) *ptr++ = *prefix++;

    // Convert integer to string
    if (count == 0) {
        *ptr++ = '0';
    } else {
        char numbuf[12];
        int i = 0;
        while (count > 0 && i < sizeof(numbuf) - 1) {
            numbuf[i++] = '0' + (count % 10);
            count /= 10;
        }
        // Reverse and write digits
        while (--i >= 0) {
            *ptr++ = numbuf[i];
        }
    }

    // Copy suffix
    while (*suffix) *ptr++ = *suffix++;

    *ptr = '\0';  // null-terminate
}


char* report() {
    format_report(report_buffer, counter);
    return report_buffer;
}