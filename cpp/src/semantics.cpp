#include <syntics/cpp.hpp>

template<int Icond,class Grammar,class Syntax>
void set_condition(const str& value)
{
    std::get<Icond>(Grammar::conditions) = [value](const Syntax& s){ return s.token == value; };
}


namespace color
{
    str fg_green(const str& s)        { return str("\033[0;35m") + s + str("\033[0m"); }
    str bf_black_green(const str& s)  { return str("\033[32;40m") + s + str("\033[0m"); }
    str bf_blue_white(const str& s)   { return str("\033[37;44m") + s + str("\033[0m"); }
    str bf_red_white(const str& s)    { return str("\033[37;41m") + s + str("\033[0m"); }
    str bf_black_red(const str& s)    { return str("\033[31;40m") + s + str("\033[0m"); }
    str bf_yellow_black(const str& s) { return str("\033[30;103m") + s + str("\033[0m"); }

    str yellow(const str& s) { return str("\033[40;93m") + s + str("\033[0m"); }
    str blue(const str& s) { return str("\033[40;34m") + s + str("\033[0m"); }
    str red(const str& s)    { return str("\033[31;40m") + s + str("\033[0m"); }
    str green(const str& s)        { return str("\033[0;35m") + s + str("\033[0m"); }
}

using namespace color;

namespace syntics::semantics
{
    using namespace syntax;

    Unresolved::Unresolved(const str& s) : representation(s) {}
    str Unresolved::to_str() const { return mkstr(bf_red_white("<unresolved-semantic-expression>")," : ",representation); }

    str Namespace::to_str() const { return red(mkstr("namespace ",name.empty() ? "<annonymous>" : name[0]->token)); }
    void Namespace::init() { set_condition<0,Namespace,Keyword>("namespace"); }

    str Class::to_str() const { return yellow(mkstr("class ",name.token)); }
    void Class::init() { set_condition<1,Class,Keyword>("class"); }

    str Struct::to_str() const { return yellow(mkstr("struct ",name.empty() ? "<annonymous>" : name[0]->token)); }
    void Struct::init() { set_condition<1,Struct,Keyword>("struct"); }

    str FullType::to_str() const {
        str::list quas = qualifiers.transform<str>([](const Qualifier* q) { return q->token; });
        str mop = moperator.empty()? "" : moperator[0]->token;
        return mkstr(join(quas,' '),quas.empty() ? "" : " ",name.token,mop);
    }
    void FullType::init() { std::get<3>(FullType::conditions) = [](const Operator& s){ return s.token == "*" || s.token == "&" || s.token == "&&"; }; }

    str VarDeclaration::to_str() const {
        return yellow(mkstr(type.to_str()," ",name.to_str()," : var-declaration"));
    }
    str Function::to_str() const {
        const bool declaration = body.empty();
        if (declaration)
            return yellow(mkstr(returnType.to_str()," ",name.to_str(), "() : function-declaration"));
        return blue(mkstr(returnType.to_str()," ",name.to_str(), "() : function-definition"));
    }

    str Constructor::to_str() const {
        const bool declaration = body.empty();
        if (declaration)
            return yellow(mkstr(name.to_str(), "() : constructor-declaration"));
        return blue(mkstr(name.to_str(), "() : constructor-definition"));
    }
    void Constructor::init(){ std::get<0>(Constructor::conditions) = [](const FullType& s){ return s.name.is_class || s.name.is_struct; }; }

    
    str NamedCall::to_str() const {
        return blue(mkstr(name.to_str(), "() : named-call"));
    }
    
    str TypedCall::to_str() const {
        return blue(mkstr(name.to_str(), "() : typed-call"));
    }
    str Index::to_str() const {
        return blue(mkstr(name.to_str(), "[] : index"));
    }
    str ConditionalControlNoBody::to_str() const {
        return blue(mkstr(name.to_str(), "() : conditional-control-no-body"));
    }
    str ConditionalControlWithBody::to_str() const {
        return blue(mkstr(name.to_str(), "() : conditional-control-with-body"));
    }

    str EnumClass::to_str() const {
        return yellow(mkstr(name.to_str(), "{...} : enum-class-declaration"));
    }
    void EnumClass::init() { set_condition<0,EnumClass,Keyword>("enum"); set_condition<1,EnumClass,Keyword>("class"); }

    str ForwardDeclaration::to_str() const {
        return yellow(mkstr(keyword.token," ",name.to_str(), " : forward-declaration"));
    }
    void ForwardDeclaration::init() { std::get<0>(ForwardDeclaration::conditions) = [](const Keyword& s){ return s.token == "class" || s.token == "struct"; }; }

    str Typename::to_str() const {
        return yellow(mkstr(name.to_str(), " : typename"));
    }
    void Typename::init() { std::get<0>(Typename::conditions) = [](const Keyword& s){ return s.token == "typename"; };
                            std::get<1>(Typename::conditions) = [](const Marker& s){ return s.token == "..."; }; }

    str Lambda::to_str() const
     { return yellow("lambda"); }
    void Lambda::init() { set_condition<2,Lambda,Operator>("->"); }

    str OperatorOL::to_str() const{
        return yellow(mkstr(name.token," : operator-overload"));
    }
    void OperatorOL::init(){
        std::get<1>(OperatorOL::conditions) = [](const Keyword& o){ return o.token == "operator"; };
        std::get<2>(OperatorOL::conditions) = [](const Operator& o){ return Lexems::operators.has(o.token) || o.token == "()" || o.token == "[]"; };
    }

    str OperatorCV::to_str() const{
        return yellow(mkstr(type.to_str()," : implicit-conversion"));
    }
    void OperatorCV::init(){
        std::get<0>(OperatorCV::conditions) = [](const Keyword& o){ return o.token == "operator"; };
    }

    str ColonExpansion::to_str() const{
        return blue(mkstr(iterator.to_str()," : ",expanded.to_str()," : colon-expansion"));
    }
}