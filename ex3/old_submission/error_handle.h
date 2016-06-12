#ifndef _ERROR_HANDLE_H
#define _ERROR_HANDLE_H

#include <iostream>
#include <string>

#define EXIT_ERROR 1

using std::string;
using std::cerr;
using std::endl;

/**
 * Handles an error with a function named <funcName> as specified in the
 * exercise instructions.
 */
void handleError(const string funcName)
{
	cerr << "MapReduceFramework Failure: " << funcName << " failed." 
	     << endl;
	exit(EXIT_ERROR);
}

#endif
