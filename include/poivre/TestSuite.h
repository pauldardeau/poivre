// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTSUITE_H
#define POIVRE_TESTSUITE_H

#include <atomic>
#include <string>

#include "TestMacros.h"
#include "Runnable.h"


namespace poivre
{
class TestCase;

class TestSuite
{
public:
   TestSuite(const std::string& suiteName);

   // no copies
   TestSuite(const TestSuite& copy) = delete;
   TestSuite& operator=(const TestSuite& copy) = delete;

   virtual ~TestSuite();


   // file management
   std::string getTempFile() const;
   bool deleteFile(const std::string& filePath);

   /**
    * Runs suiteSetup(), runTests() and suiteTearDown(). Exceptions thrown
    * by any of the three (or by setup()/tearDown() of a test case) are
    * caught and recorded as failures; run() itself does not throw them.
    * If suiteSetup() throws, runTests() is skipped; suiteTearDown() always
    * runs. If a test method listed in runTests() throws, the remaining
    * test methods of that runTests() call are skipped (runTests() is user
    * code and cannot be resumed), but the process and later suites go on.
    */
   void run();

   const std::string& getName() const;

   /**
    * @return number of assertions evaluated so far (require*, failTest,
    *         and recorded unexpected exceptions). The name is historical.
    */
   int getNumTestsExecuted() const;

   /**
    * @return number of failed assertions (including unexpected exceptions)
    */
   int getNumFailures() const;

   /**
    * @return number of test cases (TEST_CASE) started so far
    */
   int getNumTestCasesRun() const;

   /**
    * Process exit code for this suite: 0 if at least one assertion was
    * evaluated and none failed, 1 otherwise (a failure, or a suite that
    * evaluated no assertions at all). Use this instead of returning
    * getNumFailures() from main(): exit statuses are truncated (to 8 bits
    * on POSIX), so 256 failures would otherwise look like success.
    */
   int exitCode() const;

   void require(bool expression, const std::string& testDesc="");
   void requireFalse(bool expression, const std::string& testDesc="");

   void requireStringEquals(const std::string& expected,
                            const std::string& actual,
                            const std::string& testDesc="");
   void requireNonEmptyString(const std::string& actual,
                              const std::string& testDesc="");
   void requireException(const char* exceptionType,
                         Runnable* run,
                         const std::string& testDesc="");

   void startingTestCase(const TestCase& testCase);
   void endingTestCase(const TestCase& testCase);

   void failTest(const std::string& failureMessage);

   /**
    * Records one failed assertion for an exception that escaped test code.
    * Used by the framework (run(), endingTestCase(), POIVRE_TEST_CASE).
    * @param context where the exception came from, for the failure line
    * @param what the exception's what() text, or nullptr if the exception
    *        was not derived from std::exception
    */
   void recordUnexpectedException(const std::string& context,
                                  const char* what);


protected:
   virtual void setup();
   virtual void tearDown();

   virtual void suiteSetup();
   virtual void suiteTearDown();
   virtual void runTests() = 0;


private:
   std::string m_suiteName;
   std::atomic<int> m_numTestsExecuted;
   std::atomic<int> m_numFailures;
   std::atomic<int> m_numTestCasesRun;
};

}

#endif

