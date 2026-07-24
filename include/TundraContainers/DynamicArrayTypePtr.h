
#ifndef TUNDRA_DYNAMICARRAYTypePtr_H
#define TUNDRA_DYNAMICARRAYTypePtr_H

#include "tundra/internal/MacroHelper.h"

// Type flags for the template 
#define TUNDRA_NEEDS_CUSTOM_COPY 0
#define TUNDRA_NEEDS_CUSTOM_FREE 0
#define TUNDRA_NEEDS_CUSTOM_MOVE 0
#define TUNDRA_NEEDS_CUSTOM_INIT 0
#define TUNDRA_TYPE Type*
#define TUNDRA_TYPENAME TypePtr

// -----------------------------------------------------------------------------

// Create specialization for the given type
#include "tundra/internal/container_templates/DynamicArray.h"

// Clean up
#undef TUNDRA_TYPE
#undef TUNDRA_TYPENAME
#undef TUNDRA_NEEDS_CUSTOM_COPY
#undef TUNDRA_NEEDS_CUSTOM_FREE
#undef TUNDRA_NEEDS_CUSTOM_MOVE
#undef TUNDRA_NEEDS_CUSTOM_INIT
#endif // TUNDRA_DYNAMICARRAYTypePtr_H
