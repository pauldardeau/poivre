// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef POIVRE_TESTREGISTRY_H
#define POIVRE_TESTREGISTRY_H

#include <atomic>
#include <string>
#include <vector>

namespace poivre
{

/**
 * Backing store for POIVRE_TEST_CASE. Test cases register themselves here
 * at static-initialization time (see AutoRegisterTestCase), so a suite's
 * test cases no longer need to be listed by hand in runTests(). Not
 * intended to be used directly - see the POIVRE_TEST_CASE macro in
 * TestMacros.h.
 */
class TestRegistry
{
public:
   using TestCaseFn = void (*)();

   static TestRegistry& instance();

   // no copies
   TestRegistry(const TestRegistry&) = delete;
   TestRegistry& operator=(const TestRegistry&) = delete;

   void addTestCase(const std::string& suiteName,
                    const std::string& testName,
                    TestCaseFn fn);

   // called by each registered test case as it finishes, to fold its
   // suite's per-test-case counters into the running totals (counts as
   // one test case run)
   void recordResult(int numTestsExecuted, int numFailures);

   /**
    * Runs every self-registered POIVRE_TEST_CASE test case (grouped by
    * suite name, in registration order within a suite) and prints a
    * summary. Exceptions escaping a test case are recorded as failures and
    * the remaining test cases still run.
    *
    * @return total number of failed assertions across all registered
    *         test cases. This is a COUNT, not an exit status: do not return
    *         it from main(), because exit statuses are truncated (to 8
    *         bits on POSIX) and 256 failures would look like success. Use
    *         runAllAndGetExitCode() or exitCode() for that.
    */
   int runAll();

   /**
    * Runs runAll() and returns exitCode(); the intended body of main():
    *    return poivre::TestRegistry::instance().runAllAndGetExitCode();
    */
   int runAllAndGetExitCode();

   /**
    * Process exit code for the most recent runAll(): 0 if at least one
    * test case ran, at least one assertion was evaluated and none failed;
    * 1 otherwise (including when nothing ran).
    */
   int exitCode() const;

   /**
    * The rule used by exitCode(), usable on any set of totals.
    * @return 0 if numTestCases > 0, numAssertions > 0 and numFailures == 0,
    *         otherwise 1
    */
   static int exitCodeFor(int numTestCases, int numAssertions, int numFailures);

   // totals from the most recent runAll()
   int getNumTestCasesRun() const;
   int getNumAssertions() const;
   int getNumFailures() const;

private:
   TestRegistry() = default;

   struct Entry {
      std::string suiteName;
      std::string testName;
      TestCaseFn fn;
   };

   std::vector<Entry> m_entries;
   std::atomic<int> m_totalTestCasesRun{0};
   std::atomic<int> m_totalTestsExecuted{0};
   std::atomic<int> m_totalFailures{0};
};

/**
 * RAII helper that registers one test case with TestRegistry as a side
 * effect of its own static construction. Not intended to be used
 * directly - see the POIVRE_TEST_CASE macro in TestMacros.h.
 */
class AutoRegisterTestCase
{
public:
   AutoRegisterTestCase(const std::string& suiteName,
                        const std::string& testName,
                        TestRegistry::TestCaseFn fn);
};

}

#endif
