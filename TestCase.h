// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTCASE_H
#define POIVRE_TESTCASE_H

#include <string>

namespace poivre
{

class TestSuite;

class TestCase
{
public:
   TestCase(TestSuite& testSuite, const std::string& testCaseName);

   // no copies
   TestCase(const TestCase& copy) = delete;
   TestCase& operator=(const TestCase& copy) = delete;

   virtual ~TestCase();

   const std::string& getName() const;

private:
   TestSuite& m_testSuite;
   std::string m_testCaseName;
};

}

#endif
