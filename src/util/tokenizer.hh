#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

namespace tokenizer {

//-----------------------------------------------------------------------------
// Tokenizer
//-----------------------------------------------------------------------------

struct Tokenizer {
public:

    struct iterator
    {

    public: // traits
        using value_type        = std::string;
        using reference         = std::string&;
        using pointer           = std::string*;
        using iterator_category = std::input_iterator_tag;
        using difference_type   = int;

    public:
        iterator(Tokenizer& tokenizer, pointer ptr);

        iterator(const iterator&) = delete;
        iterator& operator=(const iterator&) = delete;

        iterator(iterator&&) = default;
        iterator& operator=(iterator&&) = default;

//        // pos-increment
//        inline auto operator++(int) -> iterator&;

        // pre-increment
        auto operator++() -> iterator&;

        // dereference and access members (doesn't make sense for char)
        auto operator*() const -> reference;
        auto operator->() const -> pointer;

        // boolean comparisons
        bool operator==(const iterator& rhs) const;
        bool operator!=(const iterator& rhs) const;

    public:
        Tokenizer &tokenizer;
        pointer    ptr;

    };

public:
    static const int BUFFER_SIZE = 4096;

public:

    Tokenizer(std::istream *is_ptr, char delim, bool consider_empty=false);
    Tokenizer(std::string st, char delim, bool consider_empty=false);

    std::string *next();

    iterator begin();
    iterator end();

    std::vector<std::string> readAll();

public:
     std::istream *is_ptr { nullptr };
     std::stringstream ss;
     char delim;
     bool consider_empty;
     std::string token;
     char buffer[BUFFER_SIZE];
};

}
