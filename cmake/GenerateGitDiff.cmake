# Generate git diff for current not-committed changes

# unified diff with 0 lines of context
execute_process(
    COMMAND git diff ${DIFF_TARGET} --unified=0 --output=${diff_file}
    WORKING_DIRECTORY ${working_dir}
    OUTPUT_VARIABLE GIT_DIFF
    ERROR_VARIABLE GIT_DIFF_ERROR RESULTS_VARIABLE GIT_DIFF_RETURN_VALUE)
if(NOT GIT_DIFF_RETURN_VALUE EQUAL 0)
    message(FATAL "Git diff failed: ${GIT_DIFF_ERROR}")
endif()
