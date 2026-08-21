// Copyright Paul Dardeau, SwampBits LLC 2026
// BSD License

#ifndef POIVRE_TESTREGISTRY_H
#define POIVRE_TESTREGISTRY_H

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
   // suite's per-test-case counters into the running totals
   void recordResult(int numTestsExecuted, int numFailures);

   /**
    * Runs every self-registered POIVRE_TEST_CASE test case (grouped by
    * suite name, in registration order within a suite), prints a summary,
    * and returns the total number of failures - 0 means everything
    * passed, suitable for use directly as a process exit code.
    * @return total number of failed assertions across all registered
    *         test cases
    */
   int runAll();

private:
   TestRegistry() = default;

   struct Entry {
      std::string suiteName;
      std::string testName;
      TestCaseFn fn;
   };

   std::vector<Entry> m_entries;
   int m_totalTestsExecuted = 0;
   int m_totalFailures = 0;
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
