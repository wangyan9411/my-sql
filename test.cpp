#ifdef TEST
#include "manager.hpp"
#include <vector>
#include <iostream>
#include <cstdio>
#include "parser.hpp"

int main() {
    std::remove("test:table1.db");
    std::remove("test:table2.db");
    std::remove("test.dbx");
    Manager manager;

    Parser p;
    p.parse("create table table1 (id int(10), name varchar(10), primary key (id))")->Run(manager);
    p.parse("create table table2 (id int(10), name varchar(10), primary key (id))")->Run(manager);
    p.parse("insert into table1 values (3, 'wy'), (2, 'have fun')")->Run(manager);
    p.parse("insert into table2 values (3, 'wy'), (2, 'have fun')")->Run(manager);
    p.parse("select * from table1")->Run(manager);
    p.parse("update table1 set id = 1 where name = 'wy'")->Run(manager);
    p.parse("select * from table1")->Run(manager);
    p.parse("insert into table1 values (12, 'aaa'), (21, 'wangyan')")->Run(manager);
    p.parse("select * from table1")->Run(manager);
    p.parse("select table1.id, table2.id from table1, table2 where table1.name = table2.name")->Run(manager);
    p.parse("create database wyy")->Run(manager);
    p.parse("create database wyyy")->Run(manager);
    p.parse("show tables")->Run(manager);
    p.parse("drop table table2")->Run(manager);
    p.parse("show tables")->Run(manager);
    p.parse("desc table1")->Run(manager);
    p.parse("drop database wyyy")->Run(manager);
    p.parse("use wyy")->Run(manager);
    p.parse("show tables")->Run(manager);

    //manager.Delete("test", conds);
}
#endif
