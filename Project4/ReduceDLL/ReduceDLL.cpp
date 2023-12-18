/*
CSE 687 Object Oriented Design

Reduce Class: The reduce class will have a method reduce() that will take a string
with one key and an iterator of integers. The reduce function should sum all the
values in the iterator and then call an export function (different from the Map class).
The export function will take two parameters, the key and resulting reduced value,
and write the result out to the output directory. Upon success an additional empty file
SUCCESS will be written out to the output directory. The reduce method itself should
not directly deal with any File IO.
*/

#include "pch.h" // use stdafx.h in Visual Studio 2017 and earlier
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "ReduceDLL.h"

using std::stringstream;
using std::vector;
using std::string;
using std::to_string;
using std::getline;
using std::cout;

void Reduce::import(string inputString) // breaks down text in string into multiple strings in the initial vector for use by reduce member functions
{
	stringstream stream(inputString); // intermediary placeholder between input line and vector 

	string inputLine = "";

	while (getline(stream, inputLine, '\n'))
	{
		//cout << "Adding importing strings to initial vector in Reduce class...\n";
		initialVector.push_back(inputLine);
	}

}

void Reduce::sort() {
	//cout << "Beginning sort in Reduce class...\n";
	std::sort(initialVector.begin(), initialVector.end()); // call sort function from standard C++ library to sort the strings in the vector alphabetically
	//cout << "Sort completed in Reduce class...\n";

}

void Reduce::aggregate() {
	std::unordered_map<std::string, int> wordCount;

	for (const std::string& currentLine : initialVector) {
		int begin = currentLine.find_first_of("\"");
		int end = currentLine.find_last_of("\"");
		std::string newWord = currentLine.substr(begin + 1, end - begin - 1);

		if (!newWord.empty()) {
			wordCount[newWord]++;
		}
	};

	for (const auto& pair : wordCount) {
		aggregatedVector.push_back("(\"" + pair.first + "\", " + std::to_string(pair.second) + ")");
	}
};

void Reduce::reduce() {
	for (const std::string& line : aggregatedVector) {
		int begin = line.find_first_of("\"");
		int end = line.find_last_of("\"");
		std::string currentWord = line.substr(begin + 1, end - begin - 1);

		begin = line.find_last_of(' ');
		int total = std::stoi(line.substr(begin + 1));

		reducedVector.push_back("(\"" + currentWord + "\", " + std::to_string(total) + ")");
	}
};
string Reduce::vector_export() {

	string outputString;
	int toggle = 0;

	for (string currentElement : reducedVector) // iterate through sorted vector to aggregate values into single strings to pass to the aggregatingVector
	{
		//cout << "Adding vector element to final Reduce output string..." + currentElement + "\n";

		if (toggle == 1) {
			outputString = outputString + "\n";
		}
		toggle = 1;
		outputString = outputString + currentElement;

		//cout << outputString + "\n";
	}

	return outputString;

}

Reduce* CreateReduce()
{
	return new Reduce();
}