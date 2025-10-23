set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
file(
    GLOB_RECURSE test_sources
    LIST_DIRECTORIES false
    RELATIVE "${ROOT_DIR}"
    "${ROOT_DIR}/*.cpp")

add_custom_target(all_test_binaries)

foreach(test_executable ${test_sources})
    cmake_path(REMOVE_EXTENSION test_executable OUTPUT_VARIABLE _test_target)
    string(MAKE_C_IDENTIFIER ${_test_target} _test_target)
    set(_test_target test_${_test_target})
    add_executable(${_test_target} EXCLUDE_FROM_ALL ${test_executable})
    target_link_libraries(${_test_target} PRIVATE nias_cpp Boost::ut ${Python_LIBRARIES})
    add_dependencies(all_test_binaries ${_test_target} nias_cpp_bindings)
    add_test(NAME ${_test_target} COMMAND ${_test_target})
endforeach()
