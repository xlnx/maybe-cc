#include <stdio.h>

int main()
{
	int a = 10, b = 100;

	printf( "%d\n", a > 10 && ++b );
	printf( "%d %d\n", a, b );

	printf( "%d\n", ++a > 10 && ++b );
	printf( "%d %d\n", a, b );
}