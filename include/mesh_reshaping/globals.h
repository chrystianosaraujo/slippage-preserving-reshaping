#pragma once

#include <mesh_reshaping/types.h>

// When enabled, this disables the generation and export of all debug information
#define OPTIMIZED_VERSION_ON 1

// Enables the sphericity term
#define SPHERICITY_ON 1

// Enables/disables the export of debug files for the sphericity term
#define EXPORT_SPHERICITY_DEBUG_FILES_ON 0

namespace reshaping {
    namespace globals {
        inline constexpr int MAJOR_VERSION = 1;
        inline constexpr int MINOR_VERSION = 0;

        inline constexpr const char* VERSION_TAG = "code release (Feb 15, 2024)";
    }
}
