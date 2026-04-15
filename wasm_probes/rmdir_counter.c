int counter = 0;
char buffer[64];

__attribute__((export_name("kprobe:__x64_sys_rmdir:pre")))
void rm_pre() {
    counter++;
}

__attribute__((export_name("kprobe:__x64_sys_rmdir:post")))
void rm_post() {}

__attribute__((export_name("report")))
char* report() {
    buffer[0] = 'O';
    buffer[1] = 'k';
    buffer[2] = '\n';
    buffer[3] = '\0';
    return buffer;
}