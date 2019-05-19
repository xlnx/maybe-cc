#include <stdio.h>

int main()
{
	int i = 2;
	switch ( i )
	{
	case 0:
		++i;
		printf( "%d\n", i );
		for ( ; i != 4; i++ )
		{
		case 1:
		case 2: printf( "%d\n", i );
		}
	case 3:
		++i;
		printf( "%d\n", i );
		break;
	default:
		printf( "%d\n", i );
	}
}