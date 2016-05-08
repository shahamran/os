#include "MapReduceClient.h"
#include "MapReduceFramework.h"

#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>
#include <cstring>

#define EXIT_SUCC 0
#define EXIT_FAIL 1
#define THREAD_LEVEL 5
#define CURR_DIR "."
#define PARENT_DIR ".."

using std::string;

// Constants:
const int MIN_ARGS = 3;
const string USAGE_ERROR_MESSAGE = "Usage: <substring to search> "
	"<folders, separated by spaces>";

void handleError(const string funcName)
{
	std::cerr << "runMapReduceFramework Failure: " << funcName
		  << " failed." << std::endl;
	exit(EXIT_FAIL);
}

/**
 * 
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
		return dirName.compare(dirother.dirName);
	}
	
	const string getDirName() const
	{
		return dirName;
	}
private:
	const string dirName;
};

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

//intermediate key and value.
//the key, value for the Reduce function created by the Map function
class FileNameKey1 : public k2Base
{
public:
	FileNameKey1(const string f) : fileName(f) {}

	virtual ~FileNameKey1(){}

	virtual bool operator<(const k2Base &other) const override
	{
		const FileNameKey1 &fileOther = 
			dynamic_cast<const FileNameKey1&>(other);
		return fileName.compare(fileOther.fileName);
	}

	const string getFileName() const
	{
		return fileName;
	}

private:
	const string fileName;
};

class v2Deriv : public v2Base
{
public:
	FileNameKey1 *k2Pointer;
	v2Deriv(FileNameKey1 *k2p) : k2Pointer(k2p) {}	
};

//output key and value
//the key,value for the Reduce function created by the Map function
class FileNameKey2 : public k3Base
{
public:
	FileNameKey2(const string f) : fileName(f) {}

	virtual ~FileNameKey2(){}

	virtual bool operator<(const k3Base &other) const override
	{
		const FileNameKey2 &fileOther = 
			dynamic_cast<const FileNameKey2&>(other);
		return fileName.compare(fileOther.fileName);
	}

	const string getFileName() const
	{
		return fileName;
	}

private:
	const string fileName;
};

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
		    handleError("opendir");
	    }
	    struct dirent *pent = nullptr;
	    string currFile;
	    while ( (pent = readdir(pdir)) )
	    {
		    currFile = pent->d_name;
		    // Ignore '.' and '..' folders
		    if (currFile.compare(CURR_DIR) == 0 ||
			currFile.compare(PARENT_DIR) == 0)
		    {
			    continue;
		    }
		    // If searched string was not found, continue.
		    if (currFile.find(ptoFind->getStr()) == string::npos)
		    {
			    continue;
		    }
		    FileNameKey1* pKey = 
			    new(std::nothrow) FileNameKey1(pent->d_name);
		    if (pKey == nullptr)
		    {
			    handleError("new");
		    }
		    v2Deriv* pVal = new(std::nothrow) v2Deriv(pKey);
		    if (pVal == nullptr)
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
	    v2Deriv* currItem;
	    for (auto it = vals.begin(); it != vals.end(); ++it)
	    {
		    currItem = dynamic_cast<v2Deriv*>(*it);
		    delete currItem->k2Pointer;
		    delete currItem;
	    }
    }
};

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
	OUT_ITEMS_LIST outItems;
	// Run MapReduce
	outItems = runMapReduceFramework(mapReduce, inItems, THREAD_LEVEL);
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
		string strToFind = argv[1];
		std::vector<string> dirNames(argv + 2, argv + argc);
		runMapReduce(argc - 2, strToFind, dirNames);
	}
	else
	{
		std::cerr << USAGE_ERROR_MESSAGE << std::endl;
		std::exit(EXIT_FAIL);
	}
	return EXIT_SUCC;
}
