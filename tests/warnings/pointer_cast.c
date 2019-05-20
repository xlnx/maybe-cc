struct A;

int main()
{
	int *p, a;
	p = a;	/* pointer-integer cast */
	p = main; /* pointer-pointer cast */
	struct A *x = p;
}