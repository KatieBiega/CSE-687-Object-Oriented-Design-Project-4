#include "File Management.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::getline;
using std::vector;
using std::stringstream;
using std::ifstream;
using std::ofstream;
using std::istreambuf_iterator;
using std::cerr;

FileManagement::FileManagement(const std::string& inputDir, const std::string& outputDir, const std::string& tempDir)
    : inputDirectory(inputDir), outputDirectory(outputDir), tempDirectory(tempDir) {}

// function to read files from provided directory
string FileManagement::ReadAllFiles() {

    string content; // content of directory 
    string temp; //temp file string
    ifstream fileStream; // opens and reads data from file
    string inputLine; // take input
    string fullContent;


    // functtion to iterate directory files content into a string
    for (auto& file : std::filesystem::directory_iterator(outputDirectory)) {

        fileStream.open(file.path().string());

        while (getline(fileStream, inputLine))
        {
            content = content + "\n" + inputLine;
        }

        fullContent = fullContent + content;

    }
    return fullContent; // returns directory and saves to string
    fileStream.close();
}

string FileManagement::ReadSingleFile(string filePath) {

    string content; // content of directory 
    string temp; //temp file string
    ifstream fileStream; // opens and reads data from file
    string inputLine; // take input


    // open a single file

        fileStream.open(filePath);

        while (getline(fileStream, inputLine))
        {
            content = content + "\n" + inputLine;
        }

    return content; // returns directory and saves to string
    fileStream.close();
}

//reads the directory from the temp file 
string FileManagement::ReadFromTempFile(const std::string& fileName) {
    ifstream file(tempDirectory + "/" + fileName);
    if (file.is_open()) {
        std::string content((istreambuf_iterator<char>(file)), (istreambuf_iterator<char>()));
        file.close();
        return content;
    }
    else {
        // Handle file not found or other errors
        cerr << "Error: Unable to read temp file " << fileName << endl;
        return "";
    }
}
// takes directory and creates a temp file 
void FileManagement::WriteToTempFile(const string& fileName, const string& data) {
    ofstream file(fileName);
    if (file.is_open()) {
        file << data;
        file.close();
    }
    else {

        cerr << "Error: Unable to write to temporary file " << fileName << endl; // error if unable to write to temp file
    }
}

//writes data to file in an output directory
void FileManagement::WriteToOutputFile(const string& fileName, const string& data) {
    ofstream file(fileName);
    if (file.is_open()) {
        file << data;
        file.close();
    }
    else {

        cerr << "Error: Unable to write to file " << fileName << endl; // error display if unable to write to temp file 
    }
}

int FileManagement::getCount() {

    int fileCount = 0;

    for (auto& p : std::filesystem::directory_iterator(inputDirectory))
    {
        ++fileCount;
    }

    cout << "# of files in " << inputDirectory << ": " << fileCount << '\n';

    return fileCount;
}

vector<string> FileManagement::getFilenames() {

    vector<string> fileNames;
    string filename;
    
    for (auto& f : std::filesystem::directory_iterator(inputDirectory))
    {
        filename = f.path().filename().string();
        //const auto
        fileNames.push_back(filename);
    }

    return fileNames;
}

void FileManagement::deleteAllFilesInDirectory() {
    for (auto& file : std::filesystem::directory_iterator(outputDirectory))
        std::filesystem::remove_all(file.path());
}

