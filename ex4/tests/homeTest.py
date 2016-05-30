import unittest
import argparse
import tempfile
import os
import shutil
import posix
import random


class TestFuse(unittest.TestCase):
    def setUp(self):
        os.system("head -c 10000 /dev/urandom > src/file1")
        os.system("head -c 10000 /dev/urandom > src/file2")
        os.system("%s %s/src %s/mount 100 0.30 0.30"%(TestFuse.fuserPath, os.getcwd(), os.getcwd())) 

    def tearDown(self):
        os.system("fusermount -u %s/mount"%(os.getcwd()))
        os.system("rm -rf src/*")

    @classmethod
    def setUpClass(self):
        self.baseFolderPath = tempfile.mkdtemp(dir="/tmp")
        self.cwd = os.getcwd()
        os.chdir(self.baseFolderPath)
        os.mkdir("src")
        os.mkdir("mount")

    @classmethod
    def tearDownClass(self):
        os.system("fusermount -u %s/mount 2> /dev/null"%(os.getcwd()))
        os.system("fusermount -u %s/mount2 2> /dev/null"%(os.getcwd()))
        os.chdir(self.cwd)
        shutil.rmtree(self.baseFolderPath)

    def test_randomAccessToFile(self):
        position = 0
        size = 0
        srcFileFd = posix.open("src/file1", posix.O_RDONLY)
        mountFileFd = posix.open("mount/file1", posix.O_RDONLY)
        for i in range(10000):
            position = random.randint(0, 10000)
            size = random.randint(1, 10000)
            posix.lseek(srcFileFd,position, 0)
            posix.lseek(mountFileFd, position, 0)
            if (posix.read(srcFileFd,size) !=  posix.read(mountFileFd, size)):
                posix.close(srcFileFd)
                posix.close(mountFileFd)
                self.assertTrue(False)
        posix.close(srcFileFd)
        posix.close(mountFileFd)

    def test_relativePath(self):
        os.mkdir("mount2")
        os.chdir("src")
        os.system("%s . ../mount2 100 0.30 0.30"%(TestFuse.fuserPath))
        os.chdir("..")
        isFileExists = os.path.isfile("%s/mount2/file1"%(self.baseFolderPath))
        os.system("fusermount -u mount2")
        self.assertTrue(isFileExists)

    def test_rename_check_nameing(self):
        os.rename("mount/file2", "mount/file3")

        self.assertTrue(os.path.isfile("mount/file3"))
        self.assertTrue(os.path.isfile("src/file3"))
        self.assertFalse(os.path.isfile("mount/file2"))
        self.assertFalse(os.path.isfile("src/file2"))
    
    def test_rename_correct_read(self):
        firstFileFd = posix.open("mount/file1", posix.O_RDONLY)
        secondFileFd = posix.open("mount/file2", posix.O_RDONLY)
        for i in range(1000):
            position = random.randint(0, 10000)
            size = random.randint(1, 10000)
            posix.lseek(firstFileFd,position, 0)
            posix.lseek(secondFileFd, position, 0)
            posix.read(firstFileFd, size)
            posix.read(secondFileFd, size)
        posix.rename("mount/file2", "mount/file3")
        posix.rename("mount/file1","mount/file2")
        posix.close(firstFileFd)
        posix.close(secondFileFd)
        self.assertTrue(open("mount/file2").read()== open("src/file2").read())

    def test_rename_at_close_correct_read(self):
        firstFileFd = posix.open("mount/file1", posix.O_RDONLY)
        secondFileFd = posix.open("mount/file2", posix.O_RDONLY)
        for i in range(1000):
            position = random.randint(0, 10000)
            size = random.randint(1, 10000)
            posix.lseek(firstFileFd,position, 0)
            posix.lseek(secondFileFd, position, 0)
            posix.read(firstFileFd, size)
            posix.read(secondFileFd, size)
        posix.close(firstFileFd)
        posix.close(secondFileFd)
        posix.rename("mount/file2", "mount/file3")
        posix.rename("mount/file1","mount/file2")
        self.assertTrue(open("mount/file2").read()== open("src/file2").read())

    def test_rename_folder_check_nameing(self):
        os.mkdir("src/folder1")
        os.system("cp src/file2 src/folder1/file2") 
        
        posix.rename("mount/folder1", "mount/folder2")

        self.assertTrue(os.path.isfile("mount/folder2/file2"))
        self.assertTrue(os.path.isfile("src/folder2/file2"))
        self.assertFalse(os.path.isfile("mount/folder1/file2"))
        self.assertFalse(os.path.isfile("src/folder1/file2"))
    
    def test_rename_folder_correct_read(self):
        os.mkdir("src/folder1")
        os.mkdir("src/folder2")
        os.system("cp src/file1 src/folder1/file") 
        os.system("cp src/file2 src/folder2/file") 


        firstFileFd = posix.open("mount/folder1/file", posix.O_RDONLY)
        secondFileFd = posix.open("mount/folder2/file", posix.O_RDONLY)
        for i in range(1000):
            position = random.randint(0, 10000)
            size = random.randint(1, 10000)
            posix.lseek(firstFileFd,position, 0)
            posix.lseek(secondFileFd, position, 0)
            posix.read(firstFileFd, size)
            posix.read(secondFileFd, size)
        posix.rename("mount/folder2", "mount/folder3")
        posix.rename("mount/folder1","mount/folder2")
        posix.close(firstFileFd)
        posix.close(secondFileFd)
        self.assertTrue(open("mount/folder2/file").read()== open("src/folder2/file").read())

  
def getFuserPath():
    parser = argparse.ArgumentParser()
    parser.add_argument("path", type=str)
    return parser.parse_args().path
   

if __name__ == '__main__':
    TestFuse.fuserPath = os.path.abspath(getFuserPath())
    suite = unittest.TestLoader().loadTestsFromTestCase(TestFuse)
    unittest.TextTestRunner(verbosity=2).run(suite)
