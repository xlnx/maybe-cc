struct A;

int main()
{
	int *p, a;
	p = a;				   /* pointer-integer cast */
	p = main;			   /* pointer-pointer cast */
	float *y = (float *)p; /* explicit type cast suppresses warnings */
	struct A *x = p;
}