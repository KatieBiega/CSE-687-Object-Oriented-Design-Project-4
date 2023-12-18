/*
* CSE687 Project 1
* Map Class Header file 
* Defines public methods map() and mapped_export()
*/

#pragma once

#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "MapInterface.h"

#ifdef MAP_EXPORTS
#define MAP_API __declspec(dllexport)
#else
#define MAP_API __declspec(dllimport)
#endif

class Map: public MapInterface {
public:
	void map(std::string& line); // Method takes in a string of all input files and creates a vector of words
	std::string vector_export(); // Method uses the locally defined vector of parsed words to create (key, value) pairs and stores them in a mapped string

private:
	string line; // String to store the current line
	string content; // String to export mapped (key, value) pairs
	vector<std::string> words; // Vector of tokenized words from the input line

};

extern "C"
{
	MAP_API Map* _cdecl CreateMap();
};