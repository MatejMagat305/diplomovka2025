#include <sycl/sycl.hpp>
#include <iostream>
#include <string> // Pre std::strlen a std::reverse
#include <algorithm> //Pre std::reverse

int main() {
    sycl::queue q;
    char p[] = "ahoj, original sprava";
    size_t len0 = std::strlen(p);

    char* data = sycl::malloc_device<char>(len0 + 1, q);
    q.memcpy(data, p, len0 + 1 * sizeof(char)).wait();
    q.single_task([&len0, data]() {
        std::reverse(data, data+len0); // funguje gpu
    }).wait();

    char* temp = new char[len0 + 1];
    q.memcpy(temp, data, (len0 + 1) * sizeof(char)).wait();
    temp[len0] = '\0'; //Nezabudnúť na null terminator

    std::cout << "After reverse: " << temp << std::endl;

    sycl::free(data, q);
    delete[] temp; // Správne uvoľnenie pre new char[]
    return 0;
}
