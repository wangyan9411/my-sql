#ifndef PARSER_H
#define PARSER_H
#include "stmt.hpp"
struct Token {
    enum Type {
        USE,
        DROP,
        DATABASE,
        TABLE,
        SET,
        CREATE,
        SHOW,
        DESC,
        DELETE,
        UPDATE,
        INSERT,
        VARCHAR_LIT,
        INT_LIT,
        OPER,
        ID,
        SELECT,
        WHERE,
        FROM,
        AND,
        INTO,
        VALUES,
        PRIMARY,
        INT,
        VARCHAR,
        NULL_LIT,
    } token;
    std::string raw;
};
struct Parser {
    LiteralManager litManager;
    typedef std::vector<Token> TokenList;
    typedef TokenList::iterator TokenIter;
    void clear();
    Stmt* parse(const std::string& sql);
    TokenList tokenize(const std::string& sql);
    Stmt* parseSQL(TokenIter beg, TokenIter end);
    SelectStmt* parseSelect(TokenIter beg, TokenIter end);
    DeleteStmt* parseDelete(TokenIter beg, TokenIter end);
    UpdateStmt* parseUpdate(TokenIter beg, TokenIter end);
    InsertStmt* parseInsert(TokenIter beg, TokenIter end);
    ShowTblStmt* parseShow(TokenIter beg, TokenIter end);
    DescStmt* parseDesc(TokenIter beg, TokenIter end);
    CreateTableStmt* parseCreateTable(TokenIter beg, TokenIter end);
    DropTableStmt* parseDropTable(TokenIter beg, TokenIter end);
    CreateDBStmt* parseCreateDB(TokenIter beg, TokenIter end);
    DropDBStmt* parseDropDB(TokenIter beg, TokenIter end);
    UseStmt* parseUse(TokenIter beg, TokenIter end);
    
    std::pair<std::string, std::string> parseFrom(TokenIter beg, TokenIter end);
    std::vector<Condition> parseWhere(TokenIter beg, TokenIter end);
    std::string parseTableName(TokenIter beg, TokenIter end);
    std::pair<ReadExpr*, Object> parseSet(TokenIter beg, TokenIter end);
    Condition parseCond(TokenIter beg, TokenIter end);
    std::vector<Expr*>* parseExprs(TokenIter beg, TokenIter end);
    Expr* parseExpr(TokenIter beg, TokenIter end);
    std::vector<std::vector<Object>> parseRows(TokenIter beg, TokenIter end);
    std::vector<Object> parseRow(TokenIter beg, TokenIter end);
    std::pair<std::vector<Type>, std::string> parseTypes(TokenIter beg, TokenIter end);
    Type parseType(TokenIter beg, TokenIter end);
    TokenIter findToken(TokenIter beg, TokenIter end, Token::Type token, const std::string& raw = "");
};
#endif
