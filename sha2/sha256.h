#ifndef SHA256_H
#define SHA256_H
#include <functional>
#include <mutex>
#include <CL/sycl.hpp>
using namespace cl;
using namespace sycl;
const uint32_t K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

extern bool end_gpu;
extern std::mutex mtx;
const short total_result = 1;

const size_t NUM_LEADING_ZEROS = 9;

struct Result {
    size_t number;
    uint32_t hash[8];
};


// Funkcia na kontrolu počtu vedúcich núl
inline bool has_leading_zeros(uint32_t hash[8], size_t num_zeros) {
    size_t zero_count = 0;    
    for (size_t i = 0; i < 8; ++i) {
        uint32_t value = hash[i];
        
        // Pre každý 4-bitový blok (nibble) v 32-bitovej hodnote
        for (int j = 28; j >= 0; j -= 4) {
            if (((value >> j) & 0xF) == 0) {
                zero_count++;
                if (zero_count >= num_zeros) {
                    return true;
                }
            } else {
                return false;
            }
        }
    }    
    return false;
}

inline size_t format_input_para(char* output, size_t number) {
    // Priamo nastav prefix
    output[0] = 'p';
    output[1] = 'a';
    output[2] = 'r';
    output[3] = 'a';

    // Zapisujeme číslo odzadu do pomocného buffera
    char buffer[24]; // Predpokladáme, že size_t nebude väčšie ako 20 číslic
    char* ptr = buffer + sizeof(buffer); // Ukazovateľ na koniec bufferu
    size_t length = 0;  
    do {
        *(--ptr) = (number % 10) + '0'; // Zvyšok čísla
        number /= 10;
        ++length;
    } while (number > 0);

    // Skopíruj číslo priamo za prefix
    for (size_t i = 0; i < length; ++i) {
        output[4 + i] = ptr[i];
    }
    length += 4;
    
    output[length] = '\0'; // Ukončenie reťazca

    return length; // Celková dĺžka reťazca
}

// SHA-256 pomocné funkcie
inline uint32_t right_rotate(uint32_t value, size_t shift) {
    return (value >> shift) | (value << (32 - shift));
}

#endif // SHA256_H
