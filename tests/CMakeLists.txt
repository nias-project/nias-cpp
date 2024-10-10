add_executable(test_gram_schmidt gram_schmidt.cpp)
add_executable(test_vectorarray vectorarray.cpp)
target_link_libraries(test_gram_schmidt PRIVATE nias_cpp Boost::ut)
target_link_libraries(test_vectorarray PRIVATE nias_cpp Boost::ut)
