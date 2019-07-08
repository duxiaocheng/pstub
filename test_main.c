#include "stub.h"
#include <stdio.h>

void add(int i)
{
    printf("add(%d)\n",i);
}

void add_stub(int i)
{
    printf("add_stub(%d)\n",i);
}

int main()
{
    INSTALL_STUB(add,add_stub);
    add(12);
    REMOVE_STUB(add_stub);
    add(11);
    return 0;
}
