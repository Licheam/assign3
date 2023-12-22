Print the callee functions at every call instructions. The printed format should be 
 
`${line} : ${func_name1}, ${func_name2}`, here `${line}` is unique in output.

You are supposed to implement a flow-sensitive, field- and context-insensitive algorithm to compute the points-to set for each variable at 

each distinct program point.

One approach is to extend the provided dataflow analysis framework as follows: 
. Implement your representation of points-to sets
. Implement the transfer function for each instruction
. Implement the MEET operator
. Extend the control flow graph to be inter-procedural

 

Note that we do not require the analysis to be context-sensitive or field-sensitive.

Give the test file:

```c {.line-numbers}
#include <stdlib.h>
struct fpstruct {
    int (*t_fptr)(int,int);
} ;
int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int op1=1, op2=2;
    struct fpstruct * t1 = malloc(sizeof (struct fpstruct));
    if (x == 3) {
        t1->t_fptr = a_fptr;    
    } else {
        t1->t_fptr = s_fptr;
    } 
    unsigned result = t1->t_fptr(op1, op2);
    return 0;
}
```
 

You output should be:
```
9 : malloc
15 : plus, minus
```

Give the test file:
```c {.line-numbers}
#include <stdlib.h>
struct fpstruct {
    int (*t_fptr)(int,int);
} ;
void foo(struct fpstruct *t, int flag) {
    if (flag == 3) {
        t1->t_fptr = plus;    
    } else {
        t1->t_fptr = minus;
}      
}
int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    int op1=1;
    struct fpstruct * t1 = malloc(sizeof (struct fpstruct));
    foo(t1, op1);
    unsigned result = t1->t_fptr(op1, 2);
    return 0;
}
```
 

Here the output should be
```
16: malloc
17: foo
18: plus, minus
```

Note that we do not need to further evaluate the branch condition at line 6, and both true and false branch of the if statement (line 6) are supposed to be taken.
```c {.line-numbers}
#include <stdlib.h>
struct fpstruct {
    int (*t_fptr)(int,int);
} ;
void foo(struct fpstruct *t, int (*a_fptr)(int,int)) {
    t1->t_fptr = a_fptr;    
}
int clever(int x) {
    int (*a_fptr)(int, int) = plus;
    int (*s_fptr)(int, int) = minus;
    struct fpstruct * t1 = malloc(sizeof (struct fpstruct));
    foo(t1, a_fptr);
    unsigned result = t1->t_fptr(1, 2);
    foo(t1, s_fptr);
    result = t1->t_fptr(1, 2);
    return 0;
}
```
 

Here the output should be
```
11: malloc
12: foo
13: plus, minus
14: foo
15: plus, minus
```

Note that the analysis do not need to be context-sensitive. Hence the exit of function foo can return to both callsites (line 12 and 14), resulting the points-to sets of t1 at both line 13 and 15  be {plus,minus}