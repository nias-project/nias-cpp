#ifndef NIAS_CPP_EXPORT_H
#define NIAS_CPP_EXPORT_H

#ifndef NIAS_CPP_EXPORT
#    ifdef _WIN32
#        ifdef nias_cpp_EXPORTS
#            define NIAS_CPP_EXPORT _declspec(dllexport)
#        else
#            define NIAS_CPP_EXPORT _declspec(dllimport)
#        endif
#    else
#        define NIAS_CPP_EXPORT __attribute__((visibility("default")))
#    endif
#endif

#endif  // NIAS_CPP_EXPORT_H
