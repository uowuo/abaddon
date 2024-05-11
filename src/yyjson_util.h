#pragma once
#ifndef YYJSON_UTIL_H
#define YYJSON_UTIL_H


#include "gtkmm/main.h"
#include "yyjson.h"
#include "vector"

class YYJsonArray
{
public:
    YYJsonArray();

    //void AddString(const char* name, const char* value);
    //void AddString(const char* name, std::optional<std::string> value);
    //void AddInt(const char* name, std::optional<int> value);
    //void AddBool(const char* name, std::optional<bool> value);
    void AddBlock(yyjson_mut_val* block);
    yyjson_mut_val* GetArray(){return mutarr;};
    yyjson_mut_doc* GetDoc(){return mutdoc;};

private:
    yyjson_mut_doc* mutdoc = nullptr;
    yyjson_mut_val* mutarr = nullptr;

};

class YYJsonDocument
{
public:
    YYJsonDocument();
    ~YYJsonDocument();
    //yyjson_doc* ReadString(std::string str);
    void CreateDoc(bool CleanOnDelete = true);

    yyjson_mut_doc* GetMutableDoc(){return mutdoc;}
    yyjson_mut_val* GetMutableRoot(){return DataBlocks[currentBlockIndex];}

    yyjson_doc* GetDoc()
    {
        if(!BuildingMode)
            return doc;
        return doc = yyjson_mut_doc_imut_copy(mutdoc, &mutdoc->alc);
    }
    yyjson_val* GetRoot(){return root;}

    std::string BuildJson();
    void DestroyDoc( bool force = false)
    {
        if(CleanAfterDone || force)
        {
        if(mutdoc != nullptr)
            yyjson_mut_doc_free(mutdoc);

        if(doc != nullptr)
            yyjson_doc_free(doc);

        //DataBlocks.clear();

        //if(root != nullptr)
        //    delete root;
        }

    }


    void AddString(const char* name, const char* value);
    //void AddString(const char* name, std::string value);
    void AddString(const char* name, std::optional<std::string> value);
    //void AddInt(const char* name, int value);
    void AddInt(const char* name, std::optional<int> value);
    //void AddBool(const char* name, bool value);
    void AddBool(const char* name, std::optional<bool> value);
    void AddBlock(const char* name, yyjson_mut_val* block);

    void AddNull(const char* name);
    void CreateArrayEmpty(const char* name);

    void AddArray(const char* name, yyjson_mut_val* array);

    void CreateBlock();
    void PushBlock(const char* str);



private:

    bool BuildingMode = true;
    yyjson_doc* doc = nullptr;
    yyjson_val* root = nullptr;

    bool CleanAfterDone = true;

    yyjson_mut_doc* mutdoc = nullptr;
    //yyjson_mut_val* mutroot = nullptr;

    std::vector<yyjson_mut_val*> DataBlocks;
    size_t currentBlockIndex=0;
};

#endif // YYJSON_UTIL_H
