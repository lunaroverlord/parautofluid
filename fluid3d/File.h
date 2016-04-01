/*
 * File class (from another personal project)
 */
#pragma once 

#include "main.h"

class File
{
public:
        File(string);
	//File(char fileName[]) : File(string(fileName)) {}
        ~File();

        char* ReadAll();
        int GetLength() {return length;}

private:
        string fName;
        ifstream in;
        char* buffer;
        int length;
};
