#include "b.h"
#include <stdio.h>

classB::~classB() {
    printf("classB\n");
}

void classB::myVirtualMethod() {
  printf("classA myVirtualMethod\n");
}