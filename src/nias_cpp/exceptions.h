#ifndef NIAS_CPP_EXCEPTIONS_H
#define NIAS_CPP_EXCEPTIONS_H

#include <exception>
#include <string>
#include <string_view>

#include "nias_cpp_export.h"

namespace nias
{


/// Base class for all exceptions in the nias_cpp library.
class NIAS_CPP_EXPORT ErrorInNiasCpp : public std::exception
{
   public:
    explicit ErrorInNiasCpp(std::string_view message);

    [[nodiscard]] const char* what() const noexcept override;

   private:
    const std::string message_;
};

class NIAS_CPP_EXPORT InvalidIndexError : public ErrorInNiasCpp
{
   public:
    explicit InvalidIndexError(std::string_view message);
};

class NIAS_CPP_EXPORT InvalidStateError : public ErrorInNiasCpp
{
   public:
    explicit InvalidStateError(std::string_view message);
};

class NIAS_CPP_EXPORT InvalidArgumentError : public ErrorInNiasCpp
{
   public:
    explicit InvalidArgumentError(std::string_view message);
};

class NIAS_CPP_EXPORT OverflowError : public ErrorInNiasCpp
{
   public:
    explicit OverflowError(std::string_view message);
};

class NIAS_CPP_EXPORT NotImplementedError : public ErrorInNiasCpp
{
   public:
    explicit NotImplementedError(std::string_view message);
};


}  // namespace nias


#endif  // NIASC_CPP_EXCEPTIONS_H
