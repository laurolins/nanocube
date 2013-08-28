#pragma once

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <map>

#include <cxxabi.h>

//------------------------------------------------------------------------
// getTypeName
//------------------------------------------------------------------------

template <typename T>
std::string getTypeName()
{
    char   buffer[10000];
    size_t length = 10000;
    int    status;
    abi::__cxa_demangle(typeid(T).name(),
                        buffer,
                        &length,
                        &status);
    buffer[length] = 0;
    return std::string(buffer);
}


namespace datatiles {

    namespace util {

        std::string fl(std::string st, int n=14);
        std::string fr(std::string st, int n=14);

        template <class T>
        inline std::string str(const T& t)
        {
            std::stringstream ss;
            ss << std::setprecision(1) << std::fixed << t ;
            return ss.str();
        }

        //-----------------------------------------------------------------------------
        // Histogram
        //-----------------------------------------------------------------------------

        typedef uint64_t Count;

        template <typename T>
        struct Histogram
        {
            Histogram();

            void add(T obj); // T should be small it will be copied all around
            void dumpReport(std::ostream &os);
            void reset();

            Count numAddedObjects;      // same as number of nodes

            std::map<T, Count> data;
        };


        //------------------------------------------------------------------------------------
        // Histogram Impl.
        //------------------------------------------------------------------------------------

        template <typename T>
        void Histogram<T>::add(T obj)
        {
            auto it = data.find(obj);
            if (it == data.end())
                data[obj] = 1;
            else
                data[obj]++;
            numAddedObjects++;
        }

        template <typename T>
        Histogram<T>::Histogram():
            numAddedObjects(0)
        {}

        template <typename T>
        void Histogram<T>::dumpReport(std::ostream &os)
        {
            os << "Histogram (num adds: " << numAddedObjects << ", num objs: " << data.size() << ")" << std::endl;

            std::string sep = " | ";
            os << std::setw(10) << "#Obj" << sep << std::setw(10) << "#Count" << std::endl;

            for (auto it=data.begin();it!=data.end();it++)
            {
                os << std::setw(10) << it->first << sep << std::setw(10) << it->second << std::endl;
            }
            os << std::endl;
        }

        template <typename T>
        void Histogram<T>::reset()
        {
            data.clear();
            numAddedObjects = 0;
        }

    }
}
