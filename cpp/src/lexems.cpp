#include <syntics/cpp.hpp>

namespace syntics::syntax
{

    const str::list Lexems::u_operators       = { "*", "&", "--", "++", "!", "-" };
    const str::list Lexems::b_operators       = { "=", "+=", "-=", "!=", "&&", "||", "==", "*", "/", "&", "+", "->", "-", ".", "<=",">=","<",">"};
    const str::list Lexems::Lexems::operators = str::list{"++","--","=="} + Lexems::b_operators + Lexems::u_operators;
    const str::list Lexems::block_markers     = { "{}", "()", "[]" };

    const str::list Lexems::separators        = { ";", ","};
    const str::list Lexems::keywords          = { "namespace", "class", "struct", "template", "typename", "enum", "operator", "private", "public", "protected" };
    const str::list Lexems::built_in_types    = { "int", "char", "double", "float", "void", "bool", "auto" };
    const str::list Lexems::qualifiers        = { "const", "virtual", "static", "noexcept" };
    const str::pair_list Lexems::stoppers     = { {"#","\n"}, {"//","\n"}, {"/*","*/"}, {"\"","\""}  };
    const str::list Lexems::syntax_markers    = {":", "...", "?"};
    const str::list Lexems::instructions      = {"return","new","delete","using","typedef","throw"};
    const str::list Lexems::controls          = {"if","else if","else","for","while"};
    const str::list Lexems::known_tokens      = Lexems::separators + Lexems::syntax_markers + Lexems::operators;

}