// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTSUITE_H
#define POIVRE_TESTSUITE_H

#include <string>

#include "TestMacros.h"
#include "Runnable.h"


namespace poivre
{
class TestCase;

class TestSuite
{
private:
   std::string m_suiteName;
   int m_numTestsExecuted;
   int m_numFailures;

   
protected:
   virtual void setup();
   virtual void tearDown();
   
   virtual void suiteSetup();
   virtual void suiteTearDown();
   virtual void runTests() = 0;

   
public:
   TestSuite(const std::string& suiteName);
   TestSuite(const TestSuite& copy);
   
   virtual ~TestSuite();
   
   TestSuite& operator=(const TestSuite& copy);

   
   // file management
   std::string getTempFile() const;
   bool deleteFile(const std::string& filePath);
   
   void run();
   
   const std::string& getName() const;
   
   void require(bool expression, const std::string& testDesc);
   void requireFalse(bool expression, const std::string& testDesc);

   void requireStringEquals(const std::string& expected,
                            const std::string& actual,
                            const std::string& testDesc);
   void requireNonEmptyString(const std::string& actual,
                              const std::string& testDesc);
   void requireException(const char* exceptionType,
                         Runnable* run,
                         const std::string& testDesc); 
   
   void startingTestCase(const TestCase& testCase);
   void endingTestCase(const TestCase& testCase);
   
};

}

#endif

