#include <stdlib.h>
struct fptr
{
int (*p_fptr)(int, int);
};
struct fsptr
{
struct fptr * sptr;
};

int plus(int a, int b) {
   return a+b;
}

int minus(int a, int b) {
   return a-b;
}

void swap(struct fsptr a_fptr,struct fsptr b_fptr)
{
   int (*temp)(int, int)=a_fptr.sptr->p_fptr;
   a_fptr.sptr->p_fptr=b_fptr.sptr->p_fptr;
   b_fptr.sptr->p_fptr=temp;
}

int moo(char x, int op1, int op2) {
   struct fptr a_fptr ;
   a_fptr.p_fptr=plus;
   struct fptr s_fptr ;
   s_fptr.p_fptr=minus;

   struct fsptr m_fptr;
   m_fptr.sptr=&a_fptr;
   struct fsptr n_fptr;
   n_fptr.sptr=&s_fptr;

   swap(m_fptr,n_fptr);

   int r1 = a_fptr.p_fptr(op1,op2);
   int r2 = m_fptr.sptr->p_fptr(op1,op2);
   return 0;
}

// 37 : swap
// 39 : minus
// 40 : minus