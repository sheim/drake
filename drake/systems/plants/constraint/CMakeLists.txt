macro(add_constraint_mex)
  add_mex(${ARGV} ${ARGV}.cpp)
  target_link_libraries(${ARGV} drakeRBM drakeMexUtil drakeRigidBodyConstraint drakeConstructRigidBodyConstraint)
  # These warnings are not worth fixing.
  target_compile_options(${ARGV} PRIVATE ${CXX_FLAGS_NO_SIGN_COMPARE})
endmacro()

add_library_with_exports(LIB_NAME drakeRigidBodyConstraint SOURCE_FILES RigidBodyConstraint.cpp)
target_link_libraries(drakeRigidBodyConstraint drakeRBM drakeGeometryUtil)
pods_install_libraries(drakeRigidBodyConstraint)
drake_install_headers(RigidBodyConstraint.h)
pods_install_pkg_config_file(drake-rigid-body-constraint
  LIBS -ldrakeRigidBodyConstraint
  REQUIRES
  VERSION 0.0.1)

if(MATLAB_FOUND)
  add_mex(drakeConstructRigidBodyConstraint SHARED constructPtrRigidBodyConstraint.cpp)
  target_link_libraries(drakeConstructRigidBodyConstraint drakeRigidBodyConstraint drakeRBM drakeMexUtil drakeGeometryUtil)
  pods_install_libraries(drakeConstructRigidBodyConstraint)
  add_constraint_mex(testSingleTimeKinCnstmex)
  add_constraint_mex(testMultipleTimeKinCnstmex)
  add_constraint_mex(testQuasiStaticConstraintmex)
  add_constraint_mex(testPostureConstraintmex)
  add_constraint_mex(testMultipleTimeLinearPostureConstraintmex)
  add_constraint_mex(testSingleTimeLinearPostureConstraintmex)
  add_constraint_mex(constraintTypemex)
  add_constraint_mex(constraintCategorymex)
  add_constraint_mex(constructPtrRigidBodyConstraintmex)
  add_constraint_mex(updatePtrRigidBodyConstraintmex)
endif()

add_executable(testConstraintConstructor test/testConstraintConstructor.cpp)
target_link_libraries(testConstraintConstructor drakeRBM drakeRigidBodyConstraint)
add_test(NAME testConstraintConstructor WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test" COMMAND testConstraintConstructor)

add_library_with_exports(LIB_NAME drakeDynamicConstraint
  SOURCE_FILES direct_collocation_constraint.cc)
target_link_libraries(drakeDynamicConstraint drakeCommon)
drake_install_headers(direct_collocation_constraint.h)
pods_install_libraries(drakeDynamicConstraint)
pods_install_pkg_config_file(drake-dynamic-constraint
  LIBS -ldrakeDynamicConstraint
  REQUIRES
  VERSION 0.0.0)

add_subdirectory(test)
