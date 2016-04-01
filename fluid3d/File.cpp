#include "File.h"

File::File(string fileName)
{
        fName = fileName;
        buffer = NULL;
        length = 0;
}


File::~File()
{
        if(buffer)
                delete[] buffer;
}

char* File::ReadAll()
{
        in.open(fName, ios::in | ios::binary);

        in.seekg(0, ios::end);
        length = in.tellg();
        in.seekg(0, ios::beg);

        buffer = new char[length];

        in.read(buffer, length);
        in.close();

        return buffer;
}


