#include "MapReduceClient.h"
#include "MapReduceFramework.h"

#include <cerrno>
#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <cstring>

#define ARG_STR_TO_FIND 1
#define ARG_DIR_START 2

#define EXIT_SUCC 0
#define EXIT_FAIL 1
#define THREAD_LEVEL 5
#define CURR_DIR "."
#define PARENT_DIR ".."

using std::string;

// Constants:
const int MIN_ARGS = 2;
const string USAGE_ERROR_MESSAGE = "Usage: <substring to search> "
	"<folders, separated by spaces>";


/**
 * Handle errors as required
 */
static void handleError(const string funcName)
{
	std::cerr << "MapReduceFramework Failure: " << funcName << " failed." 
	     << std::endl;
	exit(EXIT_FAIL);
}

/**
 * The directory name key (k1Derived)
 */
class DirNameKey : public k1Base
{
public:
	DirNameKey(const string d)  : dirName(d) {}	

	virtual ~DirNameKey() {}

	virtual bool operator<(const k1Base &other) const override
	{
		const DirNameKey &dirother = 
			dynamic_cast<const DirNameKey&> (other);
		return dirName < dirother.dirName;
	}
	
	const string getDirName() const
	{
		return dirName;
	}
private:
	const string dirName;
};

/**
 * The string to search in the file names
 */
class StringToFind : public v1Base
{
public:
	StringToFind(const string toFind) : str(toFind) {}

	string getStr() const
	{
		return str;
	}
private:
	const string str;
};


/**
 * The k2 class (k2Derived) - the filename (inside the input directories)
 */
class FileNameKey1 : public k2Base
{
public:
	FileNameKey1(const string f) : fileName(f) {}

	virtual ~FileNameKey1(){}

	virtual bool operator<(const k2Base &other) const override
	{
		const FileNameKey1 &fileOther = 
			dynamic_cast<const FileNameKey1&>(other);
		return fileName < fileOther.fileName;
	}

	const string getFileName() const
	{
		return fileName;
	}

private:
	const string fileName;
};


/**
 * The v2 class (v2Derived) - holds a pointer to the k2 for later
 * deletion 
 */
class v2Deriv : public v2Base
{
public:
	v2Deriv(FileNameKey1 *k2p) : k2Pointer(k2p) {}	

	virtual ~v2Deriv() 
	{ 
		delete k2Pointer;
		k2Pointer = nullptr;
	}
private:
	FileNameKey1 *k2Pointer;
};



/**
 * The k3 class (k3Derived) - the filename (inside the input directories)
 * This is the same as k2
 */
class FileNameKey2 : public k3Base
{
public:
	FileNameKey2(const string f) : fileName(f) {}

	virtual ~FileNameKey2(){}

	virtual bool operator<(const k3Base &other) const override
	{
		const FileNameKey2 &fileOther = 
			dynamic_cast<const FileNameKey2&>(other);
		return fileName < fileOther.fileName;
	}

	const string getFileName() const
	{
		return fileName;
	}

private:
	const string fileName;
};

/**
 * The v3 class (v3Derived) - Holds the number of times the k3 filename 
 * appeared (in total)
 */
class FileCountValue : public v3Base 
{
public:

	FileCountValue(int count) : myCount(count) {}

	virtual ~FileCountValue() {}

	int getCount() const
	{
		return myCount;
	}

private:
	const int myCount;
};

/**
 * The map and reduce functions implementation
 */
class MyMapReduce : public MapReduceBase {
public:

    virtual void Map(const k1Base *const key, const v1Base *const val) 
	    const override
    {
	    // Downcast to k1*
	    const DirNameKey* const pdirkey = (const DirNameKey* const)key;
	    const StringToFind* const ptoFind = 
		    (const StringToFind* const)val;

	    // Open the directory and iterate over files in it
	    string dirName = pdirkey->getDirName();
	    
	    DIR *pdir = opendir(dirName.c_str());
	    if (pdir == nullptr)
	    {
	    	    // Skip this file if it is not a directory.
		    if (errno == ENOTDIR || errno == ENOENT)
		    {
			    return;
		    }
		    handleError("opendir");
	    }
	    struct dirent *pent = nullptr;
	    string currFile;
	    while ( (pent = readdir(pdir)) )
	    {
		    currFile = pent->d_name;
		    // Ignore '.' and '..' folders
		    if (currFile.compare(CURR_DIR) == 0 ||
			currFile.compare(PARENT_DIR) == 0 ||
		    	// or if searched string was not found.
			currFile.find(ptoFind->getStr()) == string::npos)
		    {
			    continue;
		    }
		    FileNameKey1* pKey = 
			    new(std::nothrow) FileNameKey1(pent->d_name);
		    v2Deriv* pVal = new(std::nothrow) v2Deriv(pKey);
		    if (pKey == nullptr || pVal == nullptr)
		    {
			    handleError("new");
		    }
		    Emit2(pKey, pVal);
	    }
	    if (closedir(pdir) < 0)
	    {
		    handleError("closedir");
	    }

    }

    virtual void Reduce(const k2Base *const key, const V2_LIST &vals) 
	    const override
    {
	    const FileNameKey1* const pfileName = 
		    (const FileNameKey1* const)key;

	    FileNameKey2 *pKey =
		   new(std::nothrow) FileNameKey2(pfileName->getFileName());
	    FileCountValue *pVal = 
		    new(std::nothrow) FileCountValue(vals.size());
	    if (pKey == nullptr || pVal == nullptr)
	    {
		    handleError("new");
	    }
	    Emit3(pKey, pVal);
	    // Delete the k2,v2 pairs
	    v2Deriv* currV2Item;
	    for (auto it = vals.begin(); it != vals.end(); ++it)
	    {
		    currV2Item = dynamic_cast<v2Deriv*>(*it);
		    delete currV2Item;
		    currV2Item = nullptr;
	    }
    }
};


/**
 * Delete allocated <k1,v1> <k3,v3> pairs from input/output lists
 */
void cleanup(IN_ITEMS_LIST& inItems, OUT_ITEMS_LIST& outItems)
{
	for (auto it = inItems.begin(); it != inItems.end(); ++it)
	{
		delete it->first;
		delete it->second;
	}
	for (auto it = outItems.begin(); it != outItems.end(); ++it)
	{
		delete it->first;
		delete it->second;
	}
}

/**
 * Runs the framework and prints the results
 */
void runMapReduce(int numOfDirs, string strToFind, 
		std::vector<string>& dirName)
{	
	// Prepare the input
	IN_ITEMS_LIST inItems;
	MyMapReduce mapReduce;
	std::pair<k1Base*, v1Base*> currPair;
	for (int i = 0; i < numOfDirs; ++i)
	{
		currPair = std::make_pair(new DirNameKey(dirName[i]),
				new StringToFind(strToFind));
		inItems.push_back(currPair);
	}

	// Run MapReduce
	OUT_ITEMS_LIST outItems =
		runMapReduceFramework(mapReduce, inItems, THREAD_LEVEL);

	// Print the [sorted] output
	for (auto it = outItems.begin(); it != outItems.end(); ++it)
	{
		FileNameKey2* filename = 
			dynamic_cast<FileNameKey2*>(it->first);
		FileCountValue* count = 
			dynamic_cast<FileCountValue*>(it->second);

		// For each occurence of the filename, print it.
		for (int i = 0; i < count->getCount(); ++i)
		{
			std::cout << filename->getFileName() << std::endl;
		}
	}
	cleanup(inItems, outItems);
}

int main(int argc, char* argv[])
{
	if (argc >= MIN_ARGS)
	{
		string strToFind = argv[ARG_STR_TO_FIND];
		std::vector<string> dirNames(argv + ARG_DIR_START, argv + argc);
		// Minus 2 for this file name (Search.cpp) and the str to find
		runMapReduce(argc - 2, strToFind, dirNames);
	}
	else
	{
		std::cerr << USAGE_ERROR_MESSAGE << std::endl;
		std::exit(EXIT_FAIL);
	}
	return EXIT_SUCC;
}
