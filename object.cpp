#include "object.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>

Object LiteralManager::GetVarChar(std::string& l) {
    Object obj;
    char* buf = new char[MAX_LENGTH];
    memcpy(buf, l.data(), l.size());
    memset(buf + l.size(), 0, MAX_LENGTH - l.size());
    chars.push_back(buf);
    obj.loc = buf;
    obj.size = l.size();
    obj.type = TYPE_VARCHAR;
    obj.is_null = false;
    return obj;
}

Object LiteralManager::GetInt(int l) {
    Object obj;
    int* buf = new int;
    ints.push_back(buf);
    *buf = l;
    obj.loc = buf;
    obj.size = sizeof(l);
    obj.type = TYPE_INT;
    obj.is_null = false;
    return obj;
}

Object LiteralManager::GetNull() {
    Object obj;
    obj.loc = nullptr;
    obj.size = 0;
    obj.type = TYPE_INT;
    obj.is_null = true;
    return obj;
}

void LiteralManager::clear() {
    for (auto c : chars)
        delete [] c;
    for (auto i : ints)
        delete i;
    chars.clear();
    ints.clear();
}

Object ReadExpr::getObj(void* l, void* r){
    Object ret;
    ret.size = size;
    if ( useLeft ) {
        ret.is_null = nullMask & *(unsigned short*)l;
        ret.loc = (char*)l + offset;
        ret.type = type;
    } else {
        ret.is_null = nullMask & *(unsigned short*)r;
        ret.loc = (char*)r + offset;
        ret.type = type;
    }
    return ret;
}

void ReadExpr::Use(const std::string& lname, const std::string& rname, TableDesc* ldesc, TableDesc* rdesc) {
    offset = 2;
    if (tbl == "") {
        nullMask = 1;
        useLeft = true;
        for ( int i=0; i<ldesc->colSize; i++ ) {
            if ( name == ldesc->colType[i].name ) {
                size = ldesc->colType[i].size;
                type = ldesc->colType[i].type;
                return;
            } else {
                offset += ldesc->colType[i].size;
                nullMask <<= 1;
            }
        }
        useLeft = false;
         for ( int i=0; i<rdesc->colSize; i++ ) {
            if ( name == rdesc->colType[i].name ) {
                size = rdesc->colType[i].size;
                type = rdesc->colType[i].type;
                return;
            } else {
                offset += rdesc->colType[i].size;
                nullMask <<= 1;
            }
        }
    } else if (tbl == lname) {
        useLeft = true;
        for ( int i=0; i<ldesc->colSize; i++ ) {
            if ( name == ldesc->colType[i].name ) {
                size = ldesc->colType[i].size;
                type = ldesc->colType[i].type;
                return;
            } else {
                offset += ldesc->colType[i].size;
                nullMask <<= 1;
            }
        }
    } else {
        useLeft = false;
         for ( int i=0; i<rdesc->colSize; i++ ) {
            if ( name == rdesc->colType[i].name ) {
                size = rdesc->colType[i].size;
                type = rdesc->colType[i].type;
                return;
            } else {
                offset += rdesc->colType[i].size;
                nullMask <<= 1;
            }
        }
    }
}

Object LiteralExpr::getObj(void* l, void* r) {
    return obj;
}

void LiteralExpr::Use(const std::string& lname, const std::string& rname, TableDesc* ldesc, TableDesc* rdesc){
    return;
}

#define DEF_OP(OP, RAW_OP) \
bool op_##OP(const Object& lobj, const Object& robj){\
    if ( lobj.is_null || robj.is_null )\
        return false;\
    if ( lobj.type != robj.type ) {\
        throw "Type Error";\
    }\
    if ( lobj.type == TYPE_INT ){\
        int* l = (int*)lobj.loc;\
        int* r = (int*)robj.loc;\
        return (*l RAW_OP *r);\
    } else {\
        if (memcmp((char*)lobj.loc, (char*)robj.loc, lobj.size) RAW_OP 0)\
            return true;\
        return false;\
    }\
}

DEF_OP(ne, !=)
DEF_OP(lt, <)
DEF_OP(gt, >)
DEF_OP(le, <=)
DEF_OP(ge, >=)

bool op_eq(const Object& lobj, const Object& robj){
    if ( lobj.is_null || robj.is_null )
        return lobj.is_null && robj.is_null;
    if ( lobj.type != robj.type ) {
        throw "Type Error";
    }
    if ( lobj.type == TYPE_INT ){
        int* l = (int*)lobj.loc;
        int* r = (int*)robj.loc;
        return (*l == *r);
    } else {
        if (memcmp((char*)lobj.loc, (char*)robj.loc, lobj.size) == 0)
            return true;
        return false;
    }
}
