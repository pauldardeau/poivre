// Regression helper for TestSuite::getTempFile()/deleteFile() under a
// controlled temporary directory. CTest starts it with TMPDIR, TMP, TEMP and
// TEMPDIR (whichever the platform consults) pointing at a directory whose
// name contains spaces, or at a directory that does not exist.
//
//   temp_cases usable <directory>   getTempFile() must create an empty file
//                                   directly inside <directory>, the path must
//                                   contain a space, deleteFile() must remove
//                                   it, and <directory> must end up empty
//   temp_cases unusable             getTempFile() must fail cleanly (empty
//                                   string, diagnostic) without crashing

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "TestMacros.h"
#include "TestSuite.h"

namespace {

namespace fs = std::filesystem;

class UsableDirectorySuite : public poivre::TestSuite {
public:
   explicit UsableDirectorySuite(const std::string& directory) :
      TestSuite("UsableDirectorySuite"),
      m_directory(directory) {
   }

protected:
   void runTests() override {
      TEST_CASE("temp file in a directory whose name contains spaces");

      std::error_code ec;
      const std::string path = getTempFile();
      requireNonEmptyString(path, "getTempFile should succeed");

      require(path.find(' ') != std::string::npos,
              "the test directory contains spaces, so the path must too");
      require(fs::equivalent(fs::path(path).parent_path(), fs::path(m_directory), ec) && !ec,
              "the file must be created in the directory named by the environment");
      require(fs::is_regular_file(path, ec), "the file must exist");
      require(fs::file_size(path, ec) == 0 && !ec, "the file must be empty");

      require(deleteFile(path), "deleteFile must remove it");
      requireFalse(fs::exists(path, ec), "the file must be gone");

      int leftovers = 0;
      for (const auto& entry : fs::directory_iterator(m_directory, ec)) {
         (void) entry;
         ++leftovers;
      }
      require(leftovers == 0, "nothing may be left behind in the temp directory");
   }

private:
   std::string m_directory;
};

class UnusableDirectorySuite : public poivre::TestSuite {
public:
   UnusableDirectorySuite() : TestSuite("UnusableDirectorySuite") {}

protected:
   void runTests() override {
      TEST_CASE("temp file creation failure is reported, not fatal");

      const std::string path = getTempFile();
      require(path.empty(), "getTempFile must return an empty string when it cannot create a file");
      std::printf("MARKER_SURVIVED_FAILURE\n");
   }
};

}

int main(int argc, char** argv) {
   if (argc >= 3 && std::strcmp(argv[1], "usable") == 0) {
      UsableDirectorySuite suite(argv[2]);
      suite.run();
      return suite.exitCode();
   }

   if (argc >= 2 && std::strcmp(argv[1], "unusable") == 0) {
      UnusableDirectorySuite suite;
      suite.run();
      return suite.exitCode();
   }

   std::printf("usage: temp_cases usable <directory> | unusable\n");
   return 2;
}
