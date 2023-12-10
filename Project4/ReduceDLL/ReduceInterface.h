#pragma once
#include<iostream>
#include<string>
#include<vector>

using std::string;
using std::vector;

class ReduceInterface
{
public:
	virtual ~ReduceInterface() { ; }
	virtual void import(string inputString) = 0; // import the string and place it into the initialVector
	virtual void sort() = 0; // sort the strings into alphabeetical order (ignore this, unless std::map's automatic sorting is insufficient)
	virtual void aggregate() = 0; // aggregate each key/value pair with the same key into single keys with many attached values
	virtual void reduce() = 0; // reduce the aggregated values into a single total value for the associated key
	virtual string vector_export() = 0; // export the final educed vector as a single string
};