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

void swap2(int (*a_f)(int, int), int (*b_f)(int, int))
{
    int (*tmp)(int, int) = a_f;
    a_f = b_f;
    b_f = tmp;
}

int clever() {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;

    int (**a_f)(int, int) = &a_fptr;
    int (**s_f)(int, int) = &s_fptr;

    int (*tmp)(int, int) = *a_f;
    *a_f = *s_f;
    *s_f = tmp;

    unsigned result = a_fptr(1, 2);

    return 0;
}

// 38 : minus