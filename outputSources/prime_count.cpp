#include <iostream>
#include <vector>

bool isprime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

int main() {
    std::vector<int> arr = {17, 24, 29, 35, 47, 50};
    int count = 0;
    for (int x : arr) {
        if (isprime(x)) count++;
    }
    std::cout << count << std::endl;
    return 0;
}