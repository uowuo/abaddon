#include "yyjson_util.h"


YYJsonArray::YYJsonArray()
{
    mutdoc = yyjson_mut_doc_new(NULL);
    mutarr = yyjson_mut_arr(mutdoc);
}

void YYJsonArray::AddBlock(yyjson_mut_val* block)
{
    yyjson_mut_arr_add_val(mutarr, block);
}


YYJsonDocument::YYJsonDocument()
{

}

YYJsonDocument::~YYJsonDocument()
{
    DestroyDoc();
}

//yyjson_doc* YYJsonDocument::ReadString(std::string str)
//{
//
//}


void YYJsonDocument::CreateDoc(bool CleanOnDelete)
{
    mutdoc = yyjson_mut_doc_new(NULL);
    DataBlocks.push_back(yyjson_mut_obj(mutdoc));
    yyjson_mut_doc_set_root(mutdoc, DataBlocks[currentBlockIndex]);
    BuildingMode = true;
    CleanAfterDone = CleanOnDelete;
}

std::string YYJsonDocument::BuildJson()
{
    if(BuildingMode)
        return yyjson_mut_write(mutdoc,YYJSON_WRITE_ALLOW_INF_AND_NAN,NULL);
    return std::string();
}


void YYJsonDocument::AddString(const char* name, const char* value)
{
    if(BuildingMode)
        yyjson_mut_obj_add_str(mutdoc, DataBlocks[currentBlockIndex], name, value);

}

//void YYJsonDocument::AddString(const char* name, std::string value)
//{
//    if(BuildingMode)
//        yyjson_mut_obj_add_str(mutdoc, DataBlocks[DataBlocks.size()-1], name, value.c_str());
//
//}



void YYJsonDocument::AddString(const char* name, std::optional<std::string> value)
{
    if(BuildingMode)
    {
        do {
            if ((value).has_value())
                yyjson_mut_obj_add_str(mutdoc, DataBlocks[currentBlockIndex], name, (*(value)).c_str());
        } while (0);
    }

}


//void YYJsonDocument::AddInt(const char* name, int value)
//{
//    if(BuildingMode)
//        yyjson_mut_obj_add_int(mutdoc, DataBlocks[DataBlocks.size()-1], name, value);
//
//}

void YYJsonDocument::AddInt(const char* name, std::optional<int> value)
{

    if(BuildingMode)
    {
        do {
            if ((value).has_value())
                yyjson_mut_obj_add_int(mutdoc, DataBlocks[currentBlockIndex], name, *(value));
        } while (0);
    }

}

//void YYJsonDocument::AddBool(const char* name, bool value)
//{
//    if(BuildingMode)
//        yyjson_mut_obj_add_bool(mutdoc, DataBlocks[DataBlocks.size()-1], name, value);
//
//}

void YYJsonDocument::AddBool(const char* name, std::optional<bool> value)
{

    if(BuildingMode)
    {
        do {
            if ((value).has_value())
                yyjson_mut_obj_add_bool(mutdoc, DataBlocks[currentBlockIndex], name, *(value));
        } while (0);
    }
}

void YYJsonDocument::AddBlock(const char* name, yyjson_mut_val* block)
{
    if(BuildingMode)
    yyjson_mut_obj_add_val(mutdoc, DataBlocks[currentBlockIndex], name, block);
}

void YYJsonDocument::AddNull(const char* name)
{
    if(BuildingMode)
    yyjson_mut_obj_add_null(mutdoc,DataBlocks[currentBlockIndex],name);
}

void YYJsonDocument::CreateArrayEmpty(const char* name)
{
    if(BuildingMode)
    yyjson_mut_obj_add_arr(mutdoc,DataBlocks[currentBlockIndex],name);
}

void YYJsonDocument::AddArray(const char* name, yyjson_mut_val* array)
{
    if(BuildingMode)
    {
        yyjson_mut_obj_add_val(mutdoc,DataBlocks[currentBlockIndex],name, array);
    }
}

void YYJsonDocument::CreateBlock()
{
    if(BuildingMode){
    DataBlocks.push_back(yyjson_mut_obj(mutdoc));
    currentBlockIndex++;
    }
}

void YYJsonDocument::PushBlock(const char* str)
{
    if(BuildingMode){
    yyjson_mut_obj_add_val(mutdoc, DataBlocks[currentBlockIndex-1], str, DataBlocks[currentBlockIndex]);
    currentBlockIndex--;
    }
}


