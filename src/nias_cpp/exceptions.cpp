#include "exceptions.h"

#include <string_view>

namespace nias
{


ErrorInNiasCpp::ErrorInNiasCpp(std::string_view message)
    : message_(message)
{
}

const char* ErrorInNiasCpp::what() const noexcept
{
    return message_.c_str();
}

InvalidIndexError::InvalidIndexError(std::string_view message)
    : ErrorInNiasCpp(message)
{
}

InvalidStateError::InvalidStateError(std::string_view message)
    : ErrorInNiasCpp(message)
{
}

InvalidArgumentError::InvalidArgumentError(std::string_view message)
    : ErrorInNiasCpp(message)
{
}

OverflowError::OverflowError(std::string_view message)
    : ErrorInNiasCpp(message)
{
}

NotImplementedError::NotImplementedError(std::string_view message)
    : ErrorInNiasCpp(message)
{
}


}  // namespace nias
