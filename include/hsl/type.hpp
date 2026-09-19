#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace hsl {

enum class TypeKind {
    I8, U8, I16, U16, I32, U32, I64, U64, I128, U128, ISize, USize, F32, F64, Bool, Str, Void, Never,
    UserDefined, Array, Slice, Reference, List, Unknown, Error,
};

struct Type {
    TypeKind kind = TypeKind::Unknown;
    std::string name;
    std::vector<Type> arguments;
    std::size_t length = 0;

    Type() = default;
    Type(TypeKind kind_value, std::string name_value);
    Type(
        TypeKind kind_value,
        std::string name_value,
        std::vector<Type> arguments_value,
        std::size_t length_value
    );

    bool operator==(const Type& other) const noexcept;
    bool operator!=(const Type& other) const noexcept;
};

std::string type_name(const Type& type);
Type parse_type_name(std::string_view name);
bool is_numeric_type(const Type& type);
bool is_assignable_type(const Type& target, const Type& source);

}
