// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTAUTOPOINTER_H
#define POIVRE_TESTAUTOPOINTER_H

#include "TestSuite.h"

namespace poivre
{

class TestAutoPointer : public TestSuite
{
protected:
   void runTests();
   
   void testConstructor();
   void testOperatorArrow();
   void testAssign();
   void testHaveObject();
   void testDestructor();

public:
   TestAutoPointer();
   
};

}

#endif
