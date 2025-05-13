#include <iostream>

int func(int x) {
    return x;
}

float func(float x) {
    return x;
}

class C {
public:
    int func(int x) {
        return x;
    }
    class C2 {
    public:
        int func(int x) {
            return x;
        }
    };
};

namespace N {
    int func(int x) {
        return x;
    }
    class C {
    public:
        int func(int x) {
            return x;
        }
    };
}

int var = 20;  // 全局变量

int main() {
    func(10);
    func(10.0f);
    C c;
    c.func(var);
    C::C2 c2;
    c2.func(30);
    N::func(40);
    N::C nc;
    nc.func(50);
    printf("%d\n", var);
    return 0;
}
