#ifndef FILE_MANAGEMENT
#define FILE_MANAGEMENT

#include <string>
#include <vector>

using std::string;
using std::vector;

class FileManagement {
public:
    FileManagement(const string& inputDir, const string& outputDir, const string& tempDir);

    string ReadFromTempFile(const string& fileName);
    string ReadAllFiles();
    string ReadSingleFile(string path);
    void WriteToTempFile(const string& fileName, const string& data);
    void WriteToOutputFile(const string& fileName, const string& data);
    void deleteAllFilesInDirectory();
    int getCount();
    vector<string> getFilenames();


    template<typename T>
    void WriteToTempOrOutputFile(const string& fileName, const string& data);

private:
    string inputDirectory;
    string outputDirectory;
    string tempDirectory;
};

template<typename T>
void FileManagement::WriteToTempOrOutputFile(const string& fileName, const string& data) {
    T fileStream((tempDirectory + fileName).c_str());
    if (fileStream.is_open()) {
        fileStream << data;
        fileStream.close();
    }
}


#endif

