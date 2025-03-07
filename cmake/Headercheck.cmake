macro(_add_headercheck_target header)
    # add target
    set_source_files_properties(${header} PROPERTIES HEADER_FILE_ONLY 1)
    string(MAKE_C_IDENTIFIER ${header} header_cname)

    set(generated_source_file ${CMAKE_CURRENT_BINARY_DIR}/hc/${header_cname}.cpp)
    file(
        WRITE ${generated_source_file}
        "
        #include <${header}> // IWYU pragma: keep
        // NOLINTNEXTLINE(readability-duplicate-include)
        #include <${header}> // IWYU pragma: keep


        int main()
        {
        return 0;
        }
        ")
    set(target_name headercheck_${header_cname})

    # add target
    add_library(${target_name} EXCLUDE_FROM_ALL ${generated_source_file})
    add_dependencies(headercheck ${target_name})

    # link current module's library and libraries provided by the user
    target_link_libraries(${target_name} PRIVATE nias_cpp::nias_cpp)
    target_link_libraries(${target_name} PRIVATE ${arg_ADDITIONAL_LIBRARIES})

    # add current module's include directories
    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_include_directories(${target_name} PRIVATE ${PROJECT_BINARY_DIR})
    target_include_directories(${target_name} PRIVATE ${PROJECT_SOURCE_DIR})
endmacro()

function(nias_cpp_add_headercheck)
    # parse arguments
    # ADDITIONAL_LIBRARIES is a list of libraries that should be linked to the headercheck binaries
    # We always link the nias_cpp library
    set(multiValueArgs ADDITIONAL_LIBRARIES)
    cmake_parse_arguments(arg "" "" "${multiValueArgs}" "${ARGN}")
    if(arg_ADDITIONAL_LIBRARIES IN_LIST arg_KEYWORDS_MISSING_VALUES)
        message(
            FATAL_ERROR
                "[nias_cpp_add_headercheck]: ADDITIONAL_LIBRARIES argument requires at least one value")
    endif()
    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "[nias_cpp_add_headercheck]: There were unparsed arguments!")
    endif()

    # find headers in (subfolders of) current directory
    file(
        GLOB_RECURSE headers
        LIST_DIRECTORIES false
        RELATIVE "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/src/*.h")

    if(NOT TARGET headercheck)
        add_custom_target(headercheck)
    endif()

    foreach(header ${headers})
        _add_headercheck_target(${header})
    endforeach()
endfunction()
