// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <exception>
#include <unistd.h>

#include "TestSuite.h"
#include "BasicException.h"
#include "AutoPointer.h"

using namespace poivre;

namespace {

// Runs fn; if it throws, records the exception as a failure of the suite
// (instead of letting it escape) and returns false.
template <class Fn>
bool runGuarded(TestSuite& suite, const std::string& context, Fn fn) {
   try {
      fn();
      return true;
   } catch (const std::exception& e) {
      suite.recordUnexpectedException(context, e.what());
   } catch (...) {
      suite.recordUnexpectedException(context, nullptr);
   }

   return false;
}

}

//******************************************************************************

TestSuite::TestSuite(const std::string& suiteName) :
   m_suiteName(suiteName),
   m_numTestsExecuted(0),
   m_numFailures(0),
   m_numTestCasesRun(0) {
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

int TestSuite::getNumTestCasesRun() const {
   return m_numTestCasesRun;
}

//******************************************************************************

int TestSuite::exitCode() const {
   return (m_numFailures == 0 && m_numTestsExecuted > 0) ? 0 : 1;
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

   const bool setupOk =
      runGuarded(*this, "suiteSetup() of suite " + m_suiteName,
                 [this] { suiteSetup(); });

   // tests are skipped if suite setup failed; suiteTearDown() still runs
   // so that whatever suiteSetup() managed to acquire is released
   if (setupOk) {
      runGuarded(*this, "runTests() of suite " + m_suiteName,
                 [this] { runTests(); });
   }

   runGuarded(*this, "suiteTearDown() of suite " + m_suiteName,
              [this] { suiteTearDown(); });

   if (m_numTestsExecuted == 0) {
      printf("*** warning: suite %s evaluated no assertions "
             "(exitCode() reports failure)\n",
             m_suiteName.c_str());
      fflush(stdout);
   }
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
   ++m_numTestCasesRun;
   printf("--------------- running test: %s -----------\n", testCase.getName().c_str());
   // flushed so that a crash during this test case still leaves an accurate
   // record of which test was running - stdout is fully buffered (not
   // line-buffered) when captured by CI, so without this the line can sit
   // unflushed and appear out of order (or not at all) in the log
   fflush(stdout);
   // an exception from setup() propagates out of the TestCase constructor,
   // so the test body is not run (and ~TestCase does not run tearDown());
   // the caller of the test records it as a failure
   setup();
}

//******************************************************************************

void TestSuite::endingTestCase(const TestCase& testCase) {
   // called from ~TestCase, which must not throw (it would terminate the
   // process, and does so unconditionally while another exception is
   // already unwinding), so a failing tearDown() is recorded instead
   runGuarded(*this, "tearDown() of test case " + testCase.getName(),
              [this] { tearDown(); });
}

//******************************************************************************

void TestSuite::recordUnexpectedException(const std::string& context,
                                          const char* what) {
   ++m_numFailures;
   ++m_numTestsExecuted;
   if (what != nullptr) {
      printf("*** failure: unexpected exception in %s: %s\n",
             context.c_str(),
             what);
   } else {
      printf("*** failure: unexpected exception in %s: "
             "(not derived from std::exception)\n",
             context.c_str());
   }
   fflush(stdout);
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

