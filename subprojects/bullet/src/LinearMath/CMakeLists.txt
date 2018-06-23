
set(target LinearMath)
set(dependencies)

SET(sources
	btAlignedAllocator.cpp
	btConvexHull.cpp
	btConvexHullComputer.cpp
	btGeometryUtil.cpp
	btPolarDecomposition.cpp
	btQuickprof.cpp
	btSerializer.cpp
	btSerializer64.cpp
	btThreads.cpp
	btVector3.cpp
	TaskScheduler/btTaskScheduler.cpp
	TaskScheduler/btThreadSupportPosix.cpp
	TaskScheduler/btThreadSupportWin32.cpp
)

SET(headers
	btAabbUtil2.h
	btAlignedAllocator.h
	btAlignedObjectArray.h
	btConvexHull.h
	btConvexHullComputer.h
	btDefaultMotionState.h
	btGeometryUtil.h
	btGrahamScan2dConvexHull.h
	btHashMap.h
	btIDebugDraw.h
	btList.h
	btMatrix3x3.h
	btMinMax.h
	btMotionState.h
	btPolarDecomposition.h
	btPoolAllocator.h
	btQuadWord.h
	btQuaternion.h
	btQuickprof.h
	btRandom.h
	btScalar.h
	btSerializer.h
	btStackAlloc.h
	btThreads.h
	btTransform.h
	btTransformUtil.h
	btVector3.h
	TaskScheduler/btThreadSupportInterface.h
)

# Set API export file and macro
string(MAKE_C_IDENTIFIER ${target} target_id)
string(TOUPPER ${target_id} target_id)
set(feature_file         "include/${target}/${target}_features.h")
set(export_file          "include/${target}/${target}_export.h")
set(template_export_file "include/${target}/${target}_api.h")
set(export_macro         "${target_id}_API")

ADD_LIBRARY(${target} ${sources} ${headers})

# Create namespaced alias
add_library(${META_PROJECT_NAME}::${target} ALIAS ${target})

export(TARGETS ${target} NAMESPACE ${META_PROJECT_NAME}:: 
	FILE ${PROJECT_BINARY_DIR}/CMAKE/${target}/${target}-export.cmake)

#write_compiler_detection_header(
#    FILE ${feature_file}
#    PREFIX ${target_id}
#    COMPILERS AppleClang Clang GNU MSVC
#    FEATURES cxx_std_11
#    VERSION 3.2)

# Create API export header
generate_export_header(${target}
    EXPORT_FILE_NAME  ${export_file}
    EXPORT_MACRO_NAME ${export_macro})

generate_template_export_header(${target}
    ${target_id}
    ${template_export_file})

# 
# Project options
# 

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
    VERSION ${BULLET_VERSION}
    SOVERSION ${BULLET_VERSION})


# 
# Include directories
# 

message("LinearMath includes: 
	\nPROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}
    \nCMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}
    \nCMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}
    \nDEFAULT_INCLUDE_DIRECTORIES: ${DEFAULT_INCLUDE_DIRECTORIES}
	\nPROJECT_INCLUDES: ${PROJECT_INCLUDES}")

target_include_directories(${target}
    PRIVATE
    ${PROJECT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}

    PUBLIC
    ${DEFAULT_INCLUDE_DIRECTORIES}
	$<BUILD_INTERFACE:${PROJECT_INCLUDES}>

    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:.>)

# 
# Libraries
# 

target_link_libraries(${target}
    PRIVATE
	$<$<BOOL:${BUILD_SHARED_LIBS}>:${dependencies}>

    PUBLIC
    ${DEFAULT_LIBRARIES}

    INTERFACE)

# 
# Compile definitions
# 

target_compile_definitions(${target}
    PRIVATE

    PUBLIC
    $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:${target_id}_STATIC_DEFINE>
    ${DEFAULT_COMPILE_DEFINITIONS}

    INTERFACE
)


# 
# Compile options
# 

target_compile_options(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_COMPILE_OPTIONS}

    INTERFACE
)


# 
# Linker options
# 

target_link_libraries(${target}
    PRIVATE

    PUBLIC
    ${DEFAULT_LINKER_OPTIONS}

    INTERFACE)

# 
# Deployment
# 

# Library
install(TARGETS ${target}
    EXPORT  "${target}-export"            COMPONENT dev
    RUNTIME DESTINATION ${INSTALL_BIN}    COMPONENT runtime
    LIBRARY DESTINATION ${INSTALL_SHARED} COMPONENT runtime
    ARCHIVE DESTINATION ${INSTALL_LIB}    COMPONENT dev)

# Header files
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include/${target} DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev)

# Generated header files
install(DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/include/${target} DESTINATION ${INSTALL_INCLUDE}
    COMPONENT dev)

# CMake config
install(EXPORT ${target}-export
    NAMESPACE   ${META_PROJECT_NAME}::
    DESTINATION ${INSTALL_CMAKE}/${target}
    COMPONENT   dev)