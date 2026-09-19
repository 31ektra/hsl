#include "hsl/lexer.hpp"

#include <cctype>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hsl {

class Lexer {
public:
    explicit Lexer(std::string_view source) : source_(source) {}

    std::vector<Token> scan() {
        while (!end()) {
            if (line_start_) indent();
            if (!end()) token();
        }
        while (indents_.size() > 1) {
            indents_.pop_back();
            add_empty(Kind::Dedent);
        }
        add_empty(Kind::Eof);
        return tokens_;
    }

    std::vector<Diagnostic> take_diagnostics() {
        return std::move(diagnostics_);
    }

private:
    bool end() const { return pos_ >= source_.size(); }
    char peek(std::size_t n = 0) const {
        return pos_ + n < source_.size() ? source_[pos_ + n] : '\0';
    }
    char advance() { ++column_; return source_[pos_++]; }
    bool match(char c) {
        if (peek() != c) return false;
        advance();
        return true;
    }
    void error(std::string_view message) {
        diagnostics_.push_back(Diagnostic{
            DiagnosticSeverity::Error,
            SourceSpan{
                static_cast<std::uint32_t>(pos_),
                0,
                line_,
                column_,
            },
            std::string(message),
        });
    }
    void add(Kind kind, std::size_t start, std::uint32_t line, std::uint32_t column) {
        const auto length = static_cast<std::uint32_t>(pos_ - start);
        tokens_.push_back({
            kind,
            SourceSpan{
                static_cast<std::uint32_t>(start),
                length,
                line,
                column,
            },
            source_.substr(start, length),
        });
    }
    void add_empty(Kind kind) {
        tokens_.push_back({
            kind,
            SourceSpan{
                static_cast<std::uint32_t>(pos_),
                0,
                line_,
                column_,
            },
            {},
        });
    }

    void indent() {
        const auto start = pos_;
        const auto column = column_;
        std::uint32_t spaces = 0;
        while (peek() == ' ') { advance(); ++spaces; }
        if (peek() == '\t') {
            error("tabs are forbidden; use four spaces");
            while (peek() == '\t') advance();
        }
        if (peek() == '\n' || peek() == '\r' || peek() == '#') {
            line_start_ = false;
            return;
        }
        if (spaces % 4 != 0) {
            error("indentation must be a multiple of four spaces");
            spaces -= spaces % 4;
        }
        if (spaces > indents_.back()) {
            indents_.push_back(spaces);
            add(Kind::Indent, start, line_, column);
        } else if (spaces < indents_.back()) {
            while (indents_.size() > 1 && spaces < indents_.back()) {
                indents_.pop_back();
                add_empty(Kind::Dedent);
            }
            if (spaces != indents_.back()) error("dedent does not match an earlier level");
        }
        line_start_ = false;
    }

    void token() {
        const auto start = pos_;
        const auto line = line_;
        const auto column = column_;
        const char c = advance();
        switch (c) {
            case ' ': return;
            case '\t': error("tabs are forbidden"); return;
            case '#': while (!end() && peek() != '\n') advance(); return;
            case '"': string(start, line, column); return;
            case '\r': match('\n'); add(Kind::Newline, start, line, column); newline(); return;
            case '\n': add(Kind::Newline, start, line, column); newline(); return;
            case '(': add(Kind::LParen, start, line, column); return;
            case ')': add(Kind::RParen, start, line, column); return;
            case '[': add(Kind::LBracket, start, line, column); return;
            case ']': add(Kind::RBracket, start, line, column); return;
            case '{': add(Kind::LBrace, start, line, column); return;
            case '}': add(Kind::RBrace, start, line, column); return;
            case ',': add(Kind::Comma, start, line, column); return;
            case ':': add(Kind::Colon, start, line, column); return;
            case ';': add(Kind::Semicolon, start, line, column); return;
            case '.':
                if (match('.')) {
                    add(match('=') ? Kind::RangeInclusive : Kind::RangeExclusive,
                        start, line, column);
                } else {
                    add(Kind::Dot, start, line, column);
                }
                return;
            case '@': add(Kind::At, start, line, column); return;
            case '+': add(match('=') ? Kind::PlusEqual : Kind::Plus, start, line, column); return;
            case '-':
                add(match('>') ? Kind::Arrow : match('=') ? Kind::MinusEqual : Kind::Minus,
                    start, line, column);
                return;
            case '*':
                add(match('*') ? Kind::Power : match('=') ? Kind::StarEqual : Kind::Star,
                    start, line, column);
                return;
            case '/': add(match('=') ? Kind::SlashEqual : Kind::Slash, start, line, column); return;
            case '%': add(match('=') ? Kind::PercentEqual : Kind::Percent, start, line, column); return;
            case '=':
                add(match('=') ? Kind::EqualEqual : match('>') ? Kind::FatArrow : Kind::Equal,
                    start, line, column);
                return;
            case '!': add(match('=') ? Kind::BangEqual : Kind::Bang, start, line, column); return;
            case '&':
                add(match('&') ? Kind::AmpAmp : match('=') ? Kind::AmpEqual : Kind::Amp,
                    start, line, column);
                return;
            case '|':
                add(match('|') ? Kind::PipePipe : match('=') ? Kind::PipeEqual : Kind::Pipe,
                    start, line, column);
                return;
            case '^': add(match('=') ? Kind::CaretEqual : Kind::Caret, start, line, column); return;
            case '~': add(Kind::Tilde, start, line, column); return;
            case '<':
                if (match('<')) {
                    add(match('=') ? Kind::ShiftLeftEqual : Kind::ShiftLeft, start, line, column);
                } else {
                    add(match('=') ? Kind::LessEqual : Kind::Less, start, line, column);
                }
                return;
            case '>':
                if (match('>')) {
                    add(match('=') ? Kind::ShiftRightEqual : Kind::ShiftRight, start, line, column);
                } else {
                    add(match('=') ? Kind::GreaterEqual : Kind::Greater, start, line, column);
                }
                return;
            default: break;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
            const auto text = source_.substr(start, pos_ - start);
            add(keyword(text), start, line, column);
            return;
        }
        if (std::isdigit(static_cast<unsigned char>(c))) {
            number(start, line, column);
            return;
        }
        error("unexpected character");
        add(Kind::Invalid, start, line, column);
    }

    void number(std::size_t start, std::uint32_t line, std::uint32_t column) {
        auto valid_digit = [](char value, unsigned base) {
            if (value >= '0' && value <= '9') {
                return static_cast<unsigned>(value - '0') < base;
            }
            if (base == 16 && value >= 'a' && value <= 'f') return true;
            if (base == 16 && value >= 'A' && value <= 'F') return true;
            return false;
        };

        if (source_[start] == '0' &&
            (peek() == 'b' || peek() == 'o' || peek() == 'x')) {
            const char prefix = advance();
            const unsigned base = prefix == 'b' ? 2U : prefix == 'o' ? 8U : 16U;
            bool saw_digit = false;
            bool previous_separator = false;

            while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') {
                const char value = advance();
                if (value == '_') {
                    if (!saw_digit || previous_separator) {
                        error("invalid numeric separator placement");
                    }
                    previous_separator = true;
                } else {
                    if (!valid_digit(value, base)) {
                        error("invalid digit for numeric base");
                    }
                    saw_digit = true;
                    previous_separator = false;
                }
            }

            if (!saw_digit) error("numeric base prefix requires digits");
            if (previous_separator) error("numeric literal cannot end with a separator");
            add(Kind::Integer, start, line, column);
            return;
        }

        bool is_float = false;
        bool previous_separator = false;

        while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_') {
            const char value = advance();
            if (value == '_') {
                if (previous_separator) error("invalid numeric separator placement");
                previous_separator = true;
            } else {
                previous_separator = false;
            }
        }
        if (previous_separator) error("numeric literal cannot end with a separator");

        if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
            is_float = true;
            advance();
            previous_separator = false;
            while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_') {
                const char value = advance();
                if (value == '_') {
                    if (previous_separator) error("invalid numeric separator placement");
                    previous_separator = true;
                } else {
                    previous_separator = false;
                }
            }
            if (previous_separator) error("numeric literal cannot end with a separator");
        }

        if (peek() == 'e' || peek() == 'E') {
            is_float = true;
            advance();
            if (peek() == '+' || peek() == '-') advance();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) {
                error("exponent requires decimal digits");
            }
            while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '_') {
                const char value = advance();
                if (value == '_') {
                    if (previous_separator) error("invalid numeric separator placement");
                    previous_separator = true;
                } else {
                    previous_separator = false;
                }
            }
            if (previous_separator) error("numeric literal cannot end with a separator");
        }

        if (is_float && peek() == 'f' &&
            (source_.substr(pos_, 3) == "f32" || source_.substr(pos_, 3) == "f64")) {
            advance();
            advance();
            advance();
        }

        add(is_float ? Kind::Float : Kind::Integer, start, line, column);
    }

    void string(std::size_t start, std::uint32_t line, std::uint32_t column) {
        while (!end() && peek() != '"' && peek() != '\n' && peek() != '\r') {
            if (peek() == '\\') {
                advance();
                if (end() || peek() == '\n' || peek() == '\r') {
                    error("unterminated string literal");
                    add(Kind::Invalid, start, line, column);
                    return;
                }

                const char escape = advance();
                if (escape != 'n' && escape != 'r' && escape != 't' &&
                    escape != '\\' && escape != '"' && escape != '0') {
                    error("unknown string escape sequence");
                }
                continue;
            }
            advance();
        }

        if (end() || peek() != '"') {
            error("unterminated string literal");
            add(Kind::Invalid, start, line, column);
            return;
        }

        advance();
        add(Kind::String, start, line, column);
    }

    void newline() { ++line_; column_ = 1; line_start_ = true; }

    static Kind keyword(std::string_view s) {
        static const std::unordered_map<std::string_view, Kind> words = {
            {"fn", Kind::Fn}, {"import", Kind::Import}, {"let", Kind::Let}, {"var", Kind::Var},
            {"move", Kind::Move},
            {"return", Kind::Return}, {"if", Kind::If}, {"else", Kind::Else}, {"match", Kind::Match},
            {"while", Kind::While}, {"for", Kind::For}, {"in", Kind::In},
            {"break", Kind::Break}, {"continue", Kind::Continue},
            {"true", Kind::True}, {"false", Kind::False}, {"struct", Kind::Struct}, {"enum", Kind::Enum},
            {"class", Kind::Class},
            {"i8", Kind::TypeI8}, {"u8", Kind::TypeU8}, {"i16", Kind::TypeI16}, {"u16", Kind::TypeU16}, {"i32", Kind::TypeI32}, {"u32", Kind::TypeU32}, {"i64", Kind::TypeI64}, {"u64", Kind::TypeU64}, {"i128", Kind::TypeI128}, {"u128", Kind::TypeU128}, {"isize", Kind::TypeISize}, {"usize", Kind::TypeUSize},
            {"f32", Kind::TypeF32}, {"f64", Kind::TypeF64},
            {"bool", Kind::TypeBool}, {"str", Kind::TypeStr}, {"never", Kind::TypeNever}
        };
        const auto it = words.find(s);
        return it == words.end() ? Kind::Identifier : it->second;
    }

    std::string_view source_;
    std::size_t pos_ = 0;
    std::uint32_t line_ = 1;
    std::uint32_t column_ = 1;
    bool line_start_ = true;
    std::vector<Diagnostic> diagnostics_;
    std::vector<std::uint32_t> indents_{0};
    std::vector<Token> tokens_;
};

bool LexResult::failed() const noexcept {
    return !diagnostics.empty();
}

LexResult lex_source(std::string_view source) {
    Lexer lexer(source);
    auto tokens = lexer.scan();
    auto diagnostics = lexer.take_diagnostics();
    return LexResult{std::move(tokens), std::move(diagnostics)};
}

}
