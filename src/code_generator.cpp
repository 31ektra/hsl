#include "hsl/code_generator.hpp"

#include "hsl/type.hpp"

#include <cerrno>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <limits>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <string_view>
#include <unordered_map>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <variant>
#include <map>
#include <set>
#include <optional>

namespace hsl {
namespace {

std::string_view name_part(std::string_view label) {
    const auto separator = label.find(':');
    return label.substr(0, separator);
}

std::string_view type_part(std::string_view label) {
    const auto separator = label.find(':');
    return separator == std::string_view::npos
        ? std::string_view{} : label.substr(separator + 1);
}

std::string cpp_type(const Type& type) {
    switch (type.kind) {
        case TypeKind::I8: return "std::int8_t";
        case TypeKind::U8: return "std::uint8_t";
        case TypeKind::I16: return "std::int16_t";
        case TypeKind::U16: return "std::uint16_t";
        case TypeKind::I32: return "std::int32_t";
        case TypeKind::U32: return "std::uint32_t";
        case TypeKind::I64: return "std::int64_t";
        case TypeKind::U64: return "std::uint64_t";
        case TypeKind::I128: return "__int128_t";
        case TypeKind::U128: return "__uint128_t";
        case TypeKind::ISize: return "std::intptr_t";
        case TypeKind::USize: return "std::uintptr_t";
        case TypeKind::F32: return "float";
        case TypeKind::F64: return "double";
        case TypeKind::Bool: return "bool";
        case TypeKind::Str: return "std::string";
        case TypeKind::Void: return "void";
        case TypeKind::Never: return "void";
        case TypeKind::UserDefined:
            if (type.name == "Option" && type.arguments.size() == 1) return "hsl_option<" + cpp_type(type.arguments[0]) + ">";
            if (type.name == "Result" && type.arguments.size() == 2) return "hsl_result<" + cpp_type(type.arguments[0]) + ", " + cpp_type(type.arguments[1]) + ">";
            if (type.name == "Map" && type.arguments.size() == 2) return "hsl_map<" + cpp_type(type.arguments[0]) + ", " + cpp_type(type.arguments[1]) + ">";
            if (type.name == "Set" && type.arguments.size() == 1) return "hsl_set<" + cpp_type(type.arguments[0]) + ">";
            return type.name;
        case TypeKind::Array:
            if (type.arguments.size() != 1) return "void";
            return "std::array<" +
                cpp_type(type.arguments.front()) +
                ", " + std::to_string(type.length) + ">";
        case TypeKind::Slice:
            if (type.arguments.size() != 1) return "void";
            return "std::span<const " +
                cpp_type(type.arguments.front()) + ">";
        case TypeKind::Reference:
            if (type.arguments.size() != 1) return "void";
            return "const " + cpp_type(type.arguments.front()) + "&";
        case TypeKind::List:
            if (type.arguments.size() != 1) return "void";
            return "hsl_list<" +
                cpp_type(type.arguments.front()) + ">";
        case TypeKind::Unknown:
        case TypeKind::Error:
            return "void";
    }

    return "void";
}

std::string cpp_type(std::string_view spelling) {
    return cpp_type(parse_type_name(spelling));
}

std::string operator_text(std::string_view operation) {
    if (operation == "Equal") return "=";
    if (operation == "EqualEqual") return "==";
    if (operation == "BangEqual") return "!=";
    if (operation == "Plus") return "+";
    if (operation == "Minus") return "-";
    if (operation == "Star") return "*";
    if (operation == "Slash") return "/";
    if (operation == "Percent") return "%";
    if (operation == "Less") return "<";
    if (operation == "LessEqual") return "<=";
    if (operation == "Greater") return ">";
    if (operation == "GreaterEqual") return ">=";
    if (operation == "AmpAmp") return "&&";
    if (operation == "PipePipe") return "||";
    if (operation == "Amp") return "&";
    if (operation == "Pipe") return "|";
    if (operation == "Caret") return "^";
    if (operation == "ShiftLeft") return "<<";
    if (operation == "ShiftRight") return ">>";
    if (operation == "PlusEqual") return "+=";
    if (operation == "MinusEqual") return "-=";
    if (operation == "StarEqual") return "*=";
    if (operation == "SlashEqual") return "/=";
    if (operation == "PercentEqual") return "%=";
    if (operation == "AmpEqual") return "&=";
    if (operation == "PipeEqual") return "|=";
    if (operation == "CaretEqual") return "^=";
    if (operation == "ShiftLeftEqual") return "<<=";
    if (operation == "ShiftRightEqual") return ">>=";
    return "";
}

std::string numeric_text(std::string value) {
    for (auto position = value.find('_'); position != std::string::npos;
         position = value.find('_')) {
        value.erase(position, 1);
    }
    if (value.ends_with("f32") || value.ends_with("f64")) value.resize(value.size() - 3);
    return value;
}

class Emitter {
public:
    std::string emit(const AstNode& root) {
        for (const auto& child : root.children) {
            if (child->kind != AstKind::EnumDeclaration) continue;
            bool payloads = false;
            for (const auto& variant : child->children) payloads = payloads || !type_part(variant->value).empty();
            enum_payloads_[child->value] = payloads;
            auto& variants = enum_variants_[child->value];
            for (const auto& variant : child->children) variants[std::string(name_part(variant->value))] = std::string(type_part(variant->value));
        }
        output_ << R"HSLCPP(#include <array>
#include <chrono>
#include <thread>
#include <limits>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <map>
#include <set>
#include <optional>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

std::int64_t hsl_process_run(const std::string& command) {
    return static_cast<std::int64_t>(std::system(command.c_str()));
}

std::string hsl_current_directory() {
    std::error_code error;
    const auto path = std::filesystem::current_path(error);
    if (error) { std::cerr << "HSL runtime error: cannot read current directory\n"; std::exit(1); }
    return path.string();
}

std::int64_t hsl_process_run_program(const std::string& executable) {
    const pid_t child = fork();
    if (child < 0) return -1;
    if (child == 0) {
        execlp(executable.c_str(), executable.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

bool hsl_fs_create_directory(const std::string& path) {
    std::error_code error;
    return std::filesystem::create_directory(path, error) || (!error && std::filesystem::is_directory(path));
}

bool hsl_fs_create_directories(const std::string& path) {
    std::error_code error;
    return std::filesystem::create_directories(path, error) || (!error && std::filesystem::is_directory(path));
}

bool hsl_fs_remove(const std::string& path) {
    std::error_code error;
    return std::filesystem::remove(path, error) && !error;
}

bool hsl_fs_rename(const std::string& from, const std::string& to) {
    std::error_code error;
    std::filesystem::rename(from, to, error);
    return !error;
}

bool hsl_fs_is_file(const std::string& path) {
    std::error_code error;
    return std::filesystem::is_regular_file(path, error) && !error;
}

bool hsl_fs_is_directory(const std::string& path) {
    std::error_code error;
    return std::filesystem::is_directory(path, error) && !error;
}

std::int64_t hsl_process_run_arg(const std::string& executable, const std::string& argument) {
    const pid_t child = fork();
    if (child < 0) return -1;
    if (child == 0) {
        execlp(executable.c_str(), executable.c_str(), argument.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    int status = 0;
    if (waitpid(child, &status, 0) < 0) return -1;
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
    return -1;
}

bool hsl_fs_copy(const std::string& from, const std::string& to) {
    std::error_code error;
    return std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, error) && !error;
}

std::int64_t hsl_fs_remove_all(const std::string& path) {
    std::error_code error;
    const auto removed = std::filesystem::remove_all(path, error);
    return error ? -1 : static_cast<std::int64_t>(removed);
}

std::int64_t hsl_fs_file_size(const std::string& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > static_cast<std::uintmax_t>(std::numeric_limits<std::int64_t>::max())) return -1;
    return static_cast<std::int64_t>(size);
}

std::int64_t hsl_time_unix_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

void hsl_sleep_ms(std::int64_t milliseconds) {
    if (milliseconds > 0) std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

std::string hsl_read_line() {
    std::string value;
    std::getline(std::cin, value);
    return value;
}

std::int64_t hsl_program_argc = 0;
char** hsl_program_argv = nullptr;

std::int64_t hsl_arg_count() { return hsl_program_argc; }

std::string hsl_arg(std::int64_t index) {
    if (index < 0 || index >= hsl_program_argc || hsl_program_argv == nullptr) {
        std::cerr << "HSL runtime error: argument index out of bounds\n";
        std::exit(1);
    }
    return hsl_program_argv[index];
}

bool hsl_set_current_directory(const std::string& path) {
    std::error_code error;
    std::filesystem::current_path(path, error);
    return !error;
}

bool hsl_env_set(const std::string& name, const std::string& value) {
    return setenv(name.c_str(), value.c_str(), 1) == 0;
}

bool hsl_env_unset(const std::string& name) {
    return unsetenv(name.c_str()) == 0;
}

std::int64_t hsl_utf8_length(const std::string& value) {
    std::int64_t count = 0;
    for (unsigned char byte : value) if ((byte & 0xc0u) != 0x80u) ++count;
    return count;
}

std::int64_t hsl_utf8_scalar_at(const std::string& value, std::int64_t wanted) {
    if (wanted < 0) { std::cerr << "HSL runtime error: UTF-8 index out of bounds\n"; std::exit(1); }
    std::int64_t current = 0;
    for (std::size_t offset = 0; offset < value.size();) {
        const unsigned char first = static_cast<unsigned char>(value[offset]);
        std::size_t width = first < 0x80 ? 1 : (first < 0xe0 ? 2 : (first < 0xf0 ? 3 : 4));
        if (offset + width > value.size() || (first >= 0x80 && first < 0xc2) || first > 0xf4) {
            std::cerr << "HSL runtime error: invalid UTF-8\n"; std::exit(1);
        }
        std::uint32_t scalar = first & (width == 1 ? 0x7fu : (width == 2 ? 0x1fu : (width == 3 ? 0x0fu : 0x07u)));
        for (std::size_t i = 1; i < width; ++i) {
            const unsigned char next = static_cast<unsigned char>(value[offset + i]);
            if ((next & 0xc0u) != 0x80u) { std::cerr << "HSL runtime error: invalid UTF-8\n"; std::exit(1); }
            scalar = (scalar << 6) | (next & 0x3fu);
        }
        if (current++ == wanted) return static_cast<std::int64_t>(scalar);
        offset += width;
    }
    std::cerr << "HSL runtime error: UTF-8 index out of bounds\n"; std::exit(1);
}

std::int64_t hsl_byte_length(const std::string& value) { return static_cast<std::int64_t>(value.size()); }

std::string hsl_i64_to_string(std::int64_t value) { return std::to_string(value); }

bool hsl_fs_exists(const std::string& path) {
    std::error_code error;
    return std::filesystem::exists(path, error) && !error;
}

std::string hsl_fs_read(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "HSL runtime error: cannot read file: " << path << '\n';
        std::exit(1);
    }
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

bool hsl_fs_write(const std::string& path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    output << value;
    return static_cast<bool>(output);
}

std::string hsl_path_join(const std::string& left, const std::string& right) {
    return (std::filesystem::path(left) / right).lexically_normal().string();
}

bool hsl_path_is_absolute(const std::string& path) {
    return std::filesystem::path(path).is_absolute();
}

std::string hsl_path_parent(const std::string& path) {
    return std::filesystem::path(path).parent_path().string();
}

std::string hsl_path_filename(const std::string& path) {
    return std::filesystem::path(path).filename().string();
}

std::string hsl_path_extension(const std::string& path) {
    return std::filesystem::path(path).extension().string();
}

std::string hsl_path_normalize(const std::string& path) {
    return std::filesystem::path(path).lexically_normal().string();
}

bool hsl_string_starts_with(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool hsl_string_ends_with(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() && value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool hsl_string_contains(const std::string& value, const std::string& needle) {
    return value.find(needle) != std::string::npos;
}

std::int64_t hsl_string_find(const std::string& value, const std::string& needle) {
    const auto position = value.find(needle);
    return position == std::string::npos ? -1 : static_cast<std::int64_t>(position);
}

std::int64_t hsl_string_byte_at(const std::string& value, std::int64_t index) {
    if (index < 0 || static_cast<std::size_t>(index) >= value.size()) {
        std::cerr << "HSL runtime error: string index out of bounds\n";
        std::exit(1);
    }
    return static_cast<unsigned char>(value[static_cast<std::size_t>(index)]);
}

std::string hsl_string_slice(const std::string& value, std::int64_t start, std::int64_t end) {
    if (start < 0 || end < start || static_cast<std::size_t>(end) > value.size()) {
        std::cerr << "HSL runtime error: string slice out of bounds\n";
        std::exit(1);
    }
    return value.substr(static_cast<std::size_t>(start), static_cast<std::size_t>(end - start));
}

template <typename T>
struct hsl_option {
    std::optional<T> value;
    static hsl_option Some(T value) { return {std::move(value)}; }
    static hsl_option None() { return {std::nullopt}; }
    bool is_some() const { return value.has_value(); }
    bool is_none() const { return !value.has_value(); }
    const T& unwrap() const {
        if (!value) { std::cerr << "HSL runtime error: unwrapped None\n"; std::exit(1); }
        return *value;
    }
    T unwrap_or(T fallback) const { return value ? *value : std::move(fallback); }
};

hsl_option<std::string> hsl_env_get(const std::string& name) {
    const char* value = std::getenv(name.c_str());
    return value ? hsl_option<std::string>::Some(value) : hsl_option<std::string>::None();
}

template <typename T, typename E>
struct hsl_result {
    std::variant<T, E> value;
    bool ok;
    static hsl_result Ok(T value) { return {std::variant<T,E>(std::in_place_index<0>, std::move(value)), true}; }
    static hsl_result Err(E value) { return {std::variant<T,E>(std::in_place_index<1>, std::move(value)), false}; }
    bool is_ok() const { return ok; }
    bool is_err() const { return !ok; }
    const T& unwrap() const {
        if (!ok) { std::cerr << "HSL runtime error: unwrapped Err\n"; std::exit(1); }
        return std::get<0>(value);
    }
    const E& unwrap_err() const {
        if (ok) { std::cerr << "HSL runtime error: unwrapped Ok as error\n"; std::exit(1); }
        return std::get<1>(value);
    }
};

hsl_option<std::int64_t> hsl_try_parse_i64(const std::string& value) {
    try {
        std::size_t used = 0;
        const auto parsed = std::stoll(value, &used, 10);
        if (used != value.size()) return hsl_option<std::int64_t>::None();
        return hsl_option<std::int64_t>::Some(parsed);
    } catch (...) {
        return hsl_option<std::int64_t>::None();
    }
}

template <typename K, typename V>
struct hsl_map {
    std::map<K,V> values;
    void insert(K key, V value) { values.insert_or_assign(std::move(key), std::move(value)); }
    bool contains(const K& key) const { return values.contains(key); }
    hsl_option<V> get(const K& key) const {
        const auto found = values.find(key);
        return found == values.end() ? hsl_option<V>::None() : hsl_option<V>::Some(found->second);
    }
    bool remove(const K& key) { return values.erase(key) != 0; }
    void clear() { values.clear(); }
    std::uint64_t length() const { return values.size(); }
};

template <typename T>
struct hsl_set {
    std::set<T> values;
    void insert(T value) { values.insert(std::move(value)); }
    bool contains(const T& value) const { return values.contains(value); }
    bool remove(const T& value) { return values.erase(value) != 0; }
    void clear() { values.clear(); }
    std::uint64_t length() const { return values.size(); }
};

template <typename T>
struct hsl_list {
    std::vector<T> values;

    hsl_list() = default;
    hsl_list(const hsl_list&) = delete;
    hsl_list& operator=(const hsl_list&) = delete;
    hsl_list(hsl_list&&) noexcept = default;
    hsl_list& operator=(hsl_list&&) noexcept = default;
    ~hsl_list() = default;

    void push(T value) {
        values.push_back(std::move(value));
    }

    T pop() {
        if (values.empty()) {
            std::cerr
                << "HSL runtime error: pop from empty list\n";
            std::exit(1);
        }

        T value = std::move(values.back());
        values.pop_back();
        return value;
    }

    void clear() {
        values.clear();
    }

    std::uint64_t length() const {
        return values.size();
    }

    std::uint64_t capacity() const {
        return values.capacity();
    }

    T& operator[] (std::size_t index) {
        if (index >= values.size()) {
            std::cerr
                << "HSL runtime error: list index out of bounds\n";
            std::exit(1);
        }
        return values[index];
    }

    const T& operator[] (std::size_t index) const {
        if (index >= values.size()) {
            std::cerr
                << "HSL runtime error: list index out of bounds\n";
            std::exit(1);
        }
        return values[index];
    }

    auto begin() { return values.begin(); }
    auto end() { return values.end(); }
    auto begin() const { return values.begin(); }
    auto end() const { return values.end(); }
};
)HSLCPP";

        for (const auto& child : root.children) {
            if (child->kind == AstKind::EnumDeclaration) {
                enumeration(*child);
            }
            if (child->kind == AstKind::StructDeclaration) {
                structure(*child);
            }
        }

        for (const auto& child : root.children) {
            if (child->kind == AstKind::Function && name_part(child->value) != "main") {
                signature(*child);
                output_ << ";\n";
            }
        }
        output_ << '\n';
        for (const auto& child : root.children) {
            if (child->kind == AstKind::Function) function(*child);
        }
        return output_.str();
    }

private:
    std::string infer_match_enum(const AstNode& node) const {
        if (node.kind == AstKind::IdentifierExpression) {
            const auto found = local_types_.find(node.value);
            if (found != local_types_.end()) return found->second;
        }
        return {};
    }

    void indentation() {
        output_ << std::string(indent_ * 4, ' ');
    }

    void enumeration(const AstNode& node) {
        bool payloads = false;
        for (const auto& variant : node.children) {
            payloads = payloads || !type_part(variant->value).empty();
        }
        if (!payloads) {
            output_ << "enum class " << node.value << " {\n";
            ++indent_;
            for (std::size_t index = 0; index < node.children.size(); ++index) {
                indentation();
                output_ << name_part(node.children[index]->value);
                if (index + 1 != node.children.size()) output_ << ',';
                output_ << "\n";
            }
            --indent_;
            output_ << "};\n\n";
            return;
        }
        output_ << "struct " << node.value << " {\n";
        ++indent_;
        indentation(); output_ << "enum class Tag { ";
        for (std::size_t index = 0; index < node.children.size(); ++index) {
            if (index) output_ << ", ";
            output_ << name_part(node.children[index]->value);
        }
        output_ << " };\n";
        indentation(); output_ << "Tag tag;\n";
        indentation(); output_ << "std::variant<std::monostate";
        for (const auto& variant : node.children) {
            if (!type_part(variant->value).empty()) output_ << ", " << cpp_type(type_part(variant->value));
        }
        output_ << "> payload;\n";
        for (const auto& variant : node.children) {
            indentation(); output_ << "static " << node.value << ' ' << name_part(variant->value) << '(';
            if (!type_part(variant->value).empty()) output_ << cpp_type(type_part(variant->value)) << " value";
            output_ << ") { return {Tag::" << name_part(variant->value) << ", ";
            output_ << (type_part(variant->value).empty() ? "std::monostate{}" : "std::move(value)");
            output_ << "}; }\n";
        }
        indentation(); output_ << "bool operator==(const " << node.value << "& other) const { return tag == other.tag && payload == other.payload; }\n";
        --indent_;
        output_ << "};\n\n";
    }

    void structure(const AstNode& node) {
        output_ << "struct " << node.value << " {\n";
        ++indent_;

        for (const auto& field : node.children) {
            if (field->kind != AstKind::FieldDeclaration) {
                continue;
            }

            indentation();
            output_ << cpp_type(type_part(field->value))
                    << ' '
                    << name_part(field->value)
                    << ";\n";
        }

        --indent_;
        output_ << "};\n\n";
    }

    void signature(const AstNode& node) {
        const auto name = name_part(node.value);
        output_ << (name == "main" ? "int" : cpp_type(type_part(node.value)))
                << ' ' << name << '(';
        if (name == "main") {
            output_ << "int argc, char** argv";
        }
        bool first = true;
        for (const auto& child : node.children) {
            if (child->kind != AstKind::Parameter) continue;
            if (!first) output_ << ", ";
            output_ << cpp_type(type_part(child->value)) << ' ' << name_part(child->value);
            first = false;
        }
        output_ << ')';
    }

    void function(const AstNode& node) {
        local_types_.clear();
        for (const auto& child : node.children) {
            if (child->kind == AstKind::Parameter) {
                local_types_[std::string(name_part(child->value))] = std::string(type_part(child->value));
            }
        }
        signature(node);
        output_ << " {\n";
        ++indent_;
        if (name_part(node.value) == "main") {
            indentation();
            output_ << "hsl_program_argc = argc; hsl_program_argv = argv;\n";
        }
        for (const auto& child : node.children) {
            if (child->kind == AstKind::Block) block(*child);
        }
        if (name_part(node.value) == "main") {
            indentation();
            output_ << "return 0;\n";
        }
        --indent_;
        output_ << "}\n\n";
    }

    void block(const AstNode& node) {
        for (const auto& child : node.children) statement(*child);
    }

    void statement(const AstNode& node) {
        switch (node.kind) {
            case AstKind::LetDeclaration:
            case AstKind::VarDeclaration: {
                indentation();

                const Type declared_type =
                    parse_type_name(type_part(node.value));

                if (
                    node.kind == AstKind::LetDeclaration &&
                    declared_type.kind != TypeKind::List &&
                    declared_type.kind != TypeKind::Reference
                ) {
                    output_ << "const ";
                }
                local_types_[std::string(name_part(node.value))] = std::string(type_part(node.value));
                output_ << cpp_type(declared_type) << ' ' << name_part(node.value) << " = ";

                if (
                    !node.children.empty() &&
                    node.children.front()->kind == AstKind::ArrayExpression
                ) {
                    output_ << '{';
                    expression(*node.children.front());
                    output_ << '}';
                } else {
                    expression(*node.children.front());
                }

                output_ << ";\n";
                return;
            }
            case AstKind::MatchStatement: {
                const auto& subject = *node.children.front();
                const std::string subject_type = infer_match_enum(subject);
                indentation();
                output_ << "switch (";
                expression(subject);
                output_ << (enum_payloads_[subject_type] ? ".tag" : "") << ") {\n";
                ++indent_;
                for (std::size_t index = 1; index < node.children.size(); ++index) {
                    const auto& arm = *node.children[index];
                    const auto separator = arm.value.find(':');
                    const std::string pattern = arm.value.substr(0, separator);
                    const std::string binding = separator == std::string::npos ? "" : arm.value.substr(separator + 1);
                    indentation();
                    if (pattern == "_") output_ << "default: {\n";
                    else {
                        const auto dot = pattern.find('.');
                        const std::string enum_name = pattern.substr(0, dot);
                        const std::string variant = pattern.substr(dot + 1);
                        output_ << "case " << enum_name << (enum_payloads_[enum_name] ? "::Tag::" : "::") << variant << ": {\n";
                    }
                    ++indent_;
                    if (!binding.empty() && pattern != "_") {
                        const auto dot = pattern.find('.');
                        const std::string enum_name = pattern.substr(0, dot);
                        const std::string variant = pattern.substr(dot + 1);
                        indentation();
                        output_ << "const auto& " << binding << " = std::get<" << cpp_type(enum_variants_[enum_name][variant]) << ">(";
                        expression(subject); output_ << ".payload);\n";
                    }
                    if (!arm.children.empty()) block(*arm.children.front());
                    indentation(); output_ << "break;\n";
                    --indent_;
                    indentation(); output_ << "}\n";
                }
                --indent_;
                indentation(); output_ << "}\n";
                return;
            }
            case AstKind::ReturnStatement:
                indentation();
                output_ << "return";
                if (!node.children.empty()) {
                    output_ << ' ';
                    expression(*node.children.front());
                }
                output_ << ";\n";
                return;
            case AstKind::ExpressionStatement:
                indentation();
                expression(*node.children.front());
                output_ << ";\n";
                return;
            case AstKind::ForStatement: {
                const auto& iterable = *node.children[0];

                if (iterable.kind == AstKind::RangeExpression) {
                    const std::size_t range_id = range_id_++;
                    const std::string start_name =
                        "hsl_range_start_" +
                        std::to_string(range_id);
                    const std::string end_name =
                        "hsl_range_end_" +
                        std::to_string(range_id);

                    indentation();
                    output_ << "const std::int64_t "
                            << start_name << " = ";
                    expression(*iterable.children[0]);
                    output_ << ";\n";

                    indentation();
                    output_ << "const std::int64_t "
                            << end_name << " = ";
                    expression(*iterable.children[1]);
                    output_ << ";\n";

                    indentation();
                    output_ << "for (std::int64_t "
                            << node.value << " = " << start_name
                            << "; " << node.value
                            << (iterable.value == "Inclusive"
                                ? " <= "
                                : " < ")
                            << end_name << "; ++"
                            << node.value << ") {\n";
                } else {
                    indentation();
                    output_ << "for (const auto& "
                            << node.value << " : ";
                    expression(iterable);
                    output_ << ") {\n";
                }

                ++indent_;
                block(*node.children[1]);
                --indent_;
                indentation();
                output_ << "}\n";
                return;
            }
            case AstKind::WhileStatement:
                indentation();
                output_ << "while (static_cast<bool>(";
                expression(*node.children.front());
                output_ << ")) {\n";
                ++indent_;
                block(*node.children[1]);
                --indent_;
                indentation();
                output_ << "}\n";
                return;
            case AstKind::BreakStatement:
                indentation();
                output_ << "break;\n";
                return;
            case AstKind::ContinueStatement:
                indentation();
                output_ << "continue;\n";
                return;
            case AstKind::IfStatement:
                indentation();
                output_ << "if (static_cast<bool>(";
                expression(*node.children.front());
                output_ << ")) {\n";
                ++indent_;
                block(*node.children[1]);
                --indent_;
                indentation();
                output_ << '}';
                if (node.children.size() > 2) {
                    output_ << " else {\n";
                    ++indent_;
                    block(*node.children[2]->children.front());
                    --indent_;
                    indentation();
                    output_ << '}';
                }
                output_ << '\n';
                return;
            default:
                return;
        }
    }

    void expression(const AstNode& node) {
        switch (node.kind) {
            case AstKind::IdentifierExpression:
            case AstKind::BooleanLiteral:
            case AstKind::StringLiteral:
                output_ << node.value;
                return;
            case AstKind::IntegerLiteral:
            case AstKind::FloatLiteral:
                output_ << numeric_text(node.value);
                return;
            case AstKind::MemberExpression:
                if (
                    node.children.front()->kind == AstKind::IdentifierExpression &&
                    !node.children.front()->value.empty() &&
                    node.children.front()->value.front() >= 'A' &&
                    node.children.front()->value.front() <= 'Z'
                ) {
                    expression(*node.children.front());
                    output_ << "::" << node.value;
                } else {
                    expression(*node.children.front());
                    output_ << '.' << node.value;
                }
                return;
            case AstKind::IndexExpression:
                expression(*node.children.front());
                output_ << '[';
                expression(*node.children.back());
                output_ << ']';
                return;
            case AstKind::SliceExpression: {
                output_ << "std::span(";
                expression(*node.children.front());
                output_ << ')';

                const bool has_start =
                    node.value == "ExclusiveStart" ||
                    node.value == "InclusiveStart" ||
                    node.value == "ExclusiveBoth" ||
                    node.value == "InclusiveBoth";
                const bool has_end =
                    node.value == "ExclusiveEnd" ||
                    node.value == "InclusiveEnd" ||
                    node.value == "ExclusiveBoth" ||
                    node.value == "InclusiveBoth";
                const bool inclusive =
                    node.value == "InclusiveEnd" ||
                    node.value == "InclusiveBoth";

                if (has_start || has_end) {
                    output_ << ".subspan(";

                    if (has_start) {
                        expression(*node.children[1]);
                    } else {
                        output_ << '0';
                    }

                    if (has_end) {
                        output_ << ", (";
                        const std::size_t end_index =
                            has_start ? 2 : 1;
                        expression(*node.children[end_index]);

                        if (inclusive) {
                            output_ << " + 1";
                        }

                        output_ << ") - (";

                        if (has_start) {
                            expression(*node.children[1]);
                        } else {
                            output_ << '0';
                        }

                        output_ << ')';
                    }

                    output_ << ')';
                }

                return;
            }
            case AstKind::ArrayExpression:
                output_ << '{';
                for (
                    std::size_t index = 0;
                    index < node.children.size();
                    ++index
                ) {
                    if (index != 0) output_ << ", ";
                    expression(*node.children[index]);
                }
                output_ << '}';
                return;
            case AstKind::UnaryExpression:
                output_ << '(' << operator_text(node.value);
                expression(*node.children.front());
                output_ << ')';
                return;
            case AstKind::BorrowExpression:
            case AstKind::DereferenceExpression:
                expression(*node.children.front());
                return;
            case AstKind::MoveExpression:
                output_ << "std::move(";
                expression(*node.children.front());
                output_ << ')';
                return;
            case AstKind::BinaryExpression:
            case AstKind::AssignmentExpression:
                output_ << '(';
                expression(*node.children[0]);
                output_ << ' ' << operator_text(node.value) << ' ';
                expression(*node.children[1]);
                output_ << ')';
                return;
            case AstKind::CallExpression: {
                if (
                    node.children.front()->kind == AstKind::MemberExpression &&
                    !node.children.front()->children.empty()
                ) {
                    const auto& member = *node.children.front();
                    if (member.children.front()->kind == AstKind::IdentifierExpression) {
                        const std::string& base_name = member.children.front()->value;
                        if (base_name.starts_with("Option<") || base_name.starts_with("Result<")) {
                            output_ << cpp_type(base_name) << "::" << member.value << '(';
                            for (std::size_t index = 1; index < node.children.size(); ++index) {
                                if (index != 1) output_ << ", ";
                                expression(*node.children[index]);
                            }
                            output_ << ')';
                            return;
                        }
                    }
                    const auto emit_string_call = [&](std::string_view helper) {
                        output_ << helper << '(';
                        expression(*member.children.front());
                        for (std::size_t index = 1; index < node.children.size(); ++index) {
                            output_ << ", ";
                            expression(*node.children[index]);
                        }
                        output_ << ')';
                    };
                    bool is_string_receiver = member.children.front()->kind == AstKind::StringLiteral;
                    if (member.children.front()->kind == AstKind::IdentifierExpression) {
                        const auto found = local_types_.find(member.children.front()->value);
                        is_string_receiver = found != local_types_.end() && found->second == "str";
                    }
                    if (is_string_receiver && member.value == "starts_with") { emit_string_call("hsl_string_starts_with"); return; }
                    if (is_string_receiver && member.value == "ends_with") { emit_string_call("hsl_string_ends_with"); return; }
                    if (is_string_receiver && member.value == "contains") { emit_string_call("hsl_string_contains"); return; }
                    if (is_string_receiver && member.value == "find") { emit_string_call("hsl_string_find"); return; }
                    if (is_string_receiver && member.value == "byte_at") { emit_string_call("hsl_string_byte_at"); return; }
                    if (is_string_receiver && member.value == "slice") { emit_string_call("hsl_string_slice"); return; }
                }
                if (
                    node.children.front()->kind == AstKind::IdentifierExpression &&
                    (node.children.front()->value.starts_with("List<") ||
                     node.children.front()->value.starts_with("Map<") ||
                     node.children.front()->value.starts_with("Set<"))
                ) {
                    output_ << cpp_type(
                        node.children.front()->value
                    ) << "{}";
                    return;
                }
                if (node.children.front()->kind == AstKind::IdentifierExpression) {
                    const std::string& builtin = node.children.front()->value;
                    const char* native = nullptr;
                    if (builtin == "env_get") native = "hsl_env_get";
                    else if (builtin == "process_run_program") native = "hsl_process_run_program";
                    else if (builtin == "process_run_arg") native = "hsl_process_run_arg";
                    else if (builtin == "fs_copy") native = "hsl_fs_copy";
                    else if (builtin == "fs_remove_all") native = "hsl_fs_remove_all";
                    else if (builtin == "fs_file_size") native = "hsl_fs_file_size";
                    else if (builtin == "time_unix_ms") native = "hsl_time_unix_ms";
                    else if (builtin == "sleep_ms") native = "hsl_sleep_ms";
                    else if (builtin == "read_line") native = "hsl_read_line";
                    else if (builtin == "i64_to_string") native = "hsl_i64_to_string";
                    else if (builtin == "try_parse_i64") native = "hsl_try_parse_i64";
                    else if (builtin == "arg_count") native = "hsl_arg_count";
                    else if (builtin == "arg") native = "hsl_arg";
                    else if (builtin == "set_current_directory") native = "hsl_set_current_directory";
                    else if (builtin == "env_set") native = "hsl_env_set";
                    else if (builtin == "env_unset") native = "hsl_env_unset";
                    else if (builtin == "utf8_length") native = "hsl_utf8_length";
                    else if (builtin == "utf8_scalar_at") native = "hsl_utf8_scalar_at";
                    else if (builtin == "byte_length") native = "hsl_byte_length";
                    else if (builtin == "fs_create_directory") native = "hsl_fs_create_directory";
                    else if (builtin == "fs_create_directories") native = "hsl_fs_create_directories";
                    else if (builtin == "fs_remove") native = "hsl_fs_remove";
                    else if (builtin == "fs_rename") native = "hsl_fs_rename";
                    else if (builtin == "fs_is_file") native = "hsl_fs_is_file";
                    else if (builtin == "fs_is_directory") native = "hsl_fs_is_directory";
                    else if (builtin == "path_parent") native = "hsl_path_parent";
                    else if (builtin == "path_filename") native = "hsl_path_filename";
                    else if (builtin == "path_extension") native = "hsl_path_extension";
                    else if (builtin == "path_normalize") native = "hsl_path_normalize";
                    else if (builtin == "process_run") native = "hsl_process_run";
                    else if (builtin == "current_directory") native = "hsl_current_directory";
                    else if (builtin == "fs_exists") native = "hsl_fs_exists";
                    else if (builtin == "fs_read") native = "hsl_fs_read";
                    else if (builtin == "fs_write") native = "hsl_fs_write";
                    else if (builtin == "path_join") native = "hsl_path_join";
                    else if (builtin == "path_is_absolute") native = "hsl_path_is_absolute";
                    if (native) {
                        output_ << native << '(';
                        for (std::size_t index = 1; index < node.children.size(); ++index) {
                            if (index != 1) output_ << ", ";
                            expression(*node.children[index]);
                        }
                        output_ << ')';
                        return;
                    }
                }
                if (node.children.front()->value == "print") {
                    output_ << "(std::cout << ";
                    expression(*node.children[1]);
                    output_ << " << '\\n')";
                    return;
                }
                const bool aggregate =
                    node.children.front()->kind ==
                        AstKind::IdentifierExpression &&
                    !node.children.front()->value.empty() &&
                    node.children.front()->value.front() >= 'A' &&
                    node.children.front()->value.front() <= 'Z';

                expression(*node.children.front());
                output_ << (aggregate ? '{' : '(');

                for (
                    std::size_t index = 1;
                    index < node.children.size();
                    ++index
                ) {
                    if (index != 1) {
                        output_ << ", ";
                    }

                    expression(*node.children[index]);
                }

                output_ << (aggregate ? '}' : ')');
                return;
            }
            default:
                output_ << "0";
                return;
        }
    }

    std::ostringstream output_;
    std::unordered_map<std::string, bool> enum_payloads_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> enum_variants_;
    std::unordered_map<std::string, std::string> local_types_;
    std::size_t indent_ = 0;
    std::size_t range_id_ = 0;
};

int run_compiler(const std::filesystem::path& generated,
                 const std::filesystem::path& output,
                 const std::string& emission = "executable") {
    const auto environment = [](const char* name) -> std::string {
        const char* value = std::getenv(name);
        return value ? std::string(value) : std::string{};
    };
    const std::string compiler = environment("HSL_CXX").empty()
        ? "clang++" : environment("HSL_CXX");
    const std::string target = environment("HSL_TARGET");
    const std::string cpu = environment("HSL_CPU");
    const std::string sysroot = environment("HSL_SYSROOT");
    const std::string profile = environment("HSL_ARCH_PROFILE");
    const std::string generated_string = generated.string();
    const std::string output_string = output.string();

    std::vector<std::string> storage{
        compiler, "-std=c++20", "-O2", "-fstack-protector-strong",
        "-fPIE", "-pie"
    };
    if (!target.empty()) storage.push_back("--target=" + target);
    if (!sysroot.empty()) storage.push_back("--sysroot=" + sysroot);
    if (!cpu.empty()) storage.push_back("-mcpu=" + cpu);
    if (profile == "x86_64-portable") storage.push_back("-march=x86-64");
    else if (profile == "x86_64-v2") storage.push_back("-march=x86-64-v2");
    else if (profile == "aarch64-portable") storage.push_back("-march=armv8-a");
    else if (profile == "armv7-portable") storage.push_back("-march=armv7-a");
    else if (!profile.empty() && profile != "native") return 64;
    else if (profile == "native") storage.push_back("-march=native");
    if (emission == "object") storage.push_back("-c");
    else if (emission == "assembly") storage.push_back("-S");
    storage.push_back(generated_string);
    storage.push_back("-o");
    storage.push_back(output_string);

    std::vector<char*> arguments;
    arguments.reserve(storage.size() + 1);
    for (auto& value : storage) arguments.push_back(value.data());
    arguments.push_back(nullptr);

    const pid_t child = fork();
    if (child == 0) {
        execvp(arguments[0], arguments.data());
        _exit(127);
    }
    if (child < 0) return -1;

    int status = 0;
    if (waitpid(child, &status, 0) < 0) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

}

std::string generate_cpp(const AstNode& root) {
    return Emitter().emit(root);
}

CodeGenerationResult build_native(const AstNode& root,
                                  const std::filesystem::path& source_path,
                                  bool keep_generated) {
    auto output_path = source_path;
    output_path.replace_extension();
    auto generated_path = output_path;
    generated_path += ".generated.cpp";

    std::ofstream generated(generated_path, std::ios::binary);
    if (!generated) return {false, "cannot create generated C++ source"};
    generated << generate_cpp(root);
    generated.close();

    const int result = run_compiler(generated_path, output_path);
    if (result != 0) return {false, "native compiler failed with exit code " + std::to_string(result)};

    if (!keep_generated) {
        std::error_code removal_error;
        std::filesystem::remove(generated_path, removal_error);
    }
    return {true, output_path.string()};
}

CodeGenerationResult emit_artifact(const AstNode& root,
                                   const std::filesystem::path& source_path,
                                   const std::string& emission,
                                   const std::string& extension) {
    auto output_path = source_path;
    output_path.replace_extension(extension);
    auto generated_path = source_path;
    generated_path.replace_extension(".generated.cpp");
    std::ofstream generated(generated_path, std::ios::binary);
    if (!generated) return {false, "cannot create generated C++ source"};
    generated << generate_cpp(root);
    generated.close();
    const int result = run_compiler(generated_path, output_path, emission);
    std::error_code removal_error;
    std::filesystem::remove(generated_path, removal_error);
    if (result != 0) return {false, "native compiler failed with exit code " + std::to_string(result)};
    return {true, output_path.string()};
}

CodeGenerationResult emit_object(const AstNode& root,
                                 const std::filesystem::path& source_path) {
    return emit_artifact(root, source_path, "object", ".o");
}

CodeGenerationResult emit_assembly(const AstNode& root,
                                   const std::filesystem::path& source_path) {
    return emit_artifact(root, source_path, "assembly", ".s");
}

}
