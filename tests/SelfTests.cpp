// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "AutoPointer.h"
#include "BasicException.h"
#include "Runnable.h"
#include "TestCase.h"
#include "TestMacros.h"
#include "TestRegistry.h"
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

// ---- fixture for POIVRE_TEST_CASE (self-registering test cases) below.
//      Its own runTests() is deliberately empty: the whole point of
//      POIVRE_TEST_CASE is that its test cases don't need to be listed
//      there. ----

class PoivreTestCaseDemoSuite : public TestSuite {
public:
   PoivreTestCaseDemoSuite() :
      TestSuite("PoivreTestCaseDemoSuite"),
      m_counter(0) {
      ++s_constructCount;
   }

   int getCounter() const { return m_counter; }
   void bumpCounter() { ++m_counter; }

   static int s_constructCount;
   static int s_setupCount;
   static int s_tearDownCount;

protected:
   void setup() override {
      ++s_setupCount;
   }

   void tearDown() override {
      ++s_tearDownCount;
   }

   void runTests() override {
   }

private:
   int m_counter;
};

int PoivreTestCaseDemoSuite::s_constructCount = 0;
int PoivreTestCaseDemoSuite::s_setupCount = 0;
int PoivreTestCaseDemoSuite::s_tearDownCount = 0;

}  // namespace

// each POIVRE_TEST_CASE below gets its own freshly constructed
// PoivreTestCaseDemoSuite instance - if bumpCounter() from the first test
// leaked into the second, testFreshInstancePerCase (below) would fail.

POIVRE_TEST_CASE(PoivreTestCaseDemoSuite, testBumpCounter) {
   require(getCounter() == 0, "a freshly constructed instance should start with counter == 0");
   bumpCounter();
   require(getCounter() == 1, "bumpCounter should increment the counter");
}

POIVRE_TEST_CASE(PoivreTestCaseDemoSuite, testFreshInstancePerCase) {
   require(getCounter() == 0, "each POIVRE_TEST_CASE should get its own fresh instance, not share state with testBumpCounter");
}

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
      testSelfRegisteringTestCases();
      testExitCodeHelpers();
      testTempFileCreation();
      testTempFileUniquenessUnderConcurrency();
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

   void testSelfRegisteringTestCases() {
      TEST_CASE("POIVRE_TEST_CASE self-registers, discovers, and runs without being listed in runTests()");

      PoivreTestCaseDemoSuite::s_constructCount = 0;
      PoivreTestCaseDemoSuite::s_setupCount = 0;
      PoivreTestCaseDemoSuite::s_tearDownCount = 0;

      require(TestRegistry::instance().getNumRegisteredTestCases() == 2,
              "exactly the 2 demo POIVRE_TEST_CASE cases should be registered");

      const int registryFailures = TestRegistry::instance().runAll();

      require(registryFailures == 0, "both demo POIVRE_TEST_CASE cases should pass");
      require(PoivreTestCaseDemoSuite::s_constructCount == 2,
              "each of the 2 registered POIVRE_TEST_CASE cases should construct its own fixture instance");
      require(PoivreTestCaseDemoSuite::s_setupCount == 2,
              "setup() should run once per POIVRE_TEST_CASE, same as it does per TEST_CASE");
      require(PoivreTestCaseDemoSuite::s_tearDownCount == 2,
              "tearDown() should run once per POIVRE_TEST_CASE, same as it does per TEST_CASE");
   }

   void testTempFileCreation() {
      TEST_CASE("getTempFile creates an empty regular file in the platform temp directory");

      namespace fs = std::filesystem;

      const std::string path = getTempFile();
      requireNonEmptyString(path, "getTempFile should return a path");

      std::error_code ec;
      require(fs::is_regular_file(path, ec), "the returned path should be an existing regular file");
      require(fs::file_size(path, ec) == 0 && !ec, "the new file should be empty");

      const fs::path parent = fs::path(path).parent_path();
      require(fs::equivalent(parent, fs::temp_directory_path(), ec) && !ec,
              "the file should be in std::filesystem::temp_directory_path()");

      const std::string name = fs::path(path).filename().string();
      require(name.rfind("poivre", 0) == 0, "the file name should start with 'poivre'");

      // existing contents are never reused: a second call must not hand out
      // (or truncate) the first file
      {
         std::ofstream out(path);
         out << "keep me";
      }
      const std::string second = getTempFile();
      require(second != path, "a second call should return a different path");
      {
         std::ifstream in(path);
         std::string content;
         std::getline(in, content);
         requireStringEquals("keep me", content, "creating another temp file must not touch the first");
      }

      require(deleteFile(path), "deleteFile should remove the file");
      requireFalse(fs::exists(path, ec), "the file should be gone after deleteFile");
      requireFalse(deleteFile(path), "deleting a file that is already gone should report false");
      require(deleteFile(second), "the second file should be removable");
   }

   void testTempFileUniquenessUnderConcurrency() {
      TEST_CASE("concurrent getTempFile calls return distinct, existing files; all are cleaned up");

      namespace fs = std::filesystem;
      const int threads = 8;
      const int perThread = 25;

      std::vector<std::string> paths;
      std::mutex pathsLock;

      std::vector<std::thread> workers;
      for (int t = 0; t < threads; ++t) {
         workers.emplace_back([this, &paths, &pathsLock, perThread] {
            for (int i = 0; i < perThread; ++i) {
               const std::string path = getTempFile();
               std::lock_guard<std::mutex> guard(pathsLock);
               paths.push_back(path);
            }
         });
      }
      for (auto& worker : workers) {
         worker.join();
      }

      require(static_cast<int>(paths.size()) == threads * perThread, "every call should return");

      const std::set<std::string> distinct(paths.begin(), paths.end());
      require(distinct.size() == paths.size(), "all returned paths should be distinct");
      requireFalse(distinct.count("") > 0, "no call should have failed");

      std::error_code ec;
      int missing = 0;
      for (const std::string& path : paths) {
         if (!fs::exists(path, ec)) {
            ++missing;
         }
      }
      require(missing == 0, "every returned file should exist");

      int notDeleted = 0;
      for (const std::string& path : paths) {
         if (!deleteFile(path) || fs::exists(path, ec)) {
            ++notDeleted;
         }
      }
      require(notDeleted == 0, "every file should be deleted by deleteFile");
   }

   // the pure exit-code rules; process-level behaviour (exit status of real
   // executables, exceptions, concurrency) is covered by the subprocess
   // regression tests in tests/regression
   void testExitCodeHelpers() {
      TEST_CASE("exit code helpers never turn failures or empty runs into success");

      require(TestRegistry::exitCodeFor(1, 1, 0) == 0, "tests ran and passed -> 0");
      require(TestRegistry::exitCodeFor(1, 1, 1) == 1, "one failure -> 1");
      require(TestRegistry::exitCodeFor(1, 256, 256) == 1, "256 failures -> 1, not 256 truncated to 0");
      require(TestRegistry::exitCodeFor(1, 512, 512) == 1, "512 failures -> 1, not 512 truncated to 0");
      require(TestRegistry::exitCodeFor(0, 0, 0) == 1, "no test cases -> 1");
      require(TestRegistry::exitCodeFor(3, 0, 0) == 1, "no assertions -> 1");

      CountingVictimSuite failing;
      failing.run();
      require(failing.exitCode() == 1, "a suite with failures has exit code 1");

      LifecycleVictimSuite passing;
      passing.run();
      require(passing.exitCode() == 0, "a passing suite has exit code 0");

      LifecycleVictimSuite neverRun;
      require(neverRun.exitCode() == 1, "a suite that evaluated nothing has exit code 1");
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

   return suite.exitCode();
}
