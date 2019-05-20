typedef int arr_t[ 10 ];
typedef const int *cint_ptr_t;

int main()
{
	const int *p;
	volatile int *q = p;

	const arr_t a; /* const int a[10]; */
	int *r = &a[ 1 ];

	const cint_ptr_t *val_p;	   /* const int *const *vals   */
	int *const *val_ptr_p = val_p; /*       int *const *       */

	const cint_ptr_t val_a[ 2 ];   /* const int *const vals[2] */
	int *const *val_ptr_a = val_a; /*       int *const *       */
}