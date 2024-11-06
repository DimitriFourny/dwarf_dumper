#include <stdio.h>
#include <stdint.h>
#include "b.h"

enum EnumType {
  ENUM_0,
  ENUM_1
};

class classA : public classB {
public:
    classA() = default;
    virtual ~classA() {
        printf("classA\n");
    }
    virtual void myVirtualMethod() {
      printf("classA myVirtualMethod\n");
    }

    int memberZ;

    class classInClass {
      int whynot;
    };
    classInClass objClass;

private:
    classB* ptrToB;
    classB inlineB;
    int memberD;
    uint8_t memberB;
    char memberC;
    EnumType enum_member;
    float float_array[3];
    static constexpr int kMScaleX = 0;
};

int main() {
    classA objA;
    printf("a = %p\n", &objA);
    objA.myVirtualMethod();

    return 0;
}