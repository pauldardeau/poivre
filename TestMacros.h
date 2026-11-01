// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTMACROS_H
#define POIVRE_TESTMACROS_H

#include <exception>

#include "TestCase.h"
#include "TestRegistry.h"


#define TEST_CASE(testCaseName) \
poivre::TestCase macroTestCase(*this,testCaseName);


// POIVRE_TEST_CASE(SuiteClass, testName) { ... } declares a self-registering
// test case, in the style of Catch2's TEST_CASE / GoogleTest's TEST_F: it
// does not need to be listed in SuiteClass::runTests(), and cannot be
// forgotten there. It is discovered automatically and run by
// poivre::TestRegistry::instance().runAll().
//
// Unlike the classic TEST_CASE("name") macro (used inside a hand-written
// method of a shared, single suite instance), each POIVRE_TEST_CASE gets
// its own freshly constructed SuiteClass instance - so state doesn't
// leak between test cases, and none is shared across them via SuiteClass
// member variables. setup()/tearDown() still run around each one, same
// as today. SuiteClass must be default-constructible (true of every
// existing TestSuite subclass in this codebase).
//
// An exception escaping the test body, setup() or tearDown() (std::exception
// or anything else) is caught and recorded as a failure of that test case;
// the remaining test cases still run.
//
// Usage:
//   POIVRE_TEST_CASE(TestThread, testHasAttribute) {
//      PthreadsThread thread;
//      requireFalse(thread.hasAttribute("missing"), "...");
//      thread.setAttribute("present", "x");
//      require(thread.hasAttribute("present"), "...");
//   }
//
// The body runs as a method of a private subclass of SuiteClass, so it
// can call require()/requireFalse()/etc. (and any of SuiteClass's own
// protected/public helper methods) exactly like a normal TestSuite test
// method - no explicit fixture reference needed.
#define POIVRE_TEST_CASE(SuiteClass, testName) \
namespace { \
class SuiteClass##_##testName##_TestCase : public SuiteClass { \
public: \
   void poivreTestBody(); \
   void poivreInvoke() { \
      try { \
         poivre::TestCase testCase(*this, #testName); \
         poivreTestBody(); \
      } catch (const std::exception& poivreEx) { \
         recordUnexpectedException(#SuiteClass "." #testName, poivreEx.what()); \
      } catch (...) { \
         recordUnexpectedException(#SuiteClass "." #testName, nullptr); \
      } \
   } \
protected: \
   void runTests() override {} \
}; \
void poivreRun_##SuiteClass##_##testName() { \
   SuiteClass##_##testName##_TestCase fixture; \
   fixture.poivreInvoke(); \
   poivre::TestRegistry::instance().recordResult( \
      fixture.getNumTestsExecuted(), fixture.getNumFailures()); \
} \
poivre::AutoRegisterTestCase poivreAutoReg_##SuiteClass##_##testName( \
   #SuiteClass, #testName, &poivreRun_##SuiteClass##_##testName); \
} \
void SuiteClass##_##testName##_TestCase::poivreTestBody()


#endif

