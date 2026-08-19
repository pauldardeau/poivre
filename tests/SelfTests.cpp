// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include "AutoPointer.h"
#include "BasicException.h"
#include "Runnable.h"
#include "TestCase.h"
#include "TestMacros.h"
#include "TestSuite.h"

using namespace poivre;

namespace {

// ---- helper for AutoPointer RAII test ----

class DestructTracker {
public:
   explicit DestructTracker(bool* destroyedFlag) : m_destroyedFlag(destroyedFlag) {
      *m_destroyedFlag = false;
   }
   ~DestructTracker() {
      *m_destroyedFlag = true;
   }
private:
   bool* m_destroyedFlag;
};

// ---- helpers for requireException coverage ----

class ThrowsExpectedType : public Runnable {
public:
   void run() override {
      throw BasicException("expected");
   }
};

class OtherException : public BasicException {
public:
   OtherException() : BasicException("other") {}
   const char* getType() const override {
      return "OtherException";
   }
};

class ThrowsWrongType : public Runnable {
public:
   void run() override {
      throw OtherException();
   }
};

class ThrowsStdException : public Runnable {
public:
   void run() override {
      throw std::runtime_error("not a poivre exception");
   }
};

class ThrowsNothing : public Runnable {
public:
   void run() override {
      // deliberately does not throw
   }
};

// ---- throwaway suites used to exercise pass/fail counting without
//      polluting the real (PoivreSelfTests) results. Their printed
//      failures are expected and intentional. ----

class CountingVictimSuite : public TestSuite {
public:
   CountingVictimSuite() :
      TestSuite("CountingVictimSuite (failures below are intentional)") {
   }
protected:
   void runTests() override {
      require(true, "pass");
      require(false, "fail");
      requireFalse(false, "pass");
      requireFalse(true, "fail");
      requireStringEquals("a", "a", "pass");
      requireStringEquals("a", "b", "fail");
      requireNonEmptyString("x", "pass");
      requireNonEmptyString("", "fail");
      failTest("fail");
   }
};

class ExceptionVictimSuite : public TestSuite {
public:
   ExceptionVictimSuite() :
      TestSuite("ExceptionVictimSuite (failures below are intentional)") {
   }
protected:
   void runTests() override {
      requireException("BasicException", new ThrowsExpectedType(), "pass");
      requireException("BasicException", new ThrowsWrongType(), "fail: wrong type");
      requireException("BasicException", new ThrowsStdException(), "fail: std::exception");
      requireException("SomeExpectedException", new ThrowsNothing(), "fail: no exception thrown");
   }
};

class LifecycleVictimSuite : public TestSuite {
public:
   LifecycleVictimSuite() :
      TestSuite("LifecycleVictimSuite"),
      m_setupCount(0),
      m_tearDownCount(0) {
   }

   int getSetupCount() const { return m_setupCount; }
   int getTearDownCount() const { return m_tearDownCount; }

protected:
   void setup() override {
      ++m_setupCount;
   }

   void tearDown() override {
      ++m_tearDownCount;
   }

   void runTests() override {
      testOne();
      testTwo();
      testThree();
   }

private:
   void testOne() {
      TEST_CASE("one");
      require(true, "trivial");
   }

   void testTwo() {
      TEST_CASE("two");
      require(true, "trivial");
   }

   void testThree() {
      TEST_CASE("three");
      require(true, "trivial");
   }

   int m_setupCount;
   int m_tearDownCount;
};

}  // namespace

// ---- the real suite: every assertion here is expected to PASS.
//      A regression in the library will show up as a failure here. ----

class PoivreSelfTests : public TestSuite {
public:
   PoivreSelfTests() : TestSuite("PoivreSelfTests") {}

protected:
   void runTests() override {
      testAutoPointerRAII();
      testTempFileHandling();
      testCaseLifecycle();
      testAssertionCounting();
      testExceptionMatching();
   }

private:
   void testAutoPointerRAII() {
      TEST_CASE("AutoPointer releases owned object on scope exit");

      bool destroyed = false;
      {
         AutoPointer<DestructTracker> tracked(new DestructTracker(&destroyed));
         require(!destroyed, "object should be alive while AutoPointer is in scope");
      }
      require(destroyed, "object should be destroyed once AutoPointer goes out of scope");
   }

   void testTempFileHandling() {
      TEST_CASE("getTempFile/deleteFile create and remove real, unique files");

      const std::string f1 = getTempFile();
      const std::string f2 = getTempFile();

      requireNonEmptyString(f1, "getTempFile should return a non-empty path");
      require(f1 != f2, "consecutive getTempFile calls should return distinct paths");

      std::ifstream check(f1);
      require(check.good(), "file returned by getTempFile should actually exist");
      check.close();

      require(deleteFile(f1), "deleteFile should succeed for a file that exists");
      requireFalse(deleteFile(f1), "deleteFile should fail for a file that no longer exists");

      deleteFile(f2);
   }

   void testCaseLifecycle() {
      TEST_CASE("setup()/tearDown() run once per TEST_CASE");

      LifecycleVictimSuite victim;
      victim.run();

      require(victim.getSetupCount() == 3, "setup() should run once per test case");
      require(victim.getTearDownCount() == 3, "tearDown() should run once per test case");
      require(victim.getNumFailures() == 0, "all trivial assertions in the victim suite should pass");
   }

   void testAssertionCounting() {
      TEST_CASE("require/requireFalse/requireStringEquals/requireNonEmptyString/failTest count correctly");

      CountingVictimSuite victim;
      victim.run();

      require(victim.getNumTestsExecuted() == 9, "expected 9 executed assertions in the victim suite");
      require(victim.getNumFailures() == 5, "expected 5 recorded failures in the victim suite");
   }

   void testExceptionMatching() {
      TEST_CASE("requireException distinguishes correct type / wrong type / std::exception / no throw");

      ExceptionVictimSuite victim;
      victim.run();

      require(victim.getNumTestsExecuted() == 4, "expected 4 executed assertions in the victim suite");
      require(victim.getNumFailures() == 3, "expected 3 recorded failures in the victim suite");
   }
};

int main() {
   PoivreSelfTests suite;
   suite.run();

   const int executed = suite.getNumTestsExecuted();
   const int failures = suite.getNumFailures();

   printf("\n=============================================\n");
   printf("PoivreSelfTests: %d executed, %d failed\n", executed, failures);
   printf("=============================================\n");

   return (failures == 0) ? 0 : 1;
}
