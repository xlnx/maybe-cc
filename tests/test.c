#include <stdio.h>

extern void fn( int in, int *out );

int main()
{
	int in, out;
	in = 128;
	fn( in, &out );
	printf( "%d\n", *out );
}
