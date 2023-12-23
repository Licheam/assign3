#include <stdlib.h>

int plus(int a, int b)
{
    return a+b;
}

int minus(int a,int b)
{
    return a-b;
}

void swap1(int (**a_f)(int, int), int (**b_f)(int, int))
{
    int (*tmp)(int, int) = *a_f;
    *a_f = *b_f;
    *b_f = tmp;
}

void swap2(int (*a_f2)(int, int), int (*b_f2)(int, int))
{
    int (*tmp2)(int, int) = a_f2;
    a_f2 = b_f2;
    b_f2 = tmp2;
}

int clever() {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;

    swap1(&a_fptr, &s_fptr);
    swap2(a_fptr, s_fptr);

    unsigned result = a_fptr(1, 2);

    return 0;
}

// 31 : swap1
// 32 : swap2
// 34 : minus