#!/usr/bin/env python3

import os
import tarfile
import filecmp



# use make file to compile to library

# Create new file and put files in them
os.system("mkdir /tmp/Test")
os.system("cp ex2.tar /tmp/Test")
os.system("cp test.cpp /tmp/Test")
os.system("cp results.txt /tmp/Test")


curFolder = os.path.dirname(os.path.realpath(__file__))
os.chdir("/tmp/Test")
files = tarfile.open("ex2.tar")
files.extractall(path=".", members=None)
os.system("make")
os.system("g++ -c -g test.cpp")

# incase library was built in c++
if not os.system("g++ test.o -L. libuthreads.a -o runtest"):
	os.system("g++ -D ITSC test.o -L. libuthreads.a -o runtest")

#library is now linked now just run tests..
os.system("./runtest > output.txt")
os.system("diff results.txt output.txt > diff.txt")
# Check the two outputs
if os.system("diff results.txt output.txt > diff.txt"):
	print("\n--------Failed Sanity Check-------- \nCheck the output.txt and diff.txt files")
	os.system("cp diff.txt " + curFolder)
	os.system("cp output.txt " + curFolder)
else:
	print("\n--------Passed Sanity Check--------")

files.close()
os.chdir(curFolder)
os.system("rm -rf /tmp/Test")
print("End Script")





