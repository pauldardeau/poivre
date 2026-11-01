// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <exception>
#include <filesystem>
#include <random>
#include <system_error>

#ifdef _WIN32
#  ifndef NOMINMAX
#     define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#     define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif

#include "TestSuite.h"
#include "BasicException.h"
#include "AutoPointer.h"

using namespace poivre;

namespace {

enum class CreateResult { Created, AlreadyExists, Failed };

// Atomically creates a NEW, empty file at path. It either creates the file or
// reports why it could not; it never opens, truncates or reuses an existing
// file (O_EXCL / CREATE_NEW), so two creators can never end up with the same
// file, and there is no check-then-create window.
CreateResult createNewFile(const std::filesystem::path& path,
                           std::string& errorText) {
#ifdef _WIN32
   HANDLE handle = ::CreateFileW(path.c_str(),
                                 GENERIC_READ | GENERIC_WRITE,
                                 0,
                                 nullptr,
                                 CREATE_NEW,
                                 FILE_ATTRIBUTE_NORMAL,
                                 nullptr);
   if (handle == INVALID_HANDLE_VALUE) {
      const DWORD error = ::GetLastError();
      errorText = "Windows error " + std::to_string(error);
      return (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) ?
         CreateResult::AlreadyExists : CreateResult::Failed;
   }

   ::CloseHandle(handle);
   return CreateResult::Created;
#else
   const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
   if (fd == -1) {
      const int error = errno;
      errorText = std::strerror(error);
      return (error == EEXIST) ? CreateResult::AlreadyExists :
                                 CreateResult::Failed;
   }

   ::close(fd);
   return CreateResult::Created;
#endif
}

// "poivre" followed by 16 hex digits drawn from the OS random source, mixed
// with a process-wide counter and the clock so that a weak random_device
// still produces distinct names. Uniqueness is not relied upon: creation is
// exclusive and the caller retries on a collision.
std::string uniqueFileName() {
   static std::atomic<unsigned long long> counter{0};

   unsigned long long value = 0;
   try {
      std::random_device device;
      value = (static_cast<unsigned long long>(device()) << 32) ^ device();
   } catch (const std::exception&) {
      // fall through to the clock and counter
   }

   value ^= static_cast<unsigned long long>(
      std::chrono::steady_clock::now().time_since_epoch().count());
   value ^= (++counter) * 0x9E3779B97F4A7C15ULL;

   static const char hex[] = "0123456789abcdef";
   std::string name("poivre");
   for (int shift = 60; shift >= 0; shift -= 4) {
      name += hex[(value >> shift) & 0xF];
   }

   return name;
}

// for diagnostics only; never throws
std::string printablePath(const std::filesystem::path& path) {
   try {
      return path.string();
   } catch (const std::exception&) {
      return std::string("<path not representable>");
   }
}

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
   namespace fs = std::filesystem;

   // the platform's temporary directory: TMPDIR/TMP/TEMP/TEMPDIR (falling
   // back to /tmp) on POSIX, GetTempPath() on Windows
   std::error_code ec;
   const fs::path directory = fs::temp_directory_path(ec);
   if (ec || directory.empty()) {
      printf("error: unable to determine the temporary directory: %s\n",
             ec ? ec.message().c_str() : "empty path");
      fflush(stdout);
      return std::string("");
   }

   const int maxAttempts = 100;
   for (int attempt = 0; attempt < maxAttempts; ++attempt) {
      const fs::path candidate = directory / uniqueFileName();

      std::string errorText;
      const CreateResult result = createNewFile(candidate, errorText);

      if (result == CreateResult::Created) {
         try {
            return candidate.string();
         } catch (const std::exception& e) {
            // the path cannot be represented in this narrow string type
            std::error_code ignored;
            fs::remove(candidate, ignored);
            printf("error: temporary file path is not representable: %s\n",
                   e.what());
            fflush(stdout);
            return std::string("");
         }
      }

      if (result == CreateResult::Failed) {
         printf("error: unable to create a temporary file in '%s': %s\n",
                printablePath(directory).c_str(),
                errorText.c_str());
         fflush(stdout);
         return std::string("");
      }

      // AlreadyExists: an astronomically unlikely name collision; try again
   }

   printf("error: no unused temporary file name found in '%s'\n",
          printablePath(directory).c_str());
   fflush(stdout);
   return std::string("");
}

//******************************************************************************

bool TestSuite::deleteFile(const std::string& filePath) {
   std::error_code ec;
   const bool removed = std::filesystem::remove(std::filesystem::path(filePath), ec);
   return removed && !ec;
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
   } catch (std::exception&) {
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

