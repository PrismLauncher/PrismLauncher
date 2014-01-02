#include "TestFileUtils.h"

#include "FileUtils.h"
#include "TestUtils.h"

void TestFileUtils::testDirName()
{
	std::string dirName;
	std::string fileName;
	
#ifdef PLATFORM_WINDOWS
	// absolute paths
	dirName = FileUtils::dirname("E:/Some Dir/App.exe");
	TEST_COMPARE(dirName,"E:/Some Dir");
	fileName = FileUtils::fileName("E:/Some Dir/App.exe");
	TEST_COMPARE(fileName,"App.exe");

	dirName = FileUtils::dirname("C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa/MultiMC.exe");
	TEST_COMPARE(dirName,"C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa");
	fileName = FileUtils::fileName("C:/Users/kitteh/AppData/Local/Temp/MultiMC5-yidaaa/MultiMC.exe");
	TEST_COMPARE(fileName,"MultiMC.exe");

#else
	// absolute paths
	dirName = FileUtils::dirname("/home/tester/foo bar/baz");
	TEST_COMPARE(dirName,"/home/tester/foo bar");
	fileName = FileUtils::fileName("/home/tester/foo bar/baz");
	TEST_COMPARE(fileName,"baz");
#endif
	// current directory
	dirName = FileUtils::dirname("App.exe");
	TEST_COMPARE(dirName,".");
	fileName = FileUtils::fileName("App.exe");
	TEST_COMPARE(fileName,"App.exe");

	// relative paths
	dirName = FileUtils::dirname("Foo/App.exe");
	TEST_COMPARE(dirName,"Foo");
	fileName = FileUtils::fileName("Foo/App.exe");
	TEST_COMPARE(fileName,"App.exe");
}

void TestFileUtils::testIsRelative()
{
#ifdef PLATFORM_WINDOWS
	TEST_COMPARE(FileUtils::isRelative("temp"),true);
	TEST_COMPARE(FileUtils::isRelative("D:/temp"),false);
	TEST_COMPARE(FileUtils::isRelative("d:/temp"),false);
#else
	TEST_COMPARE(FileUtils::isRelative("/tmp"),false);
	TEST_COMPARE(FileUtils::isRelative("tmp"),true);
#endif
}

void TestFileUtils::testSymlinkFileExists()
{
#ifdef PLATFORM_UNIX
	const char* linkName = "link-name";
	FileUtils::removeFile(linkName);
	FileUtils::createSymLink(linkName, "target-that-does-not-exist");
	TEST_COMPARE(FileUtils::fileExists(linkName), true);
#endif
}

void TestFileUtils::testStandardDirs()
{
	std::string tmpDir = FileUtils::tempPath();
	TEST_COMPARE(FileUtils::fileExists(tmpDir.data()), true);
}

int main(int,char**)
{
	TestList<TestFileUtils> tests;
	tests.addTest(&TestFileUtils::testDirName);
	tests.addTest(&TestFileUtils::testIsRelative);
	tests.addTest(&TestFileUtils::testSymlinkFileExists);
	tests.addTest(&TestFileUtils::testStandardDirs);
	return TestUtils::runTest(tests);
}
