// Historical flat include style, as used by existing consumers (chaudiere,
// chapeau): only available when poivre's source tree is on the include path.
// One conventional include is mixed in on purpose: both spellings must name the
// same declarations (no redefinition errors).

#include <cstdio>
#include <string>

#include "BasicException.h"
#include "Runnable.h"
#include "TestMacros.h"
#include "TestRegistry.h"
#include "TestSuite.h"

#include <poivre/TestSuite.h>

#include "body.inc"
