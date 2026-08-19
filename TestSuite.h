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

   // no copies
   TestSuite(const TestSuite& copy);
   TestSuite& operator=(const TestSuite& copy);


protected:
   virtual void setup();
   virtual void tearDown();

   virtual void suiteSetup();
   virtual void suiteTearDown();
   virtual void runTests() = 0;


public:
   TestSuite(const std::string& suiteName);

   virtual ~TestSuite();


   // file management
   std::string getTempFile() const;
   bool deleteFile(const std::string& filePath);

   void run();

   const std::string& getName() const;
   int getNumTestsExecuted() const;
   int getNumFailures() const;

   void require(bool expression, std::string testDesc="");
   void requireFalse(bool expression, std::string testDesc="");

   void requireStringEquals(const std::string& expected,
                            const std::string& actual,
                            std::string testDesc="");
   void requireNonEmptyString(const std::string& actual,
                              std::string testDesc="");
   void requireException(const char* exceptionType,
                         Runnable* run,
                         std::string testDesc="");

   void startingTestCase(const TestCase& testCase);
   void endingTestCase(const TestCase& testCase);

   void failTest(const std::string& failureMessage);

};

}

#endif

