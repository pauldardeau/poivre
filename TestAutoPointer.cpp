// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#include "TestAutoPointer.h"
#include "AutoPointer.h"

using namespace poivre;

//******************************************************************************

TestAutoPointer::TestAutoPointer() :
   TestSuite("TestAutoPointer") {
}

//******************************************************************************

void TestAutoPointer::runTests() {
   testConstructor();
   testOperatorArrow();
   testAssign();
   testHaveObject();
   testDestructor();
}

//******************************************************************************

void TestAutoPointer::testConstructor() {
   TEST_CASE("testConstructor");
   //TODO: implement testConstructor
}

//******************************************************************************

void TestAutoPointer::testOperatorArrow() {
   TEST_CASE("testOperatorArrow");
   //TODO: implement testOperatorArrow
}

//******************************************************************************

void TestAutoPointer::testAssign() {
   TEST_CASE("testAssign");
   //TODO: implement testAssign
}

//******************************************************************************

void TestAutoPointer::testHaveObject() {
   TEST_CASE("testHaveObject");
   //TODO: implement testHaveObject
}

//******************************************************************************

void TestAutoPointer::testDestructor() {
   TEST_CASE("testDestructor");
   //TODO: implement testDestructor
}

//******************************************************************************

