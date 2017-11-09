#include "ptr.hh"

#include <vector>
#include <chrono>
#include <iostream>


using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
struct Stopwatch {
    Stopwatch() {
        t0 = std::chrono::high_resolution_clock::now();
    }
    double elapsed() const {
        auto t1 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count()/1.0e3;
    }
    Timestamp t0;
};

struct X {
    X() = default;
    X(int id): id(id) {}
    int id { 0 };
};

int main() {

    using namespace ptr;

    int n = (int) 1.0e9; // 100M objects ~ 400MB or RAM
    std::vector<X> objects;
    {
        Stopwatch s;
        objects.reserve(n);
        for (auto i=0;i<n;++i) { objects.push_back(X(i)); }
        std::cout << "Time to initialize objects: " << s.elapsed() << "s." << std::endl;
    }

    double ptr_init_time, ptr_get_time;
    double raw_init_time, raw_get_time;

    {
        std::vector<Ptr<X>> objects_p;
        objects_p.reserve(n);
        
        {
            Stopwatch s;
            for (auto &obj: objects) { 
                objects_p.push_back(Ptr<X>(&obj));  
                // std::cout << (void*) &obj << " offset: " << objects_p.back().offset() << std::endl;
            }
            
            ptr_init_time = s.elapsed();
        }

        {
            double sum = 0.0;
            Stopwatch s;
            for (auto &p: objects_p) { 
                // std::cout << p.raw() << " offset: " << p.offset() << std::endl;
                sum += p->id; 
            }
            ptr_get_time = s.elapsed();
            std::cout << "Ptr sum:       " << sum  << "s." << std::endl;
        }

        std::cout << "Ptr init time: " << ptr_init_time << "s." << std::endl;
        std::cout << "Ptr get  time: " << ptr_get_time  << "s." << std::endl;
    }

    {
        std::vector<X*> objects_p;
        objects_p.reserve(n);
        

        {
            Stopwatch s;
            for (auto &obj: objects) { objects_p.push_back(&obj); }
            raw_init_time = s.elapsed();
        }


        {
            double sum = 0.0;
            Stopwatch s;
            for (auto &p: objects_p) { sum += p->id; }
            raw_get_time = s.elapsed();
            std::cout << "Raw sum:       " << sum  << "s." << std::endl;
        }

        std::cout << "Raw init time: " << raw_init_time << "s." << std::endl;
        std::cout << "Raw get  time: " << raw_get_time  << "s." << std::endl;
    }





    std::cout << "Ptr init time / Raw init time: " << ptr_init_time / raw_init_time << "x" << std::endl;
    std::cout << "Ptr get time  / Raw get  time: " << ptr_get_time / raw_get_time << "x" << std::endl;

    // std::vector<X*> objects_p;
    // objects_p.reserve();

    // X a;
    // Ptr<X> a_ptr(&a);
    // a_ptr.raw();

    // auto b = new X();
    // Ptr<X> b_ptr(b);

    // std::cout << "a: " << a_ptr.raw() << " == " << &a << " ---> " << a_ptr.offset() << std::endl;
    // std::cout << "b: " << b_ptr.raw() << " == " << b  << " ---> " << b_ptr.offset() << std::endl;

    return 0;

}


// alloc git:(master) ✗ clang++ -O3 -std=c++11 -stdlib=libc++ -o ptr_test ptr_test.cc
// alloc git:(master) ✗ ./ptr_test
// Time to initialize objects: 2.064s.
// Raw sum:       1.25e+17s.
// Raw init time: 5.261s.
// Raw get  time: 10.096s.
// Ptr sum:       1.25e+17s.
// Ptr init time: 3.094s.
// Ptr get  time: 4.055s.
// Ptr init time / Raw init time: 0.588101x
// Ptr get time  / Raw get  time: 0.401644x


//
// Time to initialize objects: 1.82s.
// Ptr sum:       1.25e+17s.
// Ptr init time: 3.509s.
// Ptr get  time: 3.886s.
// Raw sum:       1.25e+17s.
// Raw init time: 3.882s.
// Raw get  time: 5.512s.
// Ptr init time / Raw init time: 0.903916x
// Ptr get time  / Raw get  time: 0.705007x
//