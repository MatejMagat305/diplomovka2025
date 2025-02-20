
#include <sycl/sycl.hpp>
#include <iostream>

int main() {
    sycl::queue q{sycl::property::queue::enable_profiling()};

    constexpr int N = 1000000;
    float* d_data = sycl::malloc_device<float>(N, q);

    // Kernel 1 - Inicializácia
    sycl::event e1 = q.submit([&](sycl::handler& h) {
        h.parallel_for(N, [=](sycl::id<1> i) {
            d_data[i] = i[0] * 1.0f;
        });
    });

    // Kernel 2 - Zvýšenie hodnôt (explicitná závislosť na e1)
    sycl::event e2 = q.submit([&](sycl::handler& h) {
        h.depends_on(e1);  // Explicitná závislosť
        h.parallel_for(N, [=](sycl::id<1> i) {
            d_data[i] += 5.0f;
        });
    });

    // Kernel 3 - Násobenie hodnôt (explicitná závislosť na e2)
    sycl::event e3 = q.submit([&](sycl::handler& h) {
        h.depends_on(e2);  // Explicitná závislosť
        h.parallel_for(N, [=](sycl::id<1> i) {
            d_data[i] *= 2.0f;
        });
    });

    e3.wait(); // Počkáme na dokončenie

    // Meranie časov
    auto t1 = e1.get_profiling_info<sycl::info::event_profiling::command_start>();
    auto t2 = e1.get_profiling_info<sycl::info::event_profiling::command_end>();
    auto t3 = e2.get_profiling_info<sycl::info::event_profiling::command_end>();
    auto t4 = e3.get_profiling_info<sycl::info::event_profiling::command_end>();

    std::cout << "Kernel 1 time: " << (t2 - t1) / 1e3 << " µs\n";
    std::cout << "Kernel 2 time: " << (t3 - t2) / 1e3 << " µs\n";
    std::cout << "Kernel 3 time: " << (t4 - t3) / 1e3 << " µs\n";
    std::cout << "Total execution time: " << (t4 - t1) / 1e3 << " µs\n";

    sycl::free(d_data, q);
    return 0;
}
/*
matej-magat@matej-mag-COM:~/AdaptiveCpp/tests$ ./threeKernels 
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
[AdaptiveCpp Warning] kernel_cache: This application run has resulted in new binaries being JIT-compiled. This indicates that the runtime optimization process has not yet reached peak performance. You may want to run the application again until this warning no longer appears to achieve optimal performance.
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
'+ptx86' is not a recognized feature for this target (ignoring feature)
Kernel 1 time: 66752.5 µs
Kernel 2 time: 51335.2 µs
Kernel 3 time: 50543.8 µs
Total execution time: 168632 µs
matej-magat@matej-mag-COM:~/AdaptiveCpp/tests$ ./threeKernels 
Kernel 1 time: 343.04 µs
Kernel 2 time: 130.405 µs
Kernel 3 time: 109.211 µs
Total execution time: 582.656 µs
matej-magat@matej-mag-COM:~/AdaptiveCpp/tests$ ./threeKernels 
Kernel 1 time: 308.224 µs
Kernel 2 time: 137.127 µs
Kernel 3 time: 115.801 µs
Total execution time: 561.152 µs
matej-magat@matej-mag-COM:~/AdaptiveCp
*/
