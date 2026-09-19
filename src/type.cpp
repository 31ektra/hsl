#include "hsl/type.hpp"

#include <charconv>
#include <utility>

namespace hsl {
namespace {

Type named(std::string_view name) {
    if (name == "i8") return {TypeKind::I8, {}, {}, 0};
    if (name == "u8") return {TypeKind::U8, {}, {}, 0};
    if (name == "i16") return {TypeKind::I16, {}, {}, 0};
    if (name == "u16") return {TypeKind::U16, {}, {}, 0};
    if (name == "i32") return {TypeKind::I32, {}, {}, 0};
    if (name == "u32") return {TypeKind::U32, {}, {}, 0};
    if (name == "i64") return {TypeKind::I64, {}, {}, 0};
    if (name == "u64") return {TypeKind::U64, {}, {}, 0};
    if (name == "i128") return {TypeKind::I128, {}, {}, 0};
    if (name == "u128") return {TypeKind::U128, {}, {}, 0};
    if (name == "isize") return {TypeKind::ISize, {}, {}, 0};
    if (name == "usize") return {TypeKind::USize, {}, {}, 0};
    if (name == "f32") return {TypeKind::F32, {}, {}, 0};
    if (name == "f64") return {TypeKind::F64, {}, {}, 0};
    if (name == "bool") return {TypeKind::Bool, {}, {}, 0};
    if (name == "str") return {TypeKind::Str, {}, {}, 0};
    if (name == "never") return {TypeKind::Never, {}, {}, 0};
    if (name.empty() || name == "void") return {TypeKind::Void, {}, {}, 0};
    const auto less = name.find('<');
    if (less != std::string_view::npos && name.ends_with(">")) {
        const std::string base(name.substr(0, less));
        const auto inner = name.substr(less + 1, name.size() - less - 2);
        std::vector<Type> arguments;
        std::size_t start = 0;
        int depth = 0;
        for (std::size_t index = 0; index <= inner.size(); ++index) {
            const char value = index < inner.size() ? inner[index] : ',';
            if (value == '<') ++depth;
            if (value == '>') --depth;
            if (value == ',' && depth == 0) {
                arguments.push_back(parse_type_name(inner.substr(start, index - start)));
                start = index + 1;
            }
        }
        if (base == "List" && arguments.size() == 1) return {TypeKind::List, {}, std::move(arguments), 0};
        return {TypeKind::UserDefined, base, std::move(arguments), 0};
    }
    return {TypeKind::UserDefined, std::string(name), {}, 0};
}

void skip_spaces(std::string_view text, std::size_t& position) {
    while (position < text.size() && text[position] == ' ') ++position;
}

Type parse(std::string_view text, std::size_t& position) {
    skip_spaces(text, position);
    if (position >= text.size()) return {TypeKind::Unknown, {}, {}, 0};

    if (text[position] == '&') {
        ++position;
        Type referenced = parse(text, position);
        return {
            TypeKind::Reference,
            {},
            {std::move(referenced)},
            0
        };
    }

    if (text[position] != '[') {
        const std::size_t start = position;
        while (position < text.size() && text[position] != ';' && text[position] != ']') ++position;
        std::size_t end = position;
        while (end > start && text[end - 1] == ' ') --end;
        return named(text.substr(start, end - start));
    }

    ++position;
    Type element = parse(text, position);
    skip_spaces(text, position);

    if (position < text.size() && text[position] == ']') {
        ++position;
        return {TypeKind::Slice, {}, {std::move(element)}, 0};
    }

    if (position >= text.size() || text[position] != ';') return {TypeKind::Unknown, {}, {}, 0};
    ++position;
    skip_spaces(text, position);

    const std::size_t start = position;
    while (position < text.size() && text[position] >= '0' && text[position] <= '9') ++position;
    if (start == position) return {TypeKind::Unknown, {}, {}, 0};

    std::size_t length = 0;
    const auto result = std::from_chars(text.data() + start, text.data() + position, length);
    if (result.ec != std::errc{}) return {TypeKind::Unknown, {}, {}, 0};

    skip_spaces(text, position);
    if (position >= text.size() || text[position] != ']') return {TypeKind::Unknown, {}, {}, 0};
    ++position;
    return {TypeKind::Array, {}, {std::move(element)}, length};
}

}

Type::Type(TypeKind kind_value, std::string name_value)
    : kind(kind_value),
      name(std::move(name_value)) {}

Type::Type(
    TypeKind kind_value,
    std::string name_value,
    std::vector<Type> arguments_value,
    std::size_t length_value
)
    : kind(kind_value),
      name(std::move(name_value)),
      arguments(std::move(arguments_value)),
      length(length_value) {}

bool Type::operator==(const Type& other) const noexcept {
    return kind == other.kind && name == other.name &&
           arguments == other.arguments && length == other.length;
}

bool Type::operator!=(const Type& other) const noexcept { return !(*this == other); }

std::string type_name(const Type& type) {
    switch (type.kind) {
        case TypeKind::I8: return "i8";
        case TypeKind::U8: return "u8";
        case TypeKind::I16: return "i16";
        case TypeKind::U16: return "u16";
        case TypeKind::I32: return "i32";
        case TypeKind::U32: return "u32";
        case TypeKind::I64: return "i64";
        case TypeKind::U64: return "u64";
        case TypeKind::I128: return "i128";
        case TypeKind::U128: return "u128";
        case TypeKind::ISize: return "isize";
        case TypeKind::USize: return "usize";
        case TypeKind::F32: return "f32";
        case TypeKind::F64: return "f64";
        case TypeKind::Bool: return "bool";
        case TypeKind::Str: return "str";
        case TypeKind::Void: return "void";
        case TypeKind::Never: return "never";
        case TypeKind::UserDefined: {
            std::string result = type.name;
            if (!type.arguments.empty()) {
                result += '<';
                for (std::size_t index = 0; index < type.arguments.size(); ++index) {
                    if (index) result += ',';
                    result += type_name(type.arguments[index]);
                }
                result += '>';
            }
            return result;
        }
        case TypeKind::Array:
            if (type.arguments.size() != 1) return "unknown";
            return "[" + type_name(type.arguments.front()) + "; " + std::to_string(type.length) + "]";
        case TypeKind::Slice:
            if (type.arguments.size() != 1) return "unknown";
            return "[" + type_name(type.arguments.front()) + "]";
        case TypeKind::Reference:
            if (type.arguments.size() != 1) return "unknown";
            return "&" + type_name(type.arguments.front());
        case TypeKind::List:
            if (type.arguments.size() != 1) return "unknown";
            return "List<" + type_name(type.arguments.front()) + ">";
        case TypeKind::Unknown: return "unknown";
        case TypeKind::Error: return "error";
    }
    return "error";
}

Type parse_type_name(std::string_view text) {
    std::size_t position = 0;
    Type result = parse(text, position);
    skip_spaces(text, position);
    return position == text.size() ? result : Type{TypeKind::Unknown, {}, {}, 0};
}

bool is_numeric_type(const Type& type) {
    return type.kind == TypeKind::I8 || type.kind == TypeKind::U8 ||
           type.kind == TypeKind::I16 || type.kind == TypeKind::U16 ||
           type.kind == TypeKind::I32 || type.kind == TypeKind::U32 ||
           type.kind == TypeKind::I64 || type.kind == TypeKind::U64 ||
           type.kind == TypeKind::I128 || type.kind == TypeKind::U128 ||
           type.kind == TypeKind::ISize || type.kind == TypeKind::USize ||
           type.kind == TypeKind::F32 || type.kind == TypeKind::F64;
}

bool is_assignable_type(const Type& target, const Type& source) {
    if (target.kind == TypeKind::Error || source.kind == TypeKind::Error) return true;
    if (target.kind == TypeKind::Unknown || source.kind == TypeKind::Unknown) return true;
    return target == source;
}

}
