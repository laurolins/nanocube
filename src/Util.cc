#include "Util.hh"

namespace datatiles {

    namespace util {

        std::string fl(std::string st, int n)
        {
            int l = (int) st.size();
            if (l > n)
                return st;
            return st + std::string(n-l,' ');
        }

        std::string fr(std::string st, int n)
        {
            int l = (int) st.size();
            if (l > n)
                return st;
            return std::string(n-l,' ') + st;
        }



    }
}
