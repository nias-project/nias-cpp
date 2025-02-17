set(TEST_EXECUTABLES gram_schmidt.cpp vectorarray/numpy.cpp vectorarray/list.cpp)

foreach(test_executable IN LISTS TEST_EXECUTABLES)
    cmake_path(REMOVE_EXTENSION test_executable OUTPUT_VARIABLE _test_target)
    string(MAKE_C_IDENTIFIER ${_test_target} _test_target)
    set(_test_target test_${_test_target})
    add_executable(${_test_target} ${test_executable})
    target_link_libraries(${_test_target} PRIVATE nias_cpp Boost::ut ${Python_LIBRARIES})
    get_target_property(linked_libraries ${_test_target} LINK_LIBRARIES)

    # copy python shared libraries to test directory
    # for some reason, the Python_LIBRARIES variable only contains the static libraries, so we
    # manually search for *.so or *.dll files in Python_RUNTIME_LIBRARY_DIRS (which has been
    # set by the find_package(Python ...) call)
    file(GLOB shared_python_libs "${Python_RUNTIME_LIBRARY_DIRS}/*.so" "${Python_RUNTIME_LIBRARY_DIRS}/*.dll")
    set(_library_files ${shared_python_libs})

    foreach(lib IN LISTS linked_libraries)
        if(TARGET ${lib})
            get_property(
                target_type
                TARGET ${lib}
                PROPERTY TYPE)
            if(${target_type} STREQUAL "SHARED_LIBRARY")
                list(APPEND library_dirs $<TARGET_FILE_DIR:${lib}>)
                list(APPEND _library_files $<TARGET_FILE:${lib}>)
                get_property(
                    is_imported_target
                    TARGET ${lib}
                    PROPERTY IMPORTED)
                # For imported targets, the TARGET_PDB_FILE generator expression is not allowed
                if(MSVC AND NOT ${is_imported_target})
                    list(APPEND _library_files
                         # For msvc presets with debug information, we also have to copy the .pdb files
                         $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:${lib}>>)
                endif()
            endif()
        endif()
    endforeach()
    # to avoid shared librarys not being found when executing a test or example manually (not
    # using ctest), e.g. from within VSCode or from the terminal, we copy the shared libraries
    # over to the test directory
    if(NOT TARGET nias_cpp_copy_libraries_to_test_folder)
        add_custom_target(
            nias_cpp_copy_libraries_to_test_folder
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${_test_target}>
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_library_files} $<TARGET_FILE_DIR:${_test_target}>
            COMMENT "Copying shared libraries to test directory")
    endif()
    add_dependencies(${_test_target} nias_cpp_copy_libraries_to_test_folder)
endforeach()
