#pragma once

#include "hsl/source_span.hpp"

#include <cstdint>
#include <string_view>

namespace hsl {

enum class Kind : std::uint16_t {
    Eof, Newline, Indent, Dedent, Identifier, Integer, Float, String,
    Fn, Import, Let, Var, Move, Return, If, Else, Match, While, For, In, Break, Continue, True, False, Struct, Enum, Class,
    TypeI8, TypeU8, TypeI16, TypeU16, TypeI32, TypeU32, TypeI64, TypeU64, TypeI128, TypeU128, TypeISize, TypeUSize, TypeF32, TypeF64, TypeBool, TypeStr, TypeNever,
    LParen, RParen, LBracket, RBracket, LBrace, RBrace,
    Comma, Colon, Semicolon, Dot, RangeExclusive, RangeInclusive, At,
    Equal, EqualEqual, FatArrow,
    Bang, BangEqual,
    Plus, PlusEqual,
    Minus, MinusEqual, Arrow,
    Star, StarEqual, Power,
    Slash, SlashEqual,
    Percent, PercentEqual,
    Amp, AmpEqual, AmpAmp,
    Pipe, PipeEqual, PipePipe,
    Caret, CaretEqual, Tilde,
    Less, LessEqual, ShiftLeft, ShiftLeftEqual,
    Greater, GreaterEqual, ShiftRight, ShiftRightEqual,
    Invalid
};

struct Token {
    Kind kind;
    SourceSpan span;
    std::string_view text;
};

std::string_view kind_name(Kind kind);

}
