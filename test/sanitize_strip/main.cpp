#include <iostream>

int main() {
    int *arr = new int[5];
    arr[5] = 42; // Out of bounds write
    std::cout << "If you see this, ASAN didn't catch it.\n";
    delete[] arr;
    return 0;
}