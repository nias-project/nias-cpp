#ifndef NIAS_CPP_EXCEPTIONS_H
#define NIAS_CPP_EXCEPTIONS_H

#include <exception>
#include <string>
#include <string_view>

namespace nias
{


/// Base class for all exceptions in the nias_cpp library.
class ErrorInNiasCpp : public std::exception
{
   public:
    explicit ErrorInNiasCpp(std::string_view message)
        : message_(message)
    {
    }

    [[nodiscard]] const char* what() const noexcept override;

   private:
    const std::string message_;
};

// See https://stackoverflow.com/questions/24511376/how-to-dllexport-a-class-derived-from-stdruntime-error
inline const char* ErrorInNiasCpp::what() const noexcept
{
    return message_.c_str();
}

class InvalidIndexError : public ErrorInNiasCpp
{
   public:
    using ErrorInNiasCpp::ErrorInNiasCpp;
};

class InvalidStateError : public ErrorInNiasCpp
{
   public:
    using ErrorInNiasCpp::ErrorInNiasCpp;
};

class InvalidArgumentError : public ErrorInNiasCpp
{
   public:
    using ErrorInNiasCpp::ErrorInNiasCpp;
};

class OverflowError : public ErrorInNiasCpp
{
   public:
    using ErrorInNiasCpp::ErrorInNiasCpp;
};

class NotImplementedError : public ErrorInNiasCpp
{
   public:
    using ErrorInNiasCpp::ErrorInNiasCpp;
};


}  // namespace nias


#endif  // NIASC_CPP_EXCEPTIONS_H
