int pack[4] = {20, 20, 20, 7};

extern int sum_get() {
    return pack[0] + pack[1] + pack[2] + pack[3];
}

extern int* array_get() {
    return &pack[0];
}