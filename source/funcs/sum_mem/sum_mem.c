int pack[4] = {21, 32, 43, 54};

extern int sum_get() {
	return pack[0] + pack[1] + pack[2] + pack[3];
}

extern int array_get() {
	return (int)&pack[0];
}