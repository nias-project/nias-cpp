#ifndef NIAS_CPP_EXPORT_H
#define NIAS_CPP_EXPORT_H

#ifndef NIAS_CPP_EXPORT
#    ifdef NIAS_CPP_STATIC
#        define NIAS_CPP_EXPORT
#    else
#        ifdef _WIN32
#            ifdef nias_cpp_EXPORTS
#                define NIAS_CPP_EXPORT __declspec(dllexport)
#            else
#                define NIAS_CPP_EXPORT __declspec(dllimport)
#            endif
#        else
#            define NIAS_CPP_EXPORT __attribute__((visibility("default")))
#        endif
#    endif
#endif

#endif  // NIAS_CPP_EXPORT_H
