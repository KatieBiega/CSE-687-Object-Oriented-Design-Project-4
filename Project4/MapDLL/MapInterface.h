#pragma once
#include<iostream>
#include<string>
#include<vector>

using std::string;
using std::vector;

class MapInterface
{
public:
	virtual ~MapInterface() { ; }
	virtual void map(std::string& line) = 0; // Method takes in a string of all input files and creates a vector of words
	virtual std::string vector_export() = 0; // Method uses the locally defined vector of parsed words to create (key, value) pairs and stores them in a mapped string
};