int pack[4] = {21, 32, 43, 54};
char ips[128];

extern int sum_get() {
	return pack[0] + pack[1] + pack[2] + pack[3];
}

extern int array_get() {
	return (int)&pack[0];
}

static void write_octet(char **p, int val) {
	if (val >= 100) {
		*(*p)++ = '0' + (val / 100);
		*(*p)++ = '0' + (val / 10) % 10;
		*(*p)++ = '0' + val % 10;
	} else if (val >= 10) {
		*(*p)++ = '0' + (val / 10);
		*(*p)++ = '0' + val % 10;
	} else {
		*(*p)++ = '0' + val;
	}
}

static void write_ip(char **p, int ip) {
	write_octet(p, (ip >> 24) & 0xFF);  *(*p)++ = '.';
	write_octet(p, (ip >> 16) & 0xFF);  *(*p)++ = '.';
	write_octet(p, (ip >>  8) & 0xFF);  *(*p)++ = '.';
	write_octet(p, (ip >>  0) & 0xFF);
}

extern int addr_ips(int src, int dst) {
	char *p = ips;

    write_ip(&p, src);
    *p++ = ' ';
    write_ip(&p, dst);
    *p = '\0';

	return (int)&ips[0];
}