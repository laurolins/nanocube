#include <iostream>
#include <chrono>
#include <bitset>
#include <iomanip>


// Example program
// #include <iostream>
// #include <string>

static inline unsigned int msb32(unsigned int x)
{
    static const unsigned int bval[] =
    {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
    unsigned int r = 0;
    if (x & 0xFFFF0000) { r += 16/1; x >>= 16/1; }
    if (x & 0x0000FF00) { r += 16/2; x >>= 16/2; }
    if (x & 0x000000F0) { r += 16/4; x >>= 16/4; }
    return r + bval[x];
}

// static inline unsigned int msb32_asm(unsigned int x)
// {
//     unsigned int msb;
//     asm("bsrl %1,%0" : "=r"(msb) : "r"(n));
// }

//
// number is of the form:
//     (1) zero    then rerturn 0
//     (2) 1xx...x  
//              number has at least one bit
//              if   all 'x' are 0 than return same number
//              if   largest 'x' is 0 than return  110...0 
//              else                       return 1000...0
//
unsigned int normalize_1(unsigned int num_bytes, unsigned int class_bits) {
    auto nobits = 0u;         // expect small number of shifts in most cases
    auto x      = num_bytes;  // expect small number of shifts in most cases
    while (x) {               // there can be 12 iterations here to get to 4096
        ++nobits;
        x = x >> 1u;
    }
    // add one to the top 3 most significant digits
    if (nobits <= class_bits) {
        return num_bytes;
    }
    else {
        auto class_shift = nobits - class_bits;
        auto remainder   = num_bytes % (1u << class_shift);
        return ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
    }
}


unsigned int normalize_3(unsigned int num_bytes, unsigned int class_bits) {
    auto nobits = msb32(num_bytes);
    if (nobits > class_bits) {
        auto class_shift = nobits - class_bits;
        auto remainder   = num_bytes % (1U << class_shift);
        return ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
    }
    else {
        return num_bytes;
    }
}

//
// all nodes should fit in one of these classes
// (with later pointers and flags we might need extra sizes)
//
//
// cache line misses will happen frequently in this code
//
static const int SIZE_CLASSES[] { 
    1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,
    80,96,112,128,160,192,224,256,320,384,448,512,640,
    768,896,1024,1280,1536,1792,2048,2560,3072,3584,4096
};

static const int NUM_SIZE_CLASSES = sizeof(SIZE_CLASSES)/sizeof(int);

static inline int normalize_2(int lower_bound) {

    //
    // assume SIZE_CLASSES[0] <= num_bytes <= SIZE_CLASSES[NUM_SIZE_CLASSES-1]
    // assume no two numbers are the same in SIZE_CLASSES
    // assume SIZE_CLASSES are sorted
    //

    // 
    // binary search for left most position on SIZE_CLASSES
    // where num_bytes is a lower bound (less than or equal)
    //
    int l = 0;
    int r = NUM_SIZE_CLASSES - 1;
    while (l < r) {
        auto m  = (l + r) / 2;
        auto class_m = SIZE_CLASSES[m];
        if (lower_bound > class_m) { // need to make class_m larger (move l)
            l = m + 1; 
        }
        else if (lower_bound < class_m) { // need to make class_m smaller (move r)
            r = m;
        }
        else return class_m;
    }
    return SIZE_CLASSES[l];
}

//
// ask for 2
//
// m
// l r
// 1 3
//
// 
struct P{};

std::ostream& operator<<(std::ostream& os, const P&) {
    os << "|" << std::setw(5) << std::right;
    return os;
}

unsigned int classSizeToClass(unsigned int x) {
    auto msb = msb32(x);
    return 1 + (x > 1) * ( (msb-1) * 2 - ( ( (1 << (msb-2)) & x ) == 0 ) );
}

int main() {

    // std::cout << msb32(7) << std::endl;

    auto N   = 100000;

    auto count = 0;
     // std::cout << "numbernormalize 1" << sizes[i] << "
    for (auto i=1;i<=1000000;++i) {
        if (i == normalize_3(i,2)) {
            ++count;
            std::cout << P() << count 
                      << P() << msb32(i)
                      << P() << i
                      << P() << std::bitset<13>(i) 
                      << P() << classSizeToClass(i)
                      << std::endl;
        }
    }
    


    auto sum3 = 0ULL;
    double e3 = 0.0;
    { // count
        auto start = std::chrono::system_clock::now();
        for (auto r=0;r<N;++r) {
            for (auto i=1;i<=4096;++i) {
                sum3 += (unsigned long long) normalize_3(i,3);
            }
        }
        auto end = std::chrono::system_clock::now();
        auto elapsed = end - start;
        std::cout << "normalize_3: " << elapsed.count() << std::endl;
        e3 = (double) elapsed.count();
    }


    auto sum1 = 0ULL;
    double e1 = 0.0;
    { // count
        auto start = std::chrono::system_clock::now();
        for (auto r=0;r<N;++r) {
            for (auto i=1;i<=4096;++i) {
                sum1 += (unsigned long long) normalize_1(i,3);
            }
        }
        auto end = std::chrono::system_clock::now();
        auto elapsed = end - start;
        std::cout << "normalize_1: " << elapsed.count() << std::endl;
        e1 = (double) elapsed.count();
    }


    auto sum2 = 0ULL;
    double e2 = 0.0;
    { // count
        auto start = std::chrono::system_clock::now();
        for (auto r=0;r<N;++r) {
            for (auto i=1;i<=4096;++i) {
                sum2 += (unsigned long long) normalize_2(i);
            }
        }
        auto end = std::chrono::system_clock::now();
        auto elapsed = end - start;
        std::cout << "normalize_2: " << elapsed.count() << std::endl;
        e2 = (double) elapsed.count();
    }



    std::cout << "sum1: " << sum1 << " sum2: " << sum2 << " sum3: " << sum3 << " ok12=" << (sum1 == sum2) << " ok13=" << (sum1 == sum3) << std::endl;
    std::cout << "e1: " << e1 << " e2: " << e2 <<  " e3: " << e3 << " e1/e2: " << (e1/e2) << " e1/e3: " << e1/e3 << std::endl;
}



    // for (auto i=1;i<=4096;++i) {
    //     if (i == normalize_3(i,2)) {
    //         ++count;
    //         std::cout << std::setw(5) << std::right << i 
    //                   << std::setw(5) << std::right << normalize_3(i,2)
    //                   << std::setw(5) << std::right << normalize_3(i,3)
    //                   << std::endl;
    //     }
    // }














// int main()
// {
//     std::cout << "7 -> " << normalize_size(7) << std::endl;
//     std::cout << "8 -> " << normalize_size(8) << std::endl;
//     std::cout << "9 -> " << normalize_size(9) << std::endl;
// }

