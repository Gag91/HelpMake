#include <cstdio>
import math;

int main() {
    if (add(2, 3) != 5) {
        std::printf("add failed\n");
        return 1;
    }
    if (mul(4, 5) != 20) {
        std::printf("mul failed\n");
        return 1;
    }
    std::printf("msvc module tests passed\n");
    return 0;
}