#include "table.hpp"
#include <fstream>
#include <iostream>

void Table::initKey() {
        if ( std::strcmp(head->keyname, "") == 0 ) {
            keyoffset = -1;
            return;
        }
        keyoffset = 0;
        for ( keyoffset=0; keyoffset<head->desc.colSize; keyoffset++ ) {
            if (std::strcmp(head->desc.colType[keyoffset].name, head->keyname) == 0)
                break;
        }
}

Table::Table(const std::string& _filename, bool init) : filename(_filename) {
    std::ifstream in(filename, std::ios::in | std::ios::binary);
    if (init) {
        if (in)
            throw "Table Already Exist!";
        in.close();
        std::fstream out(filename, std::ios::out | std::ios::binary);
        head = (HeadPage*)new char[PAGE_SIZE];
        LastInfoPage = (InfoPage*)new char[PAGE_SIZE];
        memset(head, 0, PAGE_SIZE);
        memset(LastInfoPage, 0, PAGE_SIZE);
        head->infoHeadPage = 1;
        head->pageCount = 2;
        out.write((char*)(void*)head, PAGE_SIZE);
        out.seekp(PAGE_SIZE);
        out.write((char*)(void*)LastInfoPage, PAGE_SIZE);
        out.close();
        rowSize = 0;
        pages.push_back(head);
        pages.push_back(LastInfoPage);
        pageIndex[head] = 0;
        pageIndex[LastInfoPage] = 1;
    } else {
        if (!in)
            throw "Table Not Found!";
        head = (HeadPage*)new char[PAGE_SIZE];
        in.read((char*)(void*)head, PAGE_SIZE);
        pages.push_back(head);
        pageIndex[head] = 0;
        //Read Pages
        for (int i = 1; i < head->pageCount; i++) {
            char* buf = new char[PAGE_SIZE];
            in.read(buf, PAGE_SIZE);
            pages.push_back(buf);
            pageIndex[buf] = i;
        }
        
        // get key object;
        int offset = 2;
        initKey();
        
        for ( int i=0; i<keyoffset; i++ )
            offset += head->desc.colType[i].size;
        int nullMask = 1 << keyoffset;
        TYPE type = head->desc.colType[keyoffset].type;
        int size = head->desc.colType[keyoffset].type;


        int infoIdx = head->infoHeadPage;
        while (infoIdx != 0) {
            InfoPage* infos = (InfoPage*)(void*)pages[infoIdx];
            infoIdx = infos->nextPage;
            for (int i = 0; i < infos->size; i++) {
                Info* info = infos->infos + i;
                void* rec = (char*)pages[info->page_id] + info->offset;
                recordInfoMap[rec] = info;
                if (info->free)
                    emptyRecords.insert(rec);
                else {
                    Object ret;
                    ret.size = size;
                    ret.is_null = nullMask & *(unsigned short*)rec;
                    ret.loc = (char*)rec + offset;
                    ret.type = type;
                    key_object.insert(ret);
                    usedRecords.insert(rec);
                }
            }
            LastInfoPage = infos;
        }
        rowSize = Table::RowBitmapSize;
        for (int i = 0; i < head->desc.colSize; i++)
            rowSize += head->desc.colType[i].size;
    }
}

void Table::setDirty(void* dst) {
    auto it = pageIndex.upper_bound(dst);
    it--;
    dirtyPages.insert(it->second);
}
void Table::setDirty(int page_id) {
    dirtyPages.insert(page_id);
}
void Table::writeback() {
    std::ofstream out(filename, std::ios::binary | std::ios::out | std::ios::in);
    for (auto page_id : dirtyPages) {
        out.seekp(page_id * PAGE_SIZE);
        out.write((char*)pages[page_id], PAGE_SIZE);
    }
    out.close();
}
void* Table::getPage(int page_id) {
    return pages[page_id];
}
void* Table::genNewRecord() {
    if (emptyRecords.empty()) {
        int new_page = newPage();
        char* page = (char*)getPage(new_page);
        for (int i = 0; i + rowSize <= PAGE_SIZE; i += rowSize) {
            if (LastInfoPage->size == MaxInfo) {
                setDirty(LastInfoPage);
                int new_info_page = newPage();
                LastInfoPage->nextPage = new_info_page;
                LastInfoPage = (InfoPage*)getPage(new_info_page);
            }
            Info* info = LastInfoPage->infos + LastInfoPage->size++;
            info->page_id = new_page;
            info->offset = i;
            info->free = true;
            void* rec = page + info->offset;
            emptyRecords.insert(rec);
            recordInfoMap[rec] = info;
        }
        setDirty(LastInfoPage);
    }
    Info* info = recordInfoMap[*emptyRecords.begin()];
    void* rec = (char*)getPage(info->page_id) + info->offset;
    info->free = false;
    emptyRecords.erase(rec);
    usedRecords.insert(rec);
    setDirty(info);
    return rec;
}
void Table::removeRecord(void* rec) {
    Info* info = recordInfoMap[rec];
    info->free = true;
    emptyRecords.insert(rec);
    usedRecords.erase(rec);
}
int Table::newPage() {
    char* buf = new char[PAGE_SIZE];
    memset(buf, 0, PAGE_SIZE);
    pageIndex[buf] = head->pageCount;
    pages.push_back(buf);
    head->pageCount++;
    setDirty(0);
    setDirty(head->pageCount - 1);
    return head->pageCount - 1;
}

