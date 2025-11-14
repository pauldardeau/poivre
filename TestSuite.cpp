// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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

TestSuite::TestSuite(const TestSuite& copy) :
   m_suiteName(copy.m_suiteName),
   m_numTestsExecuted(copy.m_numTestsExecuted),
   m_numFailures(copy.m_numFailures) {
}

//******************************************************************************

TestSuite::~TestSuite() {
}

//******************************************************************************

TestSuite& TestSuite::operator=(const TestSuite& copy) {
   if (this == &copy) {
      return *this;
   }

   m_suiteName = copy.m_suiteName;
   m_numTestsExecuted = copy.m_numTestsExecuted;
   m_numFailures = copy.m_numFailures;

   return *this;
}

//******************************************************************************

const std::string& TestSuite::getName() const {
   return m_suiteName;
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
   suiteSetup();
   runTests();
   suiteTearDown();
}

//******************************************************************************

std::string TestSuite::getTempFile() const {
   return "testsuite.txt";
   /*
   char fileTemplate[] = "/tmp/fileXXXXXX.test";
   const int fd = ::mkstemp(fileTemplate);
   if (-1 != fd) {
      return std::string(fileTemplate);
   } else {
      printf("error: mkstemp failed. errno=%d\n", errno);
      return std::string("");
   }
   */
}

//******************************************************************************

bool TestSuite::deleteFile(const std::string& filePath) {
   const int rc = remove(filePath.c_str());
   return (0 == rc);
}

//******************************************************************************

void TestSuite::startingTestCase(const TestCase& testCase) {
   printf("--------------- running test: %s -----------\n", testCase.getName().c_str());
   setup();
}

//******************************************************************************

void TestSuite::endingTestCase(const TestCase& testCase) {
   tearDown();
   //printf("ending test case %s\n", testCase.getName().c_str());
}

//******************************************************************************

void TestSuite::require(bool expression, std::string testDesc) {
   if (!expression) {
      ++m_numFailures;
      printf("*** failure: expected true, got false (%s)\n",
             testDesc.c_str());
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireFalse(bool expression, std::string testDesc) {
   if (expression) {
      ++m_numFailures;
      printf("*** failure: expected false, got true (%s)\n",
             testDesc.c_str());
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireStringEquals(const std::string& expected,
                                    const std::string& actual,
                                    std::string testDesc) {
   if (expected != actual) {
      ++m_numFailures;
      printf("*** failure: expected '%s', actual='%s' (%s)\n",
             expected.c_str(),
             actual.c_str(),
             testDesc.c_str());
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireNonEmptyString(const std::string& actual,
                                      std::string testDesc) {
   if (actual.empty()) {
      ++m_numFailures;
      printf("*** failure: expected non-empty string (%s)\n",
             testDesc.c_str());
   }

   ++m_numTestsExecuted;
}

//******************************************************************************

void TestSuite::requireException(const char* exceptionType,
                                 Runnable* runnable,
                                 std::string testDesc) {
   ++m_numTestsExecuted;
   AutoPointer<Runnable*> uniqueRunnable(runnable);
   try {
      uniqueRunnable->run();
      ++m_numFailures;
      throw BasicException("no exception thrown");
   } catch (BasicException& be) {
      if (strcmp(exceptionType, be.getType())) {
         ++m_numFailures;
         std::string exc_msg(testDesc);
         exc_msg += " - Wrong exception type (";
         exc_msg += "exp=";
         exc_msg += exceptionType;
         exc_msg += ",act=";
         exc_msg += be.getType();
         exc_msg += ")";
         throw BasicException(exc_msg);
      } else {
         // we got what's expected
      }
   } catch (std::exception& e) {
      printf("std::exception caught: %s\n", e.what());
   } catch (...) {
      printf("unknown exception caught\n");
   }
}

//******************************************************************************

void TestSuite::failTest(const std::string& failureMessage) {
   ++m_numFailures;
   ++m_numTestsExecuted;
   printf("*** failure: %s\n", failureMessage.c_str());
}

//******************************************************************************

