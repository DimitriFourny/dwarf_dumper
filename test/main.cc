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
        printf("hello\n");
    }

    int memberZ;

private:
    classB* ptrToB;
    classB inlineB;
    int memberD;
    int8_t memberB;
    char memberC;
    EnumType enum_member;
    float float_array[3];
    static constexpr int kMScaleX = 0;
};

int main() {
    classA objA;
    printf("a = %p\n", &objA);

    return 0;
}