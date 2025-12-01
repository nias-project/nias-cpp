#ifndef NIAS_CPP_EXPORT_H
#define NIAS_CPP_EXPORT_H

#ifndef NIAS_CPP_EXPORT
#    ifdef NIAS_CPP_STATIC
#        define NIAS_CPP_EXPORT __attribute__((visibility("default")))
#        define NIAS_CPP_DLL_LOCAL __attribute__((visibility("hidden")))
#    else
#        ifdef _WIN32
#            ifdef nias_cpp_EXPORTS
#                define NIAS_CPP_EXPORT __declspec(dllexport)
#            else
#                define NIAS_CPP_EXPORT __declspec(dllimport)
#            endif
#        else
#            define NIAS_CPP_EXPORT __attribute__((visibility("default")))
#            define NIAS_CPP_DLL_LOCAL __attribute__((visibility("hidden")))
#        endif
#    endif
#endif

#endif  // NIAS_CPP_EXPORT_H
