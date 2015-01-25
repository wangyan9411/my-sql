#ifndef OBJECT_H
#define OBJECT_H
#include <string>
#include "type.hpp"
#include "baseobj.hpp"
#include "table.hpp"
struct LiteralManager {
    const int MAX_LENGTH = 200;
    std::vector<char*> chars;
    std::vector<int*> ints;
    Object GetInt(int l);
    Object GetVarChar(std::string& l);
    Object GetNull();
    void clear();
};
struct Expr {
    virtual Object getObj(void* l, void* r = nullptr) = 0;
    virtual void Use(const std::string& lname, const std::string& rname, TableDesc* ldesc, TableDesc* rdesc = nullptr) = 0;
};
struct ReadExpr : public Expr {
    bool useLeft;
    int offset;
    int size;
    int nullMask;
    TYPE type;
    std::string tbl, name;
    ReadExpr(){};
    ReadExpr(const std::string& _name) : tbl(""), name(_name) {}
    ReadExpr(const std::string& _tbl, const std::string& _name) : tbl(_tbl), name(_name) {}
    virtual Object getObj(void* l, void* r = nullptr);
    virtual void Use(const std::string& lname, const std::string& rname, TableDesc* ldesc, TableDesc* rdesc = nullptr);
};
struct LiteralExpr : public Expr {
    Object obj;
    LiteralExpr(Object _obj) : obj(_obj) {}
    virtual Object getObj(void* l, void* r = nullptr);
    virtual void Use(const std::string& lname, const std::string& rname, TableDesc* ldesc, TableDesc* rdesc = nullptr);
};
typedef bool (*Oper)(const Object&, const Object&);

struct Condition {
    Expr *l, *r;
    Oper op;
};

bool op_eq(const Object&, const Object&);
bool op_ne(const Object&, const Object&);
bool op_lt(const Object&, const Object&);
bool op_gt(const Object&, const Object&);
bool op_le(const Object&, const Object&);
bool op_ge(const Object&, const Object&);
#endif
