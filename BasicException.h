// Compatibility forwarding header.
//
// The public headers now live in include/poivre/. New code should include them
// as <poivre/BasicException.h>. This file keeps the historical flat include
// (#include "BasicException.h") working for source-tree consumers, such as projects
// that vendor poivre as a submodule and add its root directory to their include
// path. It is not installed.

#include "include/poivre/BasicException.h"
