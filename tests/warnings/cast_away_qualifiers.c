typedef int arr_t[ 10 ];
typedef const int *cint_ptr_t;

int main()
{
	const int *p;
	volatile int *q = p;

	const arr_t a; /* const int a[10]; */
	int *r = &a[ 1 ];

	const cint_ptr_t *vals;		/* const int *const *vals */
	int *const *val_ptr = vals; /*       int *const *     */
}