
#include <stdio.h>
#include <stdlib.h>

struct node
{
	int val;
	struct node *pNext;
};

struct node *gen( int number )
{
	struct node *pHead = NULL;

int i;	
for ( i = number; i > 0; i-- )
	{
		struct node *p = (struct node *)malloc( sizeof( struct node ) );
		p->val = i;
		p->pNext = pHead;
		pHead = p;
	}
	return pHead;
}

void display( struct node *pHead )
{
	while ( pHead != NULL )
	{
		printf( "%d ", pHead->val );
		pHead = pHead->pNext;
	}
	printf( "\n" );
}

struct node *reverse( struct node *pHead )
{
	if ( pHead == NULL || pHead->pNext == NULL )
	{
		return pHead;
	}
	struct node *p = pHead->pNext;
	struct node *pNewHead = reverse( p );
	p->pNext = pHead;
	pHead->pNext = NULL;
	return pNewHead;
}

struct node *do_reverse_tail( struct node *pHead, struct node *pNewHead )
{
	if ( pHead == NULL )
	{
		return pNewHead;
	}
	else
	{
		struct node *pNext = pHead->pNext;
		pHead->pNext = pNewHead;
		return do_reverse_tail( pNext, pHead );
	}
}

struct node *reverse_tail( struct node *pHead )
{
	return do_reverse_tail( pHead, NULL );
}

struct node *reverse_it( struct node *pHead )
{
	struct node *pNewHead = NULL;
	struct node *pPrev = NULL;
	struct node *pCur = pHead;
	while ( pCur != NULL )
	{
		struct node *pTmp = pCur->pNext;
		if ( pTmp == NULL )
		{
			pNewHead = pCur;
		}
		pCur->pNext = pPrev;
		pPrev = pCur;
		pCur = pTmp;
	}

	return pNewHead;
}

int main( int argc, char *argv[] )
{
	if (argc < 2)
	{
		printf("usage: %s <number>\n", argv[0]);
		return 1;
	}
	int number = atoi( argv[ 1 ] );
	struct node *pHead = gen( number );
	display( pHead );
	pHead = reverse_it( pHead );
	display( pHead );
}
