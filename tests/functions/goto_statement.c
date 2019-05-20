#include <stdio.h>

int main()
{
	int x = 0;
PREV:;
	printf( "%d\n", x++ );
	if ( x < 5 ) goto PREV;
}
