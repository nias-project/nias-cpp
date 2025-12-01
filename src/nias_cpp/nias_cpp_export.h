#ifndef NIAS_CPP_EXPORT_H
#define NIAS_CPP_EXPORT_H

#ifndef NIAS_CPP_EXPORT
#    ifdef _WIN32
#        ifdef NIAS_CPP_STATIC
#            define NIAS_CPP_EXPORT
#            define NIAS_CPP_DLL_LOCAL
#        else
#            ifdef nias_cpp_EXPORTS
#                define NIAS_CPP_EXPORT __declspec(dllexport)
#            else
#                define NIAS_CPP_EXPORT __declspec(dllimport)
#            endif
#            define NIAS_CPP_DLL_LOCAL
#        endif
#    else  // Linux
#        define NIAS_CPP_EXPORT __attribute__((visibility("default")))
#        define NIAS_CPP_DLL_LOCAL __attribute__((visibility("hidden")))
#    endif
#endif

#endif  // NIAS_CPP_EXPORT_H
