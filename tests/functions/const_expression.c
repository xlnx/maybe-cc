typedef struct nk_ptr
{
	int a;
} * nk_ptr;

typedef short nk_short;

int a[ ( sizeof( nk_short ) == 2 ) ? 1 : -1 ];

int main()
{
}