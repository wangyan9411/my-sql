#ifndef TEST
#include "parser.hpp"
#include <string>
#include <iostream>
#include <fstream>

void RunStmt(const std::string& sql, Parser& p, Manager& manager) {
    try {
        auto X = p.parse(sql);
        X->Run(manager);
    } catch (const char* t) {
        std::cout << t << std::endl;
    }
}

int main(int argc, char** argv) {
    Manager manager;
    Parser parser;
    if (argc == 2) {
        std::ifstream input(argv[1]);
        if (!input) {
            std::cout << "File Not Found" << std::endl;
            return 0;
        }
        std::vector<char> sql;
        char c = '\0';
        while (input) {
            int x = input.get();
            if (x == -1)
                break;
            if (c == '\0' && x == ';') {
                RunStmt(std::string(sql.begin(), sql.end()), parser, manager);
                sql.clear();
            } else {
                if (c == '\0' && x == '\'')
                    c = '\'';
                else if (c == '\0' && x == '"')
                    c = '"';
                else if (c == '\'' && x == '\'')
                    c = '\0';
                else if (c == '"' && x == '"')
                    c = '\0';
                sql.push_back(x);
            }
        }
    } else {
        while (true) {
            std::string sql;
            std::cout << ">> ";
            std::getline(std::cin, sql);
            if (!std::cin)
                break;
            if (sql.substr(0, 4) == "exit")
                break;
            if (!sql.empty())
                RunStmt(sql, parser, manager);
        }
    }
}
#endif
