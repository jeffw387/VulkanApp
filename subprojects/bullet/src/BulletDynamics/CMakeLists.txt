
set(target BulletDynamics)
set(dependencies BulletCollision LinearMath)

# 
# Sources
# 


SET(sources
	Character/btKinematicCharacterController.cpp
	ConstraintSolver/btConeTwistConstraint.cpp
	ConstraintSolver/btContactConstraint.cpp
	ConstraintSolver/btFixedConstraint.cpp
	ConstraintSolver/btGearConstraint.cpp
	ConstraintSolver/btGeneric6DofConstraint.cpp
	ConstraintSolver/btGeneric6DofSpringConstraint.cpp
	ConstraintSolver/btGeneric6DofSpring2Constraint.cpp
	ConstraintSolver/btHinge2Constraint.cpp
	ConstraintSolver/btHingeConstraint.cpp
	ConstraintSolver/btPoint2PointConstraint.cpp
	ConstraintSolver/btSequentialImpulseConstraintSolver.cpp
	ConstraintSolver/btSequentialImpulseConstraintSolverMt.cpp
	ConstraintSolver/btBatchedConstraints.cpp
	ConstraintSolver/btNNCGConstraintSolver.cpp
	ConstraintSolver/btSliderConstraint.cpp
	ConstraintSolver/btSolve2LinearConstraint.cpp
	ConstraintSolver/btTypedConstraint.cpp
	ConstraintSolver/btUniversalConstraint.cpp
	Dynamics/btDiscreteDynamicsWorld.cpp
	Dynamics/btDiscreteDynamicsWorldMt.cpp
	Dynamics/btSimulationIslandManagerMt.cpp
	Dynamics/btRigidBody.cpp
	Dynamics/btSimpleDynamicsWorld.cpp
#	Dynamics/Bullet-C-API.cpp
	Vehicle/btRaycastVehicle.cpp
	Vehicle/btWheelInfo.cpp
	Featherstone/btMultiBody.cpp
	Featherstone/btMultiBodyConstraintSolver.cpp
	Featherstone/btMultiBodyDynamicsWorld.cpp
	Featherstone/btMultiBodyJointLimitConstraint.cpp
	Featherstone/btMultiBodyConstraint.cpp
	Featherstone/btMultiBodyPoint2Point.cpp
	Featherstone/btMultiBodyFixedConstraint.cpp
	Featherstone/btMultiBodySliderConstraint.cpp
	Featherstone/btMultiBodyJointMotor.cpp
	Featherstone/btMultiBodyGearConstraint.cpp
	MLCPSolvers/btDantzigLCP.cpp
	MLCPSolvers/btMLCPSolver.cpp
	MLCPSolvers/btLemkeAlgorithm.cpp
)

SET(Root_HDRS
	../btBulletDynamicsCommon.h
	../btBulletCollisionCommon.h
)
SET(ConstraintSolver_HDRS
	ConstraintSolver/btConeTwistConstraint.h
	ConstraintSolver/btConstraintSolver.h
	ConstraintSolver/btContactConstraint.h
	ConstraintSolver/btContactSolverInfo.h
	ConstraintSolver/btFixedConstraint.h
	ConstraintSolver/btGearConstraint.h
	ConstraintSolver/btGeneric6DofConstraint.h
	ConstraintSolver/btGeneric6DofSpringConstraint.h
	ConstraintSolver/btGeneric6DofSpring2Constraint.h
	ConstraintSolver/btHinge2Constraint.h
	ConstraintSolver/btHingeConstraint.h
	ConstraintSolver/btJacobianEntry.h
	ConstraintSolver/btPoint2PointConstraint.h
	ConstraintSolver/btSequentialImpulseConstraintSolver.h
	ConstraintSolver/btSequentialImpulseConstraintSolverMt.h
	ConstraintSolver/btNNCGConstraintSolver.h
	ConstraintSolver/btSliderConstraint.h
	ConstraintSolver/btSolve2LinearConstraint.h
	ConstraintSolver/btSolverBody.h
	ConstraintSolver/btSolverConstraint.h
	ConstraintSolver/btTypedConstraint.h
	ConstraintSolver/btUniversalConstraint.h
)
SET(Dynamics_HDRS
	Dynamics/btActionInterface.h
	Dynamics/btDiscreteDynamicsWorld.h
	Dynamics/btDiscreteDynamicsWorldMt.h
	Dynamics/btSimulationIslandManagerMt.h
	Dynamics/btDynamicsWorld.h
	Dynamics/btSimpleDynamicsWorld.h
	Dynamics/btRigidBody.h
)
SET(Vehicle_HDRS
	Vehicle/btRaycastVehicle.h
	Vehicle/btVehicleRaycaster.h
	Vehicle/btWheelInfo.h
)

SET(Featherstone_HDRS
	Featherstone/btMultiBody.h
	Featherstone/btMultiBodyConstraintSolver.h
	Featherstone/btMultiBodyDynamicsWorld.h
	Featherstone/btMultiBodyLink.h
	Featherstone/btMultiBodyLinkCollider.h
	Featherstone/btMultiBodySolverConstraint.h
	Featherstone/btMultiBodyConstraint.h
	Featherstone/btMultiBodyJointLimitConstraint.h
	Featherstone/btMultiBodyConstraint.h
	Featherstone/btMultiBodyPoint2Point.h
	Featherstone/btMultiBodyFixedConstraint.h
	Featherstone/btMultiBodySliderConstraint.h
	Featherstone/btMultiBodyJointMotor.h
	Featherstone/btMultiBodyGearConstraint.h	
)

SET(MLCPSolvers_HDRS
	MLCPSolvers/btDantzigLCP.h
	MLCPSolvers/btDantzigSolver.h
	MLCPSolvers/btMLCPSolver.h
	MLCPSolvers/btMLCPSolverInterface.h
	MLCPSolvers/btPATHSolver.h
	MLCPSolvers/btSolveProjectedGaussSeidel.h
	MLCPSolvers/btLemkeSolver.h
	MLCPSolvers/btLemkeAlgorithm.h
)

SET(Character_HDRS
	Character/btCharacterControllerInterface.h
	Character/btKinematicCharacterController.h
)

SET(headers
	${Root_HDRS}
	${ConstraintSolver_HDRS}
	${Dynamics_HDRS}
	${Vehicle_HDRS}
	${Character_HDRS}
	${Featherstone_HDRS}
	${MLCPSolvers_HDRS}
)

# Set API export file and macro
string(MAKE_C_IDENTIFIER ${target} target_id)
string(TOUPPER ${target_id} target_id)
#set(feature_file         "include/${target}/${target}_features.h")
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
#    FEATURES cxx_static_assert
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
message("BulletDynamics includes: 
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
    $<INSTALL_INTERFACE:.>
)

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