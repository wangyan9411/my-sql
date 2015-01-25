#ifndef BASEOBJECT_H
#define BASEOBJECT_H
#include <string>
#include "type.hpp"


struct Object {
    void* loc;
    int size;
    TYPE type;
    bool is_null;
    Object(){};
    Object(void* location, int s, TYPE t, bool null) {
        loc = location;
        size = s;
        type = t;
        is_null = null;
    }
};

inline bool operator<(const Object& lhs, const Object& rhs) {
    int size = lhs.size > rhs.size ? lhs.size : rhs.size;
    return std::memcmp(lhs.loc, rhs.loc, size) < 0;
}

inline bool operator!=(const Object& lhs, const Object& rhs) {
    int size = lhs.size > rhs.size ? lhs.size : rhs.size;
    return std::memcmp(lhs.loc, rhs.loc, size) != 0;
}


#endif

