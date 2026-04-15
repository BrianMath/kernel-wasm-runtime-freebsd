__attribute__((used)) char buffer[256];
__attribute__((used)) char report_buffer[256];
__attribute__((used)) unsigned int buffer_size = sizeof(buffer);

unsigned int mode = 0;

__attribute__((export_name("kprobe:__x64_sys_mkdir:pre")))
void pre(const char* pathname, unsigned short m) {
    mode = m;
}

__attribute__((export_name("kprobe:__x64_sys_mkdir:post")))
void post() {}

void format_report(char *buffer, int count) {
    // Write the static prefix
    const char *prefix = "The mode of the directory created is ";
    const char *suffix = "\n";

    char *ptr = buffer;

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
    format_report(buffer, mode);
    return buffer;
}