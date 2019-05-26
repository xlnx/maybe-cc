int main()
{
	int *p;
	void *q;
	p = q;
	q = p;
	p = (void *)0;
	q = (void *)0;
}
