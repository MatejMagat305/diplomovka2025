#include <iostream>
#include <CL/sycl.hpp>

int main() {
  try {
    // Získanie zoznamu dostupných zariadení SYCL
    std::vector<cl::sycl::device> devices = cl::sycl::platform::get_platforms()[0].get_devices();

    // Prechádzanie zoznamom zariadení a výpis ich informácií
    for (const auto& device : devices) {
      std::cout << "Zariadenie: " << device.get_info<cl::sycl::info::device::name>() << std::endl;
      std::cout << "  Typ: " << (device.is_cpu() ? "CPU" : (device.is_gpu() ? "GPU" : "Iné")) << std::endl;
      std::cout << "  Výrobca: " << device.get_info<cl::sycl::info::device::vendor>() << std::endl;
      std::cout << "  Verzia ovládača: " << device.get_info<cl::sycl::info::device::driver_version>() << std::endl;
      std::cout << "  Globálna pamäť: " << device.get_info<cl::sycl::info::device::global_mem_size>() << " bajtov" << std::endl;
      std::cout << "  Maximálna pracovná skupina: " << device.get_info<cl::sycl::info::device::max_work_group_size>() << std::endl;
      std::cout << std::endl;
    }
  } catch (cl::sycl::exception const& e) {
    std::cerr << "SYCL chyba: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
