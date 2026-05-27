unsafe extern "C" {
    fn sum_c(x: i32, y: i32) -> i32;
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