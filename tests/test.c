#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

extern int f();

int main()
{
	return 10 * f() * sin( 3.14 / 2 );
}
