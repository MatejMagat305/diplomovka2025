#include <sycl/sycl.hpp>
#include <iostream>

struct Vec3 {
    float x, y, z;
};

int main() {
    sycl::queue q;

    // Alokujeme pamäť
    Vec3* d_data = sycl::malloc_device<Vec3>(10, q);

    q.parallel_for(10, [=](sycl::id<1> idx) {
        int i = idx[0];
        d_data[i] = {1.0f * i, 2.0f * i, 3.0f * i};
    }).wait();
    Vec3* temp = new Vec3[10];
    q.memcpy(temp, d_data, 10*sizeof(Vec3)).wait();
    for(int i=0;i<10;i++){
        Vec3& t = temp[i];
        std::cout << "{ x:" << t.x << ", y:" <<  t.y << ", z:" << t.z << " }" << std::endl;
    }
    // Uvoľníme pamäť
    sycl::free(d_data, q);
    delete[] temp;

    return 0;
}
