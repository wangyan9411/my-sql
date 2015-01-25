#ifndef MANAGER_H
#define MANAGER_H

#include <vector>
#include "table.hpp"
#include "object.hpp"
#include <unordered_map>

struct Manager {
    std::string dbName;
    Manager();
    std::unordered_map<std::string, Table*> tables;
    std::string tblFileName(const std::string& tbl);
    Table* getTable(const std::string& tbl, bool init);
    std::vector<void*> filterOne(const std::string& tbl, const std::vector<Condition>& conds);
    std::vector<std::pair<void*, void*> > filterTwo(const std::string& tbl1, const std::string& tbl2, const std::vector<Condition>& conds);
    void Insert(const std::string& tbl, const std::vector<std::vector<Object>>& rows);
    void Delete(const std::string& tbl, const std::vector<Condition>& conds);
    void Select(const std::string& tbl1, const std::string& tbl2, const std::vector<Condition>& conds, std::vector<Expr*>* sel = nullptr);
    void Update(const std::string& tbl, const std::vector<Condition>& conds, ReadExpr& lv, const Object& rv);
    void CreateTable(const std::string& tbl, const std::vector<Type>& types, const std::string& keyname);
    void DropTable(const std::string& tbl);
    void Use(const std::string& db);
    void CreateDB(const std::string& db);
    void DropDB(const std::string& db);
    void Desc(const std::string& tbl);
    void ShowTables();
};


bool isCorrect(Object& obj1, Object& obj2, Oper op);


#endif
