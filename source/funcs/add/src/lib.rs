mod bindings;

unsafe extern "C" {
    fn sum_c(x: i32, y: i32) -> i32;
    fn copy_ip(addr: *bindings::ip);
}

unsafe extern "C" {
    fn log_c(ptr: u32, len: u32);
}

#[unsafe(no_mangle)]
pub extern "C" fn add(left: u64, right: u64) -> u64 {
    left + right
}

#[unsafe(no_mangle)]
pub extern "C" fn sum(left: i32, right: i32) -> i32 {
    unsafe {
        return sum_c(left, right);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn log(ptr: u32, len: u32) {
    unsafe {
        let packet: bindings::ip;

        copy_ip(&packet as *bindings::ip);

        return log_c(ptr, len);
    }
}
