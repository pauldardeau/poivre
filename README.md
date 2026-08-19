[![Build](https://github.com/pauldardeau/poivre/actions/workflows/cpp-build.yml/badge.svg)](https://github.com/pauldardeau/poivre/actions/workflows/cpp-build.yml)

# poivre

A small, dependency-free C++ unit testing library.

This project has no dependencies on other projects. When built, it's a single shared library file named `poivre.so`.

## Building

```bash
make        # builds poivre.so
make test   # builds and runs poivre's own self-tests (tests/SelfTests.cpp)
```

## Writing a test suite

A test suite is a class that derives from `poivre::TestSuite` and implements `runTests()`.
Each individual test is a member function that starts with the `TEST_CASE` macro, which
names the test (for output) and triggers the suite's `setup()`/`tearDown()` hooks around it.

```cpp
#include "TestSuite.h"
#include "TestMacros.h"

using namespace poivre;

class MathTests : public TestSuite {
public:
   MathTests() : TestSuite("MathTests") {}

protected:
   void runTests() override {
      testAddition();
      testSubtraction();
   }

private:
   void testAddition() {
      TEST_CASE("addition");
      require(2 + 2 == 4, "2 + 2 should equal 4");
   }

   void testSubtraction() {
      TEST_CASE("subtraction");
      require(5 - 3 == 2, "5 - 3 should equal 2");
   }
};

int main() {
   MathTests suite;
   suite.run();
   return (suite.getNumFailures() == 0) ? 0 : 1;
}
```

Compile it alongside poivre's sources and your headers directory, for example:

```bash
c++ -std=c++20 -I/path/to/poivre my_tests.cpp \
   /path/to/poivre/TestCase.cpp /path/to/poivre/TestSuite.cpp \
   -o my_tests
./my_tests
```

(You can link against `poivre.so` instead if you prefer a shared build; either way the
headers are all you need at compile time.)

## Assertions

All of these are methods on `TestSuite`, so call them directly from within a `TEST_CASE`:

| Method | Fails when |
|---|---|
| `require(bool expression, testDesc)` | `expression` is `false` |
| `requireFalse(bool expression, testDesc)` | `expression` is `true` |
| `requireStringEquals(expected, actual, testDesc)` | `expected != actual` |
| `requireNonEmptyString(actual, testDesc)` | `actual` is empty |
| `requireException(exceptionType, Runnable*, testDesc)` | see below |
| `failTest(message)` | always — unconditionally records a failure |

`testDesc` is optional on all of the above and is only used in failure output.

### Testing that code throws

`requireException` expects a `poivre::BasicException` (or a subclass overriding `getType()`)
to be thrown with a type name matching `exceptionType`. Wrap the code under test in a
`poivre::Runnable` — `requireException` takes ownership of it and calls `run()`:

```cpp
#include "Runnable.h"
#include "BasicException.h"

class DivideByZero : public Runnable {
public:
   void run() override {
      throwIfDenominatorIsZero(1, 0);  // expected to throw a BasicException
   }
};
```

```cpp
requireException("DivideByZeroException", new DivideByZero(), "dividing by zero should throw");
```

It also fails (with a descriptive message) if the code throws the wrong exception type,
throws a plain `std::exception`, or doesn't throw at all.

## Suite and test lifecycle hooks

Override any of these `protected` virtuals in your suite as needed:

| Method | Runs |
|---|---|
| `suiteSetup()` / `suiteTearDown()` | once, before/after the whole suite |
| `setup()` / `tearDown()` | once per `TEST_CASE`, before/after each test |

## Other utilities

- `getTempFile()` / `deleteFile(path)` — create/remove a scratch file for tests that need one
- `getNumTestsExecuted()` / `getNumFailures()` — inspect results after `run()`, e.g. to set a
  process exit code for CI (see the `main()` example above)
- `poivre::AutoPointer<T>` — a minimal RAII owning-pointer used internally (e.g. by
  `requireException` to own the `Runnable`)

## A complete example

See [`tests/SelfTests.cpp`](tests/SelfTests.cpp) — poivre's own test suite, which exercises
every assertion type, exception matching, temp files, and the setup/tearDown lifecycle.
