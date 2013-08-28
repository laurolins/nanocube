#include <iostream>

#include <boost/mpl/list.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include <boost/type_traits/is_same.hpp>

namespace tuple {

typedef boost::mpl::l_end ListEnd;

namespace detail {

    template <bool Flag=true>
    struct Aux
    {
        template <typename E, typename T>
        static void set(E &e, T v)
        {
            e.head = v;
        }

        template <typename E, typename T>
        static T& get(E &e)
        {
            return e.head;
        }

    };

    template <>
    struct Aux<false>
    {
        template <typename E, typename T>
        static void set(E &e, T v)
        {
            e.tail.set(v);
        }

        template <typename E, typename T>
        static T& get(E &e)
        {
            return e.tail.template get<T>();
        }
    };
}

template <typename TypeList> // TypeList assuming is a boost::mpl::list
struct Tuple
{
    typedef TypeList                                           Types;
    typedef typename boost::mpl::front<TypeList>::type         Head;
    typedef typename boost::mpl::pop_front<TypeList>::type     TailTypes;
    typedef Tuple<TailTypes>                                   Tail;

    template <typename T>
    void set(T v)
    {
        const bool FOUND =  boost::is_same<Head, T >::value;
        detail::Aux<FOUND>::set(*this, v);
    }

    template <typename T>
    T &get()
    {
        const bool FOUND =  boost::is_same<Head, T >::value;
        return detail::Aux<FOUND>::template get<Tuple, T>(*this);
    }

    Head head;
    Tail tail;
};


template <>
struct Tuple<ListEnd>
{
    template <typename T>
    T get()
    {
        throw std::string("ooops");
    }

    template <typename T>
    void set(T /*v*/)
    {
        throw std::string("ooops");
    }
};

}
