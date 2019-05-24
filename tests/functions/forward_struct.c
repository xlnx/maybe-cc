typedef struct A
{
	struct A *next;
} A;

int main()
{
	A a, *p = &a;
	p->next = p;
	p->next->next = p;
}