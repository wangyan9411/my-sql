#include <iostream>
#include <cstdlib>
#include <unordered_map>

#include "parser.hpp"


Stmt* Parser::parse(const std::string& sql) {
    TokenList l  = tokenize(sql);
    return parseSQL(l.begin(), l.end());
}
static const std::unordered_map<std::string, Token::Type> specialToken = {
    {"use", Token::USE},
    {"drop", Token::DROP},
    {"create", Token::CREATE},
    {"table", Token::TABLE},
    {"databases", Token::DATABASE},
    {"show", Token::SHOW},
    {"desc", Token::DESC},
    {"insert", Token::INSERT},
    {"into", Token::INTO},
    {"values", Token::VALUES},
    {"update", Token::UPDATE},
    {"set", Token::SET},
    {"select", Token::SELECT},
    {"delete", Token::DELETE},    
    {"where", Token::WHERE},
    {"from", Token::FROM},
    {"and", Token::AND},
    {"primary", Token::PRIMARY},
    {"int", Token::INT},
    {"varchar", Token::VARCHAR},
    {"null", Token::NULL_LIT},
};
static const std::unordered_set<char> IDChar = {
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',
    'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '_',
};
static const std::unordered_set<std::string> Op = {
    ">", "<", "=", "!=", ">=", "<=", ",", ".",  "*", "(", ")", ";",
};
static std::string toLower(const std::string& str) {
    std::string ret;
    ret.resize(str.size());
    for (int i = 0; i < str.size(); i++) {
        char ch = str[i];
        if (ch >= 'A' && ch <= 'Z')
            ch ^= 0x20;
        ret[i] = ch;
    }
    return std::move(ret);
}
Parser::TokenList Parser::tokenize(const std::string& sql) {
    TokenList ret;
    auto iter = sql.begin();
    while (true) {
        for (; iter != sql.end() && isspace(*iter); iter++);
        if (iter == sql.end())
            break;
        Token t;
        if (*iter == '"' || *iter == '\'') {
            char flag = *iter;
            auto beg = ++iter;
            for (; iter != sql.end() && *iter != flag; iter++);
            if (iter == sql.end())
                throw "String Parse Error";
            t.raw = std::string(beg, iter++);
            t.token = Token::VARCHAR_LIT;
        } else if (isdigit(*iter)) {
            auto beg = iter;
            for (; iter != sql.end() && isdigit(*iter); iter++);
            t.raw = std::string(beg, iter);
            t.token = Token::INT_LIT;
        } else if (IDChar.find(*iter) != IDChar.end()) {
            auto beg = iter;
            for (; iter != sql.end() && IDChar.find(*iter) != IDChar.end(); iter++);
            t.raw = toLower(std::string(beg, iter));
            auto i = specialToken.find(t.raw);
            if (i != specialToken.end()) {
                t.token = i->second;
            } else {
                t.token = Token::ID;
            }
        } else if (iter + 1 != sql.end() && Op.find(std::string(iter, iter + 2)) != Op.end()) {
            t.raw = std::string(iter, iter + 2);
            t.token = Token::OPER;
            iter += 2;
        } else if (Op.find(std::string(iter, iter + 1)) != Op.end()) { 
            t.raw = std::string(iter, iter + 1);
            t.token = Token::OPER;
            iter += 1;
        } else {
            throw "Tokenize Error";
        }
        ret.push_back(t);
    }
    return ret;
}
Stmt* Parser::parseSQL(Parser::TokenIter beg, Parser::TokenIter end) {
    switch (beg->token) {
        case Token::SELECT:
            return parseSelect(beg + 1, end);
        case Token::DELETE:
            return parseDelete(beg + 1, end);
        case Token::UPDATE:
            return parseUpdate(beg + 1, end);
        case Token::INSERT:
            return parseInsert(beg + 1, end);
        case Token::USE:
            return parseUse(beg + 1, end);
        case Token::SHOW:
            return parseShow(beg + 1, end);
        case Token::DESC:
            return parseDesc(beg + 1, end);
        case Token::CREATE:
            beg = beg + 1;
            if (beg->token == Token::TABLE) {
                return parseCreateTable(beg + 1, end);
            } else {
                return parseCreateDB(beg + 1, end);
            }
        case Token::DROP:
            beg = beg + 1;
            if (beg->token == Token::TABLE) {
                return parseDropTable(beg + 1, end);
            } else {
                return parseDropDB(beg + 1, end);
            }
            
        default:
            throw "Syntax Error";
    }
}

UpdateStmt* Parser::parseUpdate(Parser::TokenIter beg, Parser::TokenIter end) {
    auto setLoc = findToken(beg, end, Token::SET);
    auto whereLoc = findToken(setLoc, end, Token::WHERE);
    auto tablename = parseTableName(beg, setLoc);
    auto set = parseSet(setLoc + 1, whereLoc);
    auto where = parseWhere(whereLoc + 1, end);
    UpdateStmt* ret = new UpdateStmt;
    ret->tbl = tablename;
    ret->lv = *set.first;
    ret->obj = set.second;
    ret->conds = std::move(where);
    return ret;
}


DeleteStmt* Parser::parseDelete(Parser::TokenIter beg, Parser::TokenIter end) {
    auto fromLoc = findToken(beg, end, Token::FROM);
    auto whereLoc = findToken(fromLoc, end, Token::WHERE);
    auto from = parseFrom(fromLoc + 1, whereLoc);
    auto where = parseWhere(whereLoc + 1, end);
    DeleteStmt* ret = new DeleteStmt;
    ret->tbl = from.first;
    //ret->tbl2 = from.second;
    ret->conds = std::move(where);
    return ret;
}

SelectStmt* Parser::parseSelect(Parser::TokenIter beg, Parser::TokenIter end) {
    auto fromLoc = findToken(beg, end, Token::FROM);
    auto whereLoc = findToken(fromLoc, end, Token::WHERE);
    auto from = parseFrom(fromLoc + 1, whereLoc);
    auto where = parseWhere(whereLoc + 1, end);
    SelectStmt* ret = new SelectStmt;
    ret->tbl1 = from.first;
    ret->tbl2 = from.second;
    ret->conds = std::move(where);
    ret->exprs = parseExprs(beg, fromLoc);
    return ret;
}
InsertStmt* Parser::parseInsert(Parser::TokenIter beg, Parser::TokenIter end) {
    InsertStmt* ret = new InsertStmt;
    ret->tbl = (beg + 1)->raw;
    ret->rows = Parser::parseRows(beg + 3, end);
    return ret;
}
std::pair<std::string, std::string> Parser::parseFrom(Parser::TokenIter beg, Parser::TokenIter end) {
    auto COMMA = findToken(beg, end, Token::OPER, ",");
    if (COMMA == end)
        return std::make_pair(beg->raw, "");
    else
        return std::make_pair(beg->raw, (COMMA + 1)->raw);
}

std::string Parser::parseTableName(Parser::TokenIter beg, Parser::TokenIter end) {
    return beg->raw;
}

std::vector<Condition> Parser::parseWhere(Parser::TokenIter beg, Parser::TokenIter end) {
    std::vector<Condition> ret;
    if (beg < end) {
        while (true) {
            auto COMMA = findToken(beg, end, Token::AND);
            ret.push_back(parseCond(beg, COMMA));
            if (COMMA == end)
                break;
            beg = COMMA + 1;
        }
    }
    return std::move(ret);
}
Condition Parser::parseCond(Parser::TokenIter beg, Parser::TokenIter end) {
    Condition ret;
    auto iter = beg;
    TokenIter OPER;
    while (true) {
        OPER = findToken(iter, end, Token::OPER);
        if (OPER == end)
            throw "Syntax Error";
        if (OPER->raw == "="
                or OPER->raw == "is"
                or OPER->raw == "IS") {
            ret.op = op_eq;
        } else if (OPER->raw == "!=") {
            ret.op = op_ne;
        } else if (OPER->raw == "<") {
            ret.op = op_lt;
        } else if (OPER->raw == ">") {
            ret.op = op_gt;
        } else if (OPER->raw == "<=") {
            ret.op = op_le;
        } else if (OPER->raw == ">=") {
            ret.op = op_ge;
        } else {
            iter = OPER + 1;
            continue;
        }
        break;
    }
    ret.l = parseExpr(beg, OPER);
    ret.r = parseExpr(OPER + 1, end);
    return ret;
}

ShowTblStmt* Parser::parseShow(TokenIter beg, TokenIter end){
    ShowTblStmt* ret = new ShowTblStmt;
    return ret;
}

DescStmt* Parser::parseDesc(TokenIter beg, TokenIter end){
    DescStmt* ret = new DescStmt;
    ret->tbl = beg->raw;
    return ret;
}

CreateTableStmt* Parser::parseCreateTable(TokenIter beg, TokenIter end){
    CreateTableStmt* ret = new CreateTableStmt;
    ret->tbl = beg->raw;
    auto types = Parser::parseTypes(beg + 2, end - 1);
    ret->types = std::move(types.first);
    ret->key = std::move(types.second);
    return ret;
}

DropTableStmt* Parser::parseDropTable(TokenIter beg, TokenIter end){
    DropTableStmt* ret = new DropTableStmt;
    ret->tbl = beg->raw;
    return ret;
}

CreateDBStmt* Parser::parseCreateDB(TokenIter beg, TokenIter end){
    CreateDBStmt* ret = new CreateDBStmt;
    ret->db = beg->raw;
    return ret;
}

UseStmt* Parser::parseUse(TokenIter beg, TokenIter end){
    UseStmt* ret = new UseStmt;
    ret->db = beg->raw;
    return ret;
}


DropDBStmt* Parser::parseDropDB(TokenIter beg, TokenIter end){
    DropDBStmt* ret = new DropDBStmt;
    ret->db = beg->raw;
    return ret;
}

std::pair<ReadExpr*, Object> Parser::parseSet(Parser::TokenIter beg, Parser::TokenIter end) {
    auto iter = beg;
    TokenIter OPER;
   // while (true) {
        OPER = findToken(iter, end, Token::OPER);
        if (OPER == end)
            throw "Syntax Error";
        if (OPER->raw == "=") {
           
        } else {
            throw "Syntax Error";
        }
   // }
    ReadExpr* readexpr = (ReadExpr*)parseExpr(beg, OPER);
    Object obj = ((LiteralExpr*)parseExpr(OPER + 1, end))->obj;
    return std::pair<ReadExpr*, Object>(readexpr, obj);
}


std::vector<Expr*>* Parser::parseExprs(Parser::TokenIter beg, Parser::TokenIter end) {
    if (beg->raw == "*")
        return nullptr;
    std::vector<Expr*>* ret = new std::vector<Expr*>;
    while (true) {
        auto iter = findToken(beg, end, Token::OPER, ",");
        ret->push_back(parseExpr(beg, iter));
        if (iter == end)
            break;
        beg = iter + 1;
    }
    return ret;
}
Expr* Parser::parseExpr(Parser::TokenIter beg, Parser::TokenIter end) {
    if (end == beg + 1) {
        if (beg->token == Token::ID)
            return new ReadExpr(beg->raw);
        if (beg->token == Token::VARCHAR_LIT)
            return new LiteralExpr(litManager.GetVarChar(beg->raw));
        if (beg->token == Token::INT_LIT)
            return new LiteralExpr(litManager.GetInt(std::stoi(beg->raw)));
        if (beg->token == Token::NULL_LIT)
            return new LiteralExpr(litManager.GetNull());
        throw "Syntax Error";
    } else if (end == beg + 3) {
        return new ReadExpr(beg->raw, (beg + 2)->raw);
    } else {
        throw "Syntax Error";
    }
}

std::vector<std::vector<Object>> Parser::parseRows(Parser::TokenIter beg, Parser::TokenIter end) {
    std::vector<std::vector<Object>> ret;
    auto iter = beg, XBeg = beg;
    bool eor = false;
    while (XBeg != end) {
        iter = findToken(XBeg, end, Token::OPER, ")");
        ret.push_back(parseRow(XBeg + 1, iter));
        XBeg = findToken(iter, end, Token::OPER, "(");
    }
    return std::move(ret);
}
std::vector<Object> Parser::parseRow(Parser::TokenIter beg, Parser::TokenIter end) {
    std::vector<Object> ret;
    for (auto iter = beg; iter < end; iter += 2) {
        if (iter->token == Token::VARCHAR_LIT)
            ret.push_back(litManager.GetVarChar(iter->raw));
        else if (iter->token == Token::INT_LIT)
            ret.push_back(litManager.GetInt(std::stoi(iter->raw)));
        else if (iter->token == Token::NULL_LIT)
            ret.push_back(litManager.GetNull());
        else
            throw "Syntax Error";
    }
    return std::move(ret);
}
std::pair<std::vector<Type>, std::string> Parser::parseTypes(TokenIter beg, TokenIter end) {
    std::vector<Type> ret;
    std::string pri = "";
    while (true) {
        auto iter = findToken(beg, end, Token::OPER, ",");
        if (beg->token == Token::PRIMARY) {
            pri = (beg + 3)->raw;
        } else {
            ret.push_back(parseType(beg, iter));
        }
        if (iter == end)
            break;
        beg = iter + 1;
    }
    return std::make_pair(std::move(ret), pri);
}
Type Parser::parseType(TokenIter beg, TokenIter end) {
    Type ret;
    strcpy(ret.name, beg->raw.c_str());
    beg++;
    if (beg->token == Token::INT) {
        ret.type = TYPE_INT;
        ret.size = 4;
        beg += 4;
    } else {
        ret.type = TYPE_VARCHAR;
        beg += 2;
        ret.size = std::stoi(beg->raw);
        beg += 2;
    }
    ret.null = (beg == end) || (beg->token == Token::NULL_LIT);
    return ret;
}
Parser::TokenIter Parser::findToken(Parser::TokenIter beg, Parser::TokenIter end, Token::Type token, const std::string& raw) {
    for (auto iter = beg; iter != end; iter++)
        if (iter->token == token && (raw == "" || iter->raw == raw))
            return iter;
    return end;
}
