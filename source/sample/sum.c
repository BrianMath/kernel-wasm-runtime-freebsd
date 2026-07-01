int pack[2] = {41, 42};

extern int sum(int a, int b)
{
	return a+b;
}

extern int* get()
{
	//int *i = malloc(4);
	//return i;
	return &pack;
}

/*
extern int pos(int i)
{
	return vet[i];
}
*/
