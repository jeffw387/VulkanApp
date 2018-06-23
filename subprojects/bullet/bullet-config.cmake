
# This config script tries to locate the project either in its source tree
# or from an install location.
# 
# Please adjust the list of submodules to search for.

# Macro to search for a specific module
macro(find_module FILENAME MODULE_NAME)
	#message("find_module with FILENAME:${FILENAME} and MODULE_NAME:${MODULE_NAME}")
    if(EXISTS "${FILENAME}")
        set(${MODULE_NAME}_FOUND TRUE)
        include("${FILENAME}")
    endif()
endmacro()

message("Requested components: ${bullet_FIND_COMPONENTS}")
# Macro to search for all modules
macro(find_modules PREFIX)
	#message("Starting find_modules execution with prefix:${PREFIX}.")
    foreach(MODULE_NAME ${bullet_FIND_COMPONENTS})
        if(TARGET ${MODULE_NAME})
			#message("${MODULE_NAME} exists as a target.")
            set(${MODULE_NAME}_FOUND TRUE)
        else()
            find_module("${CMAKE_CURRENT_LIST_DIR}/${PREFIX}/${MODULE_NAME}/${MODULE_NAME}-export.cmake" ${MODULE_NAME})
        endif()
    endforeach(MODULE_NAME)
endmacro()

#message(WARNING "Starting Bullet module search:\n")
# Try install location
foreach(MODULE_NAME ${bullet_FIND_COMPONENTS})
	set(${MODULE_NAME}_FOUND FALSE)
endforeach(MODULE_NAME)

find_modules("cmake")

# Try common build locations
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    find_modules("build-debug/cmake")
    find_modules("build/cmake")
else()
    find_modules("build/cmake")
    find_modules("build-debug/cmake")
endif()

set(bullet_FOUND TRUE)
foreach(MODULE_NAME ${bullet_FIND_COMPONENTS})
	message("Module ${MODULE_NAME} found? ${${MODULE_NAME}_FOUND}")
	if(NOT ${${MODULE_NAME}_FOUND})
		set(bullet_FOUND FALSE)
	endif()
endforeach(MODULE_NAME)
message("Module found var: ${bullet_FOUND}.")

# Signal success/failure to CMake
