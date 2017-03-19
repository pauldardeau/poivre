// Copyright Paul Dardeau, SwampBits LLC 2017
// BSD License

#ifndef POIVRE_TESTMACROS_H
#define POIVRE_TESTMACROS_H

#include "TestCase.h"


#define TEST_CASE(testCaseName) \
poivre::TestCase macroTestCase(*this,testCaseName);


#endif

