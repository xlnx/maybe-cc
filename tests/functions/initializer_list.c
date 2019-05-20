#include <stdio.h>

void print_array( int *ptr, int size )
{
	int *p;
	for ( p = ptr; p != ptr + size; ++p )
	{
		printf( "%d ", *p );
	}
	puts( "" );
}

int main()
{
	int a[] = {
		1, 2, 3, 4, 5
	};
	print_array( a, sizeof( a ) / sizeof( a[ 0 ] ) );
}