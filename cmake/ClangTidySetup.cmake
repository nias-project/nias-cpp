function(add_dummy_targets_tidy)
    set(singleValueArgs MESSAGE)
    set(multiValueArgs TARGETS)
    cmake_parse_arguments(TIDY "" "${singleValueArgs}" "${multiValueArgs}" "${ARGN}")

    foreach(target ${TIDY_TARGETS})
        add_custom_target(
            ${PROJECT_NAME}_${target}
            COMMAND echo "${TIDY_MESSAGE}" && exit 1
            COMMENT "Dummy target to indicate that clang-tidy is not available")

        if(NOT TARGET ${target})
            add_custom_target(${target})
        endif()

        add_dependencies(${target} ${PROJECT_NAME}_${target})
    endforeach()
endfunction()

macro(_find_tidy)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        # clang-tidy needs a compile_commands.json file and that file is not created by MSVC
        set(message "clang-tidy targets are not supported for MSVC presets! Dummy targets got added.")
        add_dummy_targets_tidy(TARGETS ${tidy_targets} MESSAGE ${message})
        message(WARNING ${message})
        return()
    endif()
    find_program(
        CLANG_TIDY_BIN
        NAMES clang-tidy clang-tidy_ci clang-tidy-17
        DOC "Path to clang-tidy executable")

    # we need to execute the tidy driver .py with python
    find_package(Python3 COMPONENTS Interpreter)

    if(NOT Python3_FOUND OR NOT CLANG_TIDY_BIN)
        set(message "Python3 or clang-tidy not found! Dummy targets got added.")
        add_dummy_targets_tidy(TARGETS ${tidy_targets} MESSAGE ${message})
        message(WARNING ${message})
        return()
    endif()

    # dummy target
    add_custom_target(install_python_${PROJECT_NAME})

    set(tidy_python ${Python3_EXECUTABLE})

    # find optional clang-apply-replacements
    find_program(
        CLANG_APPLY_REPLACEMENTS_BIN
        NAMES clang-apply-replacements clang-apply-replacements_ci clang-apply-replacements-17
              clang-apply-replacements-18 clang-apply-replacements-19 clang-apply-replacements-20
        DOC "Path to clang-apply-replacements executable")
endmacro()

function(inspect_with_clang_tidy)
    set(tidy_targets clang-tidy-fix clang-tidy-report clang-tidy-fix-diff-head clang-tidy-report-diff-head
                     clang-tidy-fix-diff-main clang-tidy-report-diff-main)

    _find_tidy()

    set(replacements_file ${PROJECT_BINARY_DIR}/tidy-fixes.yaml)
    set(files_regex ".*\\.(c|cc|cpp|cxx|h|hh|hpp|hxx)$")
    set(source_dir)

    foreach(arg IN LISTS ARGN)
        set(source_dir -source-dir ${CMAKE_CURRENT_SOURCE_DIR}/${arg} ${source_dir})
    endforeach()

    # Add a custom target for clang-tidy
    cmake_host_system_information(RESULT parallel_jobs_count QUERY NUMBER_OF_PHYSICAL_CORES)

    set(cmake_dir ${PROJECT_SOURCE_DIR}/cmake)
    set(tidy_driver
        ${cmake_dir}/clang-tidy-diff.py
        -export-fixes
        ${replacements_file}
        -clang-tidy-binary
        ${CLANG_TIDY_BIN}
        -path
        ${PROJECT_BINARY_DIR}
        -j
        ${parallel_jobs_count}
        -p
        1
        -regex
        ${files_regex})

    # pass current git diff to clang-tidy
    set(diff_file_head ${PROJECT_BINARY_DIR}/clang-tidy-diff-HEAD.patch)
    add_custom_target(
        ${PROJECT_NAME}_clang-tidy-generate-diff-head
        COMMAND ${CMAKE_COMMAND} -Ddiff_file=${diff_file_head} -Dworking_dir=${PROJECT_SOURCE_DIR}
                -DDIFF_TARGET=HEAD -P ${cmake_dir}/GenerateGitDiff.cmake
        VERBATIM
        COMMENT "Generating git diff to HEAD for clang-tidy")
    set_property(TARGET ${PROJECT_NAME}_clang-tidy-generate-diff-head PROPERTY FOLDER_ALWAYS_OUT_OF_DATE 1)
    set(diff_file_main ${PROJECT_BINARY_DIR}/clang-tidy-diff-MAIN.patch)
    add_custom_target(
        ${PROJECT_NAME}_clang-tidy-generate-diff-main
        COMMAND ${CMAKE_COMMAND} -Ddiff_file=${diff_file_main} -Dworking_dir=${PROJECT_SOURCE_DIR}
                -DDIFF_TARGET=origin/main -P ${cmake_dir}/GenerateGitDiff.cmake
        VERBATIM
        COMMENT "Generating git diff to origin/main for clang-tidy")
    set_property(TARGET ${PROJECT_NAME}_clang-tidy-generate-diff-main PROPERTY FOLDER_ALWAYS_OUT_OF_DATE 1)

    _add_tidy_targets()

    foreach(tidy_target ${tidy_targets})
        if(NOT TARGET ${tidy_target})
            add_custom_target(${tidy_target})
        endif()

        add_dependencies(${tidy_target} ${PROJECT_NAME}_${tidy_target})
    endforeach()
endfunction()

macro(_add_tidy_targets)
    # the interpreter genex needs to be it's own string, else it does not get evaluated
    # We first create the report-only targets that do not apply fixes
    # "plain tidy"
    add_custom_target(
        ${PROJECT_NAME}_clang-tidy-report
        COMMAND "${tidy_python}" ${tidy_driver} ${source_dir}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
        DEPENDS install_python_${PROJECT_NAME}
        COMMENT "Running clang-tidy")
    # "tidy on diff to HEAD"
    add_custom_target(
        ${PROJECT_NAME}_clang-tidy-report-diff-head
        COMMAND "${tidy_python}" ${tidy_driver} -diff-file ${diff_file_head}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
        DEPENDS install_python_${PROJECT_NAME}
        COMMENT "Running clang-tidy on the diff to HEAD")
    add_dependencies(${PROJECT_NAME}_clang-tidy-report-diff-head
                     ${PROJECT_NAME}_clang-tidy-generate-diff-head)
    # "tidy on diff to origin/main"
    add_custom_target(
        ${PROJECT_NAME}_clang-tidy-report-diff-main
        COMMAND "${tidy_python}" ${tidy_driver} -diff-file ${diff_file_main}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
        DEPENDS install_python_${PROJECT_NAME}
        COMMENT "Running clang-tidy on the diff to origin/main")
    add_dependencies(${PROJECT_NAME}_clang-tidy-report-diff-main
                     ${PROJECT_NAME}_clang-tidy-generate-diff-main)

    # For the targets that apply fixes, we would like to use clang-apply-replacements if available,
    # but we also want to offer a fallback if it is not available.
    # The fallback is using the clang-tidy-diff.py script with the -fix option, which works fine
    # but sometimes applies the same fix multiple times in parallel, which can lead to fixes like
    # int some_function() override over overrid ridee
    if(CLANG_APPLY_REPLACEMENTS_BIN)
        set(CLANG_APPLY_REPLACEMENTS_FLAGS --format --style=file --style-config=${PROJECT_SOURCE_DIR}
                                           --remove-change-desc-files)
        # "plain tidy"
        # clang-tidy-report returns a non-zero error code if it reports any issues, so
        # clang-apply-replacements will never be executed if it depends on clang-tidy-report.
        # We thus add a report target that always succeeds.
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-report-always-succeed
            COMMAND "${tidy_python}" ${tidy_driver} ${source_dir} || echo "clang-tidy found issues"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Running clang-tidy to create tidy-fixes.yaml file")
        add_custom_target(
            ${PROJECT_NAME}_clang-apply-replacements-all
            COMMAND ${CLANG_APPLY_REPLACEMENTS_BIN} ${PROJECT_BINARY_DIR} ${CLANG_APPLY_REPLACEMENTS_FLAGS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Applying clang-tidy fixes using clang-apply-replacements")
        add_dependencies(${PROJECT_NAME}_clang-apply-replacements-all
                         ${PROJECT_NAME}_clang-tidy-report-always-succeed)
        add_custom_target(${PROJECT_NAME}_clang-tidy-fix DEPENDS ${PROJECT_NAME}_clang-apply-replacements-all)

        # "tidy on diff to HEAD"
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-report-diff-head-always-succeed
            COMMAND "${tidy_python}" ${tidy_driver} -diff-file ${diff_file_head} || echo
                    "clang-tidy found issues"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Running clang-tidy on diff to HEAD to create tidy-fixes.yaml file")
        add_dependencies(${PROJECT_NAME}_clang-tidy-report-diff-head-always-succeed
                         ${PROJECT_NAME}_clang-tidy-generate-diff-head)
        add_custom_target(
            ${PROJECT_NAME}_clang-apply-replacements-diff-head
            COMMAND ${CLANG_APPLY_REPLACEMENTS_BIN} ${PROJECT_BINARY_DIR} ${CLANG_APPLY_REPLACEMENTS_FLAGS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Applying clang-tidy fixes using clang-apply-replacements")
        add_dependencies(${PROJECT_NAME}_clang-apply-replacements-diff-head
                         ${PROJECT_NAME}_clang-tidy-report-diff-head-always-succeed)
        add_custom_target(${PROJECT_NAME}_clang-tidy-fix-diff-head
                          DEPENDS ${PROJECT_NAME}_clang-apply-replacements-diff-head)

        # "tidy on diff to origin/main"
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-report-diff-main-always-succeed
            COMMAND "${tidy_python}" ${tidy_driver} -diff-file ${diff_file_main} || echo
                    "clang-tidy found issues"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Running clang-tidy on diff to origin/main to create tidy-fixes.yaml file")
        add_dependencies(${PROJECT_NAME}_clang-tidy-report-diff-main-always-succeed
                         ${PROJECT_NAME}_clang-tidy-generate-diff-main)
        add_custom_target(
            ${PROJECT_NAME}_clang-apply-replacements-diff-main
            COMMAND ${CLANG_APPLY_REPLACEMENTS_BIN} ${PROJECT_BINARY_DIR} ${CLANG_APPLY_REPLACEMENTS_FLAGS}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Applying clang-tidy fixes using clang-apply-replacements")
        add_dependencies(${PROJECT_NAME}_clang-apply-replacements-diff-main
                         ${PROJECT_NAME}_clang-tidy-report-diff-main-always-succeed)
        add_custom_target(${PROJECT_NAME}_clang-tidy-fix-diff-main
                          DEPENDS ${PROJECT_NAME}_clang-apply-replacements-diff-main)
    else()
        set(note_parallel_fixes_part_1
            "clang-apply-replacements was not found. Fixes will be applied in parallel w/o synchronization.")
        set(note_parallel_fixes_part_2
            "If you encounter issues, install clang-apply-replacements and reconfigure.")
        set(note_parallel_fixes "Note: ${note_parallel_fixes_part_1} ${note_parallel_fixes_part_2}")
        # "plain tidy"
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-fix
            COMMAND "${tidy_python}" ${tidy_driver} ${source_dir} -fix
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Running clang-tidy --fix. ${note_parallel_fixes}")
        # "tidy on diff to HEAD"
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-fix-diff-head
            COMMAND "${tidy_python}" ${tidy_driver} -fix -diff-file ${diff_file_head}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "\
        Running clang-tidy --fix on the diff to HEAD. ${note_parallel_fixes}")
        add_dependencies(${PROJECT_NAME}_clang-tidy-fix-diff-head
                         ${PROJECT_NAME}_clang-tidy-generate-diff-head)
        # "tidy on diff to origin/main"
        add_custom_target(
            ${PROJECT_NAME}_clang-tidy-fix-diff-main
            COMMAND "${tidy_python}" ${tidy_driver} -fix -diff-file ${diff_file_main}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            VERBATIM
            DEPENDS install_python_${PROJECT_NAME}
            COMMENT "Running clang-tidy --fix on the diff to origin/main. ${note_parallel_fixes}")
        add_dependencies(${PROJECT_NAME}_clang-tidy-fix-diff-main
                         ${PROJECT_NAME}_clang-tidy-generate-diff-main)
        # dummy targets to avoid errors due to non-existing targets
        add_custom_target(${PROJECT_NAME}_clang-tidy-report-always-succeed)
        add_custom_target(${PROJECT_NAME}_clang-tidy-report-diff-head-always-succeed)
        add_custom_target(${PROJECT_NAME}_clang-tidy-report-diff-main-always-succeed)
    endif()
endmacro()
