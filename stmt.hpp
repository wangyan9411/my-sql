#ifndef Stmt_H
#define Stmt_H
#include "manager.hpp"

struct Stmt {
    virtual void Run(Manager& manager) = 0;
    virtual ~Stmt() {}
};
struct InsertStmt : public Stmt {
    std::string tbl;
    std::vector<std::vector<Object>> rows;
    virtual void Run(Manager& manager);
};
struct DeleteStmt : public Stmt {
    std::string tbl;
    std::vector<Condition> conds;
    virtual void Run(Manager& manager);
};
struct SelectStmt : public Stmt {
    std::string tbl1, tbl2;
    std::vector<Condition> conds;
    std::vector<Expr*>* exprs;
    virtual void Run(Manager& manager);
};
struct UpdateStmt : public Stmt {
    std::string tbl;
    std::vector<Condition> conds;
    ReadExpr lv;
    Object obj;
    UpdateStmt(){};
    virtual void Run(Manager& manager);
};
struct CreateTableStmt : public Stmt {
    std::string tbl;
    std::string key;
    std::vector<Type> types;
    ~CreateTableStmt() {};
    virtual void Run(Manager& manager);
};
struct DropTableStmt : public Stmt {
    std::string tbl;
    virtual void Run(Manager& manager);
};
struct UseStmt : public Stmt {
    std::string db;
    virtual void Run(Manager& manager);
};
struct CreateDBStmt : public Stmt {
    std::string db;
    virtual void Run(Manager& manager);
};
struct DropDBStmt : public Stmt {
    std::string db;
    virtual void Run(Manager& manager);
};

struct ShowTblStmt : public Stmt {
    virtual void Run(Manager& manager);
};
struct DescStmt : public Stmt {
    std::string tbl;
    virtual void Run(Manager& manager);
};

#endif

