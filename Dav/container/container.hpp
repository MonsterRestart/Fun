// Andrew Davies 2015

#if !defined( DAV_CONTAINER_HPP )
#define DAV_CONTAINER_HPP

//#define DAV_CONTAINER_USE_FUZ

#if defined( DAV_CONTAINER_USE_FUZ )
#pragma warning (disable: 4530)
#include <fuz/container/fixed_vector.hpp>
#pragma warning (default: 4530)
#include <fuz/container/fixed_list.hpp>
#else
#include <vector>
#include <list>
#endif

namespace Dav
{

#if defined( DAV_CONTAINER_USE_FUZ )

template< typename Type, size_t Capacity >
class Vector : public fuz::fixed_vector< Type, capacity >
{
};

template< typename Type, size_t Capacity >
class List : public fuz::fixed_list< Type, capacity >
{
};

#else

template< typename Type, size_t Capacity >
class Vector : public std::vector< Type >
{

public:
    bool full() const
    {
        return size() >= Capacity;
    }

};

template< typename Type, size_t Capacity >
class List : public std::list< Type >
{

public:
    bool full() const
    {
        return size() >= Capacity;
    }

};

#endif

}

#endif
