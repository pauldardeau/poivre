// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <unistd.h>

#include "TestSuite.h"
#include "BasicException.h"
#include "AutoPointer.h"

using namespace poivre;

//******************************************************************************

TestSuite::TestSuite(const std::string& suiteName) :
   m_suiteName(suiteName),
   m_numTestsExecuted(0),
   m_numFailures(0) {
}

//******************************************************************************

TestSuite::~TestSuite() {
}

//******************************************************************************

const std::string& TestSuite::getName() const {
   return m_suiteName;
}

//******************************************************************************

int TestSuite::getNumTestsExecuted() const {
   return m_numTestsExecuted;
}

//******************************************************************************

int TestSuite::getNumFailures() const {
   return m_numFailures;
}

//******************************************************************************

void TestSuite::setup() {
}

//******************************************************************************

void TestSuite::tearDown() {
}

//******************************************************************************

void TestSuite::suiteSetup() {
}

//******************************************************************************

void TestSuite::suiteTearDown() {
}

//******************************************************************************

void TestSuite::run() {
   printf("+++++++++++++ running suite: %s +++++++++++\n", m_suiteName.c_str());
   // flushed for the same reason as startingTestCase() below - a crash
   // anywhere in this suite should still leave an accurate log of which
   // suite was running
   fflush(stdout);
   suiteSetup();
   runTests();
   suiteTearDown();
}

//******************************************************************************

std::string TestSuite::getTempFile() const {
   char fileTemplate[] = "/tmp/poivreXXXXXX";
   const int fd = ::mkstemp(fileTemplate);
   if (-1 != fd) {
      ::close(fd);
      return std::string(fileTemplate);
   } else {
      printf("error: mkstemp failed. errno=%d\n", errno);
      fflush(stdout);
      return std::string("");
   }
}

//******************************************************************************

bool TestSuite::deleteFile(const std::string& filePath) {
   const int rc = remove(filePath.c_str());
   return (0 == rc);
}

//******************************************************************************

void TestSuite::startingTestCase(const TestCase& testCase) {
   printf("--------------- running test: %s -----------\n", testCase.getName().c_str());
   // flushed so that a crash during this test case still leaves an accurate
   // record of which test was running - stdout is fully buffered (not
   // line-buffered) when captured by CI, so without this the line can sit
   // unflushed and appear out of order (or not at all) in the log
   fflush(stdout);
   setup();
}

//******************************************************************************

void TestSuite::endingTestCase(const TestCase&) {
   tearDown();
   //printf("ending test case %s\n", testCase.getName().c_str());
}

//******************************************************************************

void TestSuite::require(bool expression, const std::string& testDesc) {
   if (!expression) {
      ++m_numFailures;
      printf("*** failure: expected true, got false (%s)\n",
             testDesc.c_str());
      // flushed so a failure is on record even if a subsequent statement
      // in the same test case crashes
      fflush(stdout);
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireFalse(bool expression, const std::string& testDesc) {
   if (expression) {
      ++m_numFailures;
      printf("*** failure: expected false, got true (%s)\n",
             testDesc.c_str());
      fflush(stdout);
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireStringEquals(const std::string& expected,
                                    const std::string& actual,
                                    const std::string& testDesc) {
   if (expected != actual) {
      ++m_numFailures;
      printf("*** failure: expected '%s', actual='%s' (%s)\n",
             expected.c_str(),
             actual.c_str(),
             testDesc.c_str());
      fflush(stdout);
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireNonEmptyString(const std::string& actual,
                                      const std::string& testDesc) {
   if (actual.empty()) {
      ++m_numFailures;
      printf("*** failure: expected non-empty string (%s)\n",
             testDesc.c_str());
      fflush(stdout);
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireException(const char* exceptionType,
                                 Runnable* runnable,
                                 const std::string& testDesc) {
   ++m_numTestsExecuted;
   AutoPointer<Runnable> uniqueRunnable(runnable);
   try {
      uniqueRunnable->run();
      ++m_numFailures;
      std::string msg(testDesc);
      msg += " - no exception thrown (exp=";
      msg += exceptionType;
      msg += ")";
      printf("*** failure: %s\n", msg.c_str());
      fflush(stdout);
      return;
   } catch (BasicException& be) {
      if (strcmp(exceptionType, be.getType())) {
         ++m_numFailures;
         std::string msg(testDesc);
         msg += " - Wrong exception type (";
         msg += "exp=";
         msg += exceptionType;
         msg += ",act=";
         msg += be.getType();
         msg += ")";
         printf("*** failure: %s\n", msg.c_str());
         fflush(stdout);
      } else {
         // we got what's expected
      }
   } catch (std::exception& e) {
      //printf("std::exception caught: %s\n", e.what());

      ++m_numFailures;
      std::string msg(testDesc);
      msg += " - Wrong exception type (";
      msg += "exp=";
      msg += exceptionType;
      msg += ",act=std::exception)";
      printf("*** failure: %s\n", msg.c_str());
      fflush(stdout);
   } catch (...) {
      //printf("unknown exception caught\n");

      ++m_numFailures;
      std::string msg(testDesc);
      msg += " - Wrong exception type (";
      msg += "exp=";
      msg += exceptionType;
      msg += ",act=unknown)";
      printf("*** failure: %s\n", msg.c_str());
      fflush(stdout);
   }
}

//******************************************************************************

void TestSuite::failTest(const std::string& failureMessage) {
   ++m_numFailures;
   ++m_numTestsExecuted;
   printf("*** failure: %s\n", failureMessage.c_str());
   fflush(stdout);
}

//******************************************************************************

