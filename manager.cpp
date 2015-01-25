#include <iostream>
#include <cstdlib>
#include <fstream>

#include "manager.hpp"
#include "table.hpp"
#include "object.hpp"

struct DBInfo {
    int tableCount;
    char tables[20][NAME_LEN];
};
void checkType(Type type, const Object& obj) {
    if (obj.is_null) {
        if (!type.null)
            throw "Type Check Error";
        return;
    }
    if (obj.type != type.type)
        throw "Type Check Error";
}

void WriteBinRow(void* buf, const TableDesc& desc, const std::vector<Object>& objs) {
    unsigned short nullMask = 1;
    unsigned short& nullX = *(unsigned short*)buf;
    nullX = 0;
    void* iter = (char*)buf + 2;
    for (int i = 0; i < desc.colSize; i++) {
        if (objs[i].is_null)
            nullX |= nullMask;
        else
            memcpy(iter, objs[i].loc, desc.colType[i].size);
        nullMask <<= 1;
        (char*&)iter += desc.colType[i].size;
    }
}

void WriteRow(void* rec, const TableDesc& desc) {
    unsigned short nullmap = *(unsigned short*)rec;
    (char*&)rec += 2;
    for (int i = 0; i < desc.colSize; i++) {
        const Type& t = desc.colType[i];
        if (nullmap & (1 << i)) {
            std::cout << "NULL ";
        } else {
            switch (t.type) {
                case TYPE_INT:
                    std::cout << *(int*)rec << " ";
                    break;
                case TYPE_VARCHAR:
                    for (char* p = (char*)rec; *p && p != (char*)rec + t.size; p++)
                        std::cout << *p;
                    std::cout << " ";
                    break;
            };
        }
        (char*&)rec += t.size;
    }
}

void WriteObj(Object obj) {
    if (obj.is_null) {
        std::cout << "NULL ";
    } else {
        switch (obj.type) {
            case TYPE_INT:
                std::cout << *(int*)obj.loc << " ";
                break;
            case TYPE_VARCHAR:
                for (char* p = (char*)obj.loc; *p && p != (char*)obj.loc + obj.size; p++)
                    std::cout << *p;
                std::cout << " ";
                break;
        };
    }
}

Manager::Manager() {
    dbName = "test";
    DBInfo info;
    info.tableCount = 0;
    std::fstream out("test.dbx", std::ios::in | std::ios::binary);
    if (!out) {
        out.close();
        out.open("test.dbx", std::ios::out | std::ios::binary | std::ios::trunc);
        out.write((char*)(void*)&info, sizeof(info));
    }
    out.close();
}

std::string Manager::tblFileName(const std::string& tbl) {
    return dbName + ":" + tbl + ".db";
}

void Manager::Insert(const std::string& tbl, const std::vector<std::vector<Object>>& rows) {
    Table* tblX = getTable(tbl, false);
    
    
    for (auto row : rows) {
         if (row.size() != tblX->head->desc.colSize) {
            throw "Column Size Error;";
        }
        for (int i = 0; i < row.size(); i++) {
            checkType(tblX->head->desc.colType[i], row[i]);
        }
        //check primary and insert
        if ( tblX->keyoffset != -1 ) {
            Object keyobj = row[tblX->keyoffset];
            if ( auto iter = tblX->key_object.find(keyobj) != tblX->key_object.end()) 
                throw "primary key clustered"; 
            tblX->key_object.insert(keyobj);
        }
    }
    
    for (auto row : rows) {
        void* rec = (char*)tblX->genNewRecord();
        tblX->setDirty(rec);
        WriteBinRow(rec, tblX->head->desc, row);
    }
    tblX->writeback();
}
void Manager::Delete(const std::string& tbl, const std::vector<Condition>& conds){
    Table* table = getTable(tbl, false);
    std::vector<void*> filtered = filterOne(tbl, conds);
    
    if ( table->keyoffset != -1 ) {
        ReadExpr* lv = new ReadExpr(tbl, table->head->keyname); 
        lv->Use(tbl, "", &table->head->desc);
        for ( auto record : filtered ) {
            Object keyobj = lv->getObj(record);
            auto iter = table->key_object.find(keyobj);
            if ( iter != table->key_object.end() )
                table->key_object.erase(iter);
        }
    }
    
    for ( const auto& record : filtered) {
        table->removeRecord(record);
        WriteRow(record, table->head->desc);
        std::cout << std::endl;

    }
    table->writeback();
}

void Manager::Select(const std::string& tbl1, const std::string& tbl2, const std::vector<Condition>& conds, std::vector<Expr*>* sel){
    if ( tbl2 == "" ) {
        auto filtered = filterOne(tbl1, conds);
        auto table = getTable(tbl1, false);
        if (sel != nullptr)
            for (auto& expr : *sel)
                expr->Use(tbl1, tbl2, &table->head->desc, nullptr);
        for (auto row : filtered) {
            if (sel == nullptr) {
                WriteRow(row, table->head->desc);
            } else {
                for (auto& expr : *sel)
                    WriteObj(expr->getObj(row, nullptr));
            }
            std::cout << std::endl;
        }
    } else {
        auto filtered = filterTwo(tbl1, tbl2, conds);
        auto table1 = getTable(tbl1, false);
        auto table2 = getTable(tbl2, false);
        if (sel != nullptr)
            for (auto& expr : *sel)
                expr->Use(tbl1, tbl2, &table1->head->desc, &table2->head->desc);
        for (auto row : filtered) {
            if (sel == nullptr) {
                WriteRow(row.first, table1->head->desc);
                WriteRow(row.second, table2->head->desc);
            } else {
                for (auto& expr : *sel)
                    WriteObj(expr->getObj(row.first, row.second));
            }
            std::cout << std::endl;
        }
    }
}

void Manager::Update(const std::string& tbl, const std::vector<Condition>& conds, ReadExpr& lv, const Object& rv){
    std::vector<void*> filtered = filterOne(tbl, conds);
    if ( filtered.size() == 0 )
        return;
    
    Table* table = getTable(tbl, false);
    lv.Use(tbl, "", &table->head->desc);
    
    if ( lv.type != rv.type ) {
        throw "Type Check Error";
    }

    
    //check whether the update is primary key : rv is primary key and is not ""
    // realize : if it exists in key_obj but not in filtered

    if ( rv.is_null ) {
        for ( void* record : filtered ) {
            *(unsigned short*)record |= lv.nullMask;
            table->setDirty(record);
        }
        table->writeback();
        return;
    }
  
    if ( std::strcmp(lv.name.c_str(), table->head->keyname) == 0
        && table->keyoffset != -1 ) {
        // construct set of filtered key objs;
        
        if ( filtered.size() > 1 )
            throw "primary key clustered";
            
        Object obj = lv.getObj(filtered[0]);
        
        auto iter = table->key_object.find(rv);
        if ( iter != table->key_object.end() && rv != obj ) 
            throw "primary key clustered";

        table->key_object.erase(table->key_object.find(obj));
        table->key_object.insert(rv);
    }
    
    for ( void* record : filtered ) {
        *(unsigned short*)record &= ~lv.nullMask;
        Object obj = lv.getObj(record);
        memcpy(obj.loc, rv.loc, lv.size);
        table->setDirty(record);
    }
    table->writeback();
}


void Manager::Use(const std::string& db) {
    dbName = db;
}


void Manager::CreateTable(const std::string& tbl, const std::vector<Type>& types, const std::string& key_name){
    Table* table = getTable(tbl, true);
    table->head->desc.colSize = types.size();
    table->rowSize = Table::RowBitmapSize;
    std::strcpy(table->head->keyname, key_name.c_str());
    for ( int i=0; i<types.size(); i++ ) {
        table->head->desc.colType[i] = types[i];
        table->rowSize += types[i].size;
    }
    table->setDirty(0);
    table->writeback();
    table->initKey();
    std::fstream dbf(dbName + ".dbx", std::ios::in | std::ios::out | std::ios::binary);
    DBInfo info;
    dbf.read((char*)(void*)&info, sizeof(info));
    strcpy(info.tables[info.tableCount++], tbl.c_str());
    dbf.seekp(0);
    dbf.write((char*)(void*)&info, sizeof(info));
}


void Manager::DropTable(const std::string& tbl) {
    tables.erase(tblFileName(tbl));
    std::remove(tblFileName(tbl).c_str());
    std::fstream dbf(dbName + ".dbx", std::ios::in | std::ios::out | std::ios::binary);
    DBInfo info;
    dbf.read((char*)(void*)&info, sizeof(info));
    for (int i = 0; i < info.tableCount; i++)
        if (info.tables[i] == tbl)
            memcpy(info.tables[i], info.tables[--info.tableCount], 20);
    dbf.seekp(0);
    dbf.write((char*)(void*)&info, sizeof(info));
}

void Manager::ShowTables() {
    std::fstream dbf(dbName + ".dbx", std::ios::in | std::ios::out | std::ios::binary);
    DBInfo info;
    dbf.read((char*)(void*)&info, sizeof(info));
    for (int i = 0; i < info.tableCount; i++)
        std::cout << info.tables[i] << std::endl;
}

void Manager::CreateDB(const std::string& dbName) {
    std::fstream out(dbName + ".dbx", std::ios::out | std::ios::binary | std::ios::trunc);
    DBInfo info;
    info.tableCount = 0;
    out.write((char*)(void*)&info, sizeof(info));
    out.close();
}

void Manager::DropDB(const std::string& dbName) {
    std::remove((dbName + ".dbx").c_str());
}

void Manager::Desc(const std::string& tbl) {
    Table* table = getTable(tbl, false);
    for (int i = 0; i < table->head->desc.colSize; i++) {
        std::cout << table->head->desc.colType[i].name;
        if (table->head->desc.colType[i].type == TYPE_INT)
            std::cout << " INT";
        else
            std::cout << " VARCHAR";
        if (table->head->desc.colType[i].null)
            std::cout << " NULL";
        else
            std::cout << " NOT NULL";
        std::cout << std::endl;
    }
}

Table* Manager::getTable(const std::string& tbl, bool init){
    std::string t_tbl = tblFileName(tbl);
    if (init) {
        auto& ptbl = tables[t_tbl];
        if (ptbl == nullptr) {
            ptbl = new Table(t_tbl, true);
            return ptbl;
        }
        throw "Table Already Exist";
    } else {
        auto& ptbl = tables[t_tbl];
        if (ptbl == nullptr) {
            ptbl = new Table(t_tbl, false);
        }
        return ptbl;
    }
}

std::vector<void*> Manager::filterOne(const std::string& tbl, const std::vector<Condition>& conds) {
    std::vector<void*> filtered;
    Table* table = getTable(tbl, false);
    for (int k = 0; k < conds.size(); k++) {
        conds[k].l->Use(tbl, "", &table->head->desc);
        conds[k].r->Use(tbl, "", &table->head->desc);
    }
    for ( auto record : table->usedRecords ) {
        bool OK = true;
       for (int k = 0; k<conds.size(); k++ ) {
           Expr *l = conds[k].l;
           Expr *r = conds[k].r;
           if (!conds[k].op(l->getObj(record, nullptr), r->getObj(record, nullptr))) {
               OK = false;
               break;
           }
       }
       if (OK)
           filtered.push_back(record);
    }
    return std::move(filtered);
}


std::vector<std::pair<void*, void*>> Manager::filterTwo(const std::string& tbl1, const std::string& tbl2, const std::vector<Condition>& conds) {
    std::vector<std::pair<void*, void*>> filtered;
    Table* table1 = getTable(tbl1, false);
    Table* table2 = getTable(tbl2, false);
    for (int k = 0; k < conds.size(); k++) {
        conds[k].l->Use(tbl1, tbl2, &table1->head->desc, &table2->head->desc);
        conds[k].r->Use(tbl1, tbl2, &table1->head->desc, &table2->head->desc);
    }
    for ( auto record1 : table1->usedRecords ) {
        for (auto record2 : table2->usedRecords) {
            bool OK = true;
            for (int k = 0; k<conds.size(); k++ ) {
                Expr *l = conds[k].l;
                Expr *r = conds[k].r;
                if (!conds[k].op(l->getObj(record1, record2),
                            r->getObj(record1, record2))) {
                    OK = false;
                    break;
                }
            }
            if (OK)
                filtered.push_back(std::make_pair(record1, record2));
        }
    }
    return std::move(filtered);
}

