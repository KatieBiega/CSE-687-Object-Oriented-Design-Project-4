// reduceProcess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "File Management.h"
#include "C:\Users\moimeme\Downloads\CSE-687-Object-Oriented-Design-Project-4-testing\CSE-687-Object-Oriented-Design-Project-4-testing\Project4\ReduceDLL\ReduceInterface.h"

using std::stringstream;
using std::vector;
using std::string;
using std::wstring;
using std::to_string;
using std::getline;
using std::cout;
using std::cin;
using std::endl;

typedef ReduceInterface* (*CREATE_REDUCER) ();

int main()
{
    string fileName = "";  // Temporary
    string fileString = "";  // Temporary
    string inputDirectory = "";  // Temporary
    string outputDirectory = "";  // Temporary
    string tempDirectory = "";  // Temporary

    string reduced_string;
    
    FileManagement FileManage(inputDirectory, outputDirectory, tempDirectory); //Create file management class based on the user inputs
    cout << "FileManagement Class initialized.\n";

    HMODULE reduceDLL = LoadLibraryA("ReduceDLL.dll"); // load dll for library functions
    if (reduceDLL == NULL) // exit main function if reduceDLL is not found
    {
        cout << "Failed to load reduceDLL." << endl;
        return 1;
    }
    
    CREATE_REDUCER reducerPtr = (CREATE_REDUCER)GetProcAddress(reduceDLL, "CreateReduce");  // create pointer to function to create new Reduce object
    ReduceInterface* pReducer = reducerPtr();

    //Read from intermediate file and pass data to Reduce class
    fileString = FileManage.ReadSingleFile(tempDirectory);     //Read single file into single string
    //cout << "Single file read.\n";

    pReducer->import(fileString);
    //cout << "String imported by reduce class function and placed in vector.\n";

    pReducer->sort();
    //cout << "Vector sorted.\n";

    pReducer->aggregate();
    //cout << "Vector aggregated.\n";

    pReducer->reduce();
    //cout << "Vector reduced.\n";

    reduced_string = pReducer->vector_export();
    //cout << "Vector exported to string.\n";

    //Sorted, aggregated, and reduced output string is written into final output file
    FileManage.WriteToOutputFile(outputDirectory, reduced_string);
    cout << "Reduced string written to output file.\n";
        
    system("pause");

    FreeLibrary(reduceDLL);
}
