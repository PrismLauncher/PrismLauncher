#include "TestParseScript.h"

#include "TestUtils.h"
#include "UpdateScript.h"

#include <iostream>
#include <algorithm>

void TestParseScript::testParse()
{
	UpdateScript script;

	script.parse("mmc_updater/src/tests/file_list.xml");

	TEST_COMPARE(script.isValid(),true);
}

int main(int,char**)
{
	TestList<TestParseScript> tests;
	tests.addTest(&TestParseScript::testParse);
	return TestUtils::runTest(tests);
}

