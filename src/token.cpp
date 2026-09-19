#include "hsl/token.hpp"

namespace hsl {

std::string_view kind_name(Kind kind) {
#define CASE(x) case Kind::x: return #x
    switch (kind) {
        CASE(Eof); CASE(Newline); CASE(Indent); CASE(Dedent);
        CASE(Identifier); CASE(Integer); CASE(Float); CASE(String); CASE(Fn); CASE(Import); CASE(Let); CASE(Var); CASE(Move);
        CASE(Return); CASE(If); CASE(Else); CASE(Match); CASE(While); CASE(For); CASE(In);
        CASE(Break); CASE(Continue);
        CASE(True); CASE(False); CASE(Struct); CASE(Enum); CASE(Class);
        CASE(TypeI8); CASE(TypeU8); CASE(TypeI16); CASE(TypeU16); CASE(TypeI32); CASE(TypeU32); CASE(TypeI64); CASE(TypeU64); CASE(TypeI128); CASE(TypeU128); CASE(TypeISize); CASE(TypeUSize); CASE(TypeF32); CASE(TypeF64);
        CASE(TypeBool); CASE(TypeStr); CASE(TypeNever); CASE(LParen); CASE(RParen);
        CASE(LBracket); CASE(RBracket); CASE(LBrace); CASE(RBrace);
        CASE(Comma); CASE(Colon); CASE(Semicolon); CASE(Dot);
        CASE(RangeExclusive); CASE(RangeInclusive); CASE(At);
        CASE(Equal); CASE(EqualEqual); CASE(FatArrow); CASE(Bang); CASE(BangEqual);
        CASE(Plus); CASE(PlusEqual); CASE(Minus); CASE(MinusEqual); CASE(Arrow);
        CASE(Star); CASE(StarEqual); CASE(Power); CASE(Slash); CASE(SlashEqual);
        CASE(Percent); CASE(PercentEqual); CASE(Amp); CASE(AmpEqual); CASE(AmpAmp);
        CASE(Pipe); CASE(PipeEqual); CASE(PipePipe); CASE(Caret); CASE(CaretEqual);
        CASE(Tilde); CASE(Less); CASE(LessEqual); CASE(ShiftLeft); CASE(ShiftLeftEqual);
        CASE(Greater); CASE(GreaterEqual); CASE(ShiftRight); CASE(ShiftRightEqual);
        CASE(Invalid);
    }
#undef CASE
    return "Unknown";
}

}
