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
private:
   TestSuite& m_testSuite;
   std::string m_testCaseName;
   
   TestCase(const TestCase& copy);
   TestCase& operator=(const TestCase& copy);

   
public:
   TestCase(TestSuite& testSuite, const std::string& testCaseName);
   virtual ~TestCase();
   
   const std::string& getName() const;
   
};

}

#endif
