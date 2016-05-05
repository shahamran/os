#include "MapReduceClient.h"
#include "MapReduceFramework.h"
#include <string>
#include <vector>
#include <iostream>
#include <dirent.h>

#define EXIT_SUCC 0
#define EXIT_FAIL 1

using std::string;

// Constants:
const int MIN_ARGS = 2;
const string USAGE_ERROR_MESSAGE = "Usage: <substring to search> "
	"<folders, separated by spaces>";

/**
 * 
 */
class DirNameKey : k1Base
{
public:
	DirNameKey(const string f)  : dirName(f) {}	

	virtual ~DirNameKey() {}

	virtual bool operator<(const DirNameKey &other) const
	{
		return dirName.compare(other.dirName);
	}
	
	const string getDirName() const
	{
		return dirName;
	}
private:
	const string dirName;
};

//intermediate key and value.
//the key, value for the Reduce function created by the Map function
class FileNameKey : public k2Base, public k3Base {
public:
	FileNameKey(const string f) : fileName(f) {}

	virtual ~FileNameKey(){}

	virtual bool operator<(const FileNameKey &other) const
	{
		return fileName.compare(other.fileName);
	}

	const string getFileName() const
	{
		return fileName;
	}

private:
	const string fileName;
};

class v2Base {
public:
	virtual ~v2Base(){}
};

//output key and value
//the key,value for the Reduce function created by the Map function

class v3Base {
public:
	virtual ~v3Base() {}
};

typedef std::list<v2Base *> V2_LIST;

/**
 * The map and reduce functions implementation
 */
class MapReduce : MapReduceBase {
public:
    virtual void Map(const k1Base *const key, const v1Base *const val) const
    {
	    // Downcast to k1*
	    const DirNameKey* const pdirkey = (const DirNameKey* const)key;
	    // Open the directory and iterate over files in it
	    string dirName = pdirkey->getDirName();
	    DIR *pdir = opendir(dirName.c_str());
	    if (pdir == NULL)
	    {
		    
	    }
	    struct dirent *pent = NULL;
	    while ( (pent = readdir(pdir)) )
	    {
		    Emit2(std::make_pair(new FileNameKey(pent->d_name), &1));
	    }

    }

    virtual void Reduce(const k2Base *const key, const V2_LIST &vals) const
    {
	    
    }
};


// Implementation
int main(int argc, char* argv[])
{
	std::vector<std::pair<DirNameKey, v1Base>> inList;
	if (argc >= MIN_ARGS)
	{
				
	}
	else
	{
		std::cerr << USAGE_ERROR_MESSAGE << std::endl;
		std::exit(EXIT_FAIL);
	}
	return EXIT_SUCC;
}
