# Find stb_image
#
# This module defines 
# - STB_IMAGE_INCLUDE_DIRS
# - STB_IMAGE_FOUND
#
# The following variables can be set as arguments for the module.
# - DEPENDENCIES_BASE_DIR: Root library directory of stb_image folder

# Additional modules
include(FindPackageHandleStandardArgs)

if (WIN32)
	# Find include files
	find_path(
		STB_IMAGE_INCLUDE_DIR
		NAMES stb_image_write.h
		PATHS
		$ENV{PROGRAMFILES}
		${DEPENDENCIES_BASE_DIR}
		${DEPENDENCIES_BASE_DIR}/stb_image
		DOC "The directory where stb_image_write.h resides")
else()
	# Find include files
	find_path(
		STB_IMAGE_INCLUDE_DIR
		NAMES stb_image_write.h
		PATHS
		${DEPENDENCIES_BASE_DIR}
		${DEPENDENCIES_BASE_DIR}/stb_image
		DOC "The directory where stb_image_write.h resides")
endif()

# Handle REQUIRD argument, define *_FOUND variable
find_package_handle_standard_args(STB_IMAGE DEFAULT_MSG STB_IMAGE_INCLUDE_DIR)

# Define STB_IMAGE_INCLUDE_DIRS
if (STB_IMAGE_FOUND)
	set(STB_IMAGE_INCLUDE_DIRS ${STB_IMAGE_INCLUDE_DIR})
endif()

# Hide some variables
mark_as_advanced(GLM_INCLUDE_DIR)