#ifndef __SYNTICS_CPP_HPP__
#define __SYNTICS_CPP_HPP__

#include <est1>
#include <est/struct_tree.hpp>
#include <jch/tree.hpp>
#include <jch/parsers.hpp>
#include <syntics/grammar.hpp>

namespace color { str green(const str&); str yellow(const str&); }
namespace syntics
{
    struct Base {virtual str to_str() const{ return "<base>"; }};
    struct Root   : Base { str to_str() const { return "root"; } Root(){} Root(Base*){} static Root* make(const str& s); };

    template<typename ModelClass>
    struct ParseModel
    {
        int beg,end;
        str opening,closing;
        ModelClass model;
        ParseModel() : beg(-1), end(-1) {}
        ParseModel(Base* b) : beg(-1), end(-1), model(b) {}
        ParseModel(int b,int e,const str& o,const str& c,Base* bc) : beg(b), end(e), opening(o), closing(c), model(bc) {}
        virtual str to_str(const str& text) const
        {
            if (beg == -1 || end == -1) return mkstr("\\\\BAD INDEX// || ",model()->to_str());
            return mkstr(".",text.substr(beg,end-beg+1),". || ",model()->to_str());
        }

        template<class ModelBaseClassDerived> bool is()       { return model.template is       <ModelBaseClassDerived> (); }
        template<class BaseClassDerived>      bool is_cross() { return model.template is_cross <BaseClassDerived>     (); }
        
        template<class BaseClassDerived> BaseClassDerived* cast() const { return dynamic_cast<BaseClassDerived*>(model()); }
    };

    struct ParserHelper {
        size_t total,total_ok,total_ko;
        str::list failures;
        void ok();
        void ko(const str&);
        void reset();
        ParserHelper ();
        str::list known_types;
        str::list user_classes;
        str::list user_structs;
        bool is_type(const str& name);
        void add_class(const str&);
        void add_struct(const str&);
        str::list types() const;
    };

    
    namespace syntax
    {

        struct Lexems
        {
            static const str::list      u_operators;
            static const str::list      b_operators;
            static const str::list      operators;
            static const str::list      block_markers;
            static const str::pair_list stoppers;
            static const str::list      separators;
            static const str::list      keywords;
            static const str::list      built_in_types;
            static const str::list      syntax_markers;
            static const str::list      qualifiers;
            static const str::list      known_tokens;
            static const str::list      instructions;
            static const str::list      controls;
        };

        namespace impl
        {
            template<int I> struct Block 
            {
                static bool match(const str& block_string) { return block_string.begs(Lexems::block_markers[I][0]) && block_string.ends(Lexems::block_markers[I][1]); }
                static str  to_str(const str&,bool){ str s = mkstr(Lexems::block_markers[I][0],Lexems::block_markers[I][1]); return s; }
            };

            template<int I,bool NotEnd> struct Annotation
            {
                static bool match(const str& block_string) { return block_string.begs(Lexems::stoppers[I].first.c_str()) && (NotEnd || block_string.ends(Lexems::stoppers[I].second.c_str())); }
                static str  to_str(const str&,bool){ str s = mkstr(Lexems::stoppers[I].first,Lexems::stoppers[I].second); return s; }
            };

            template<const str::list & AllowedValues>
            struct KnownToken
            {
                static constexpr const str::list& allowed_values = AllowedValues;
                static bool match(const str& sep) { return allowed_values.has(sep); }
            };

            struct Separator : KnownToken<Lexems::separators>{ static str to_str(const str& s,bool){ return mkstr("sep: ",s); } };
            struct Operator  : KnownToken<Lexems::operators> { static str to_str(const str& s,bool){ return mkstr("ope: ",s); } };
            struct Qualifier : KnownToken<Lexems::qualifiers>{ static str to_str(const str& s,bool){ return mkstr("qua: ",s); } };
            struct Keyword   : KnownToken<Lexems::keywords>  { static str to_str(const str& s,bool){ return mkstr("key: ",s); } };
            struct Marker    : KnownToken<Lexems::syntax_markers>  { static str to_str(const str& s,bool){ return mkstr("mar: ",s); } };
            struct LangType  : KnownToken<Lexems::built_in_types>  { static str to_str(const str& s,bool){ return mkstr("lty: ",s); } };
            struct Instruction  : KnownToken<Lexems::instructions>  { static str to_str(const str& s,bool b){ if (b) return color::green(s); return mkstr("ins: ",s); } };
            struct Control  : KnownToken<Lexems::controls>  { static str to_str(const str& s,bool b){ if (b) return color::green(s); return mkstr("con: ",s); } };

            struct Name { static bool match(const str& s) { return !Lexems::known_tokens.has(s); } static str to_str(const str& token,bool sem){ if (sem) return color::yellow(token); return mkstr("nam: ",token); } };
            struct Colon { static bool match(const str& s){ return s == ":"; } static str to_str(const str& token,bool sem){ return mkstr(" : "); }  };
            struct QuestionMark { static bool match(const str& s){ return s == "?"; } static str to_str(const str& token,bool sem){ return mkstr(" ? "); }  };
        }

        template<typename Impl,class BaseClass = Base>
        struct SyntaxBase : BaseClass {
            bool semantic;
            static SyntaxBase<Impl,BaseClass>* make(const str& s)
            {
                if (!Impl::match(s)) return nullptr;
                return new SyntaxBase<Impl,BaseClass>(s);
            }
            virtual str to_str() const { return Impl::to_str(token,semantic); }
            SyntaxBase(const str& t) : token(t), semantic(false) {}
            str token;// TODO: here bc im lazy. put back to private and refine classes
        private:
        };

        struct Annotation : Base { virtual str to_str() const { return Base::to_str(); } };

        using CBlock    = SyntaxBase<impl::Block<0>>;
        using PBlock    = SyntaxBase<impl::Block<1>>;
        using SBlock    = SyntaxBase<impl::Block<2>>;
        using Operator  = SyntaxBase<impl::Operator>;
        using Separator = SyntaxBase<impl::Separator>;
        using Qualifier = SyntaxBase<impl::Qualifier>;
        using Keyword   = SyntaxBase<impl::Keyword>;
        using Colon     = SyntaxBase<impl::Colon>;
        using QuestionMark     = SyntaxBase<impl::QuestionMark>;
        using Marker    = SyntaxBase<impl::Marker>;
        using Name      = SyntaxBase<impl::Name>;
        using Instruction     = SyntaxBase<impl::Instruction>;
        using Control     = SyntaxBase<impl::Control>;
        using LangType      = SyntaxBase<impl::LangType>;
        struct LBlock : Base { // TODO: better usage of this
            str token;
            static bool match(const str&){ return false; }
            static LBlock* make(const str& s){ return nullptr; }
            str to_str() const { return mkstr("{",token,":}"); }
            LBlock(const str& t = "") : token(t) {}
        };
        struct TBlock : Base { // TODO: better usage of this
            bool definition;
            static bool match(const str&){ return false; }
            static TBlock* make(const str& s){ return nullptr; }
            str to_str() const { return "<>"; }
            TBlock(bool def = false) : definition(def) {}
        };
        struct UserType : Name {
            bool is_class,is_struct;
            UserType(const str& t,bool ic,bool is) : Name(t), is_class(ic), is_struct(is) {}
            str to_str() const{ return mkstr("uty: ",token); }
        };
        struct TypeName : Base {
            str token;
            bool built_in;
            bool is_class,is_struct;
            TypeName(const LangType& lt) : token(lt.token), built_in(true), is_class(false), is_struct(false) {}
            TypeName(const UserType& ut) : token(ut.token), built_in(false), is_class(ut.is_class), is_struct(ut.is_struct) {}
            str to_str() const { return mkstr("typ: ",token); }
        };
        using OneLineComment   = SyntaxBase<impl::Annotation<0,true>, Annotation>;
        using MultiLineComment = SyntaxBase<impl::Annotation<1,false>, Annotation>;
        using Directive        = SyntaxBase<impl::Annotation<2,true>, Annotation>;
        /*struct PBlock     : Base { static PBlock*     make(const str& s);     PBlock(const str&); str to_str() const { return "()"; }};
        struct CBlock     : Base { static CBlock*     make(const str& s);     CBlock(const str&); str to_str() const { return "{}"; }};
        struct SBlock     : Base { static SBlock*     make(const str& s);     SBlock(const str&); str to_str() const { return "[]"; }};
        struct TBlock     : Base { static TBlock*     make(const str& s);     str to_str() const { return "<>"; }};
        struct LBlock     : Base { static LBlock*     make(const str& s);     str to_str() const { return "{:}"; }};
        struct StrLiteral : Base { static StrLiteral* make(const str& s); StrLiteral(const str&); str token;  str to_str() const { return token; }};
        struct Colon      : Base { static Colon*      make(const str& s);     Colon();           str to_str() const { return ":"; }};
        struct Separator  : Base { static Separator*  make(const str& s);  Separator(const str&); str token;  str to_str() const { return mkstr("sep: ",token); }};
        struct Operator   : Base { static Operator*   make(const str& s);   Operator(const str&); str token;  str to_str() const { return mkstr("ope: ",token); }};
        struct Keyword    : Base { static Keyword*    make(const str& s);    Keyword(const str&); str token;  str to_str() const { return mkstr("key: ",token); }};
        struct Qualifier  : Base { static Qualifier*  make(const str& s);  Qualifier(const str&); str token;  str to_str() const { return mkstr("qua: ",token); }};
        struct Name       : Base { static Name*       make(const str& s);       Name(const str&); str token;  str to_str() const { return mkstr("nam: ",token); }};
        struct BuiltType  : Base { static BuiltType*  make(const str& s);  BuiltType(const str&); str token;  str to_str() const { return mkstr("bit: ",token); }};
        struct Annotation : Base { static Annotation* make(const str& s); Annotation(const str&); str token;  str to_str() const { return mkstr("cmt: ",token); }};*/


        //using BaseUnit = StructTree<Base,Root,
        //                         Annotation,PBlock,CBlock,SBlock,LBlock,TBlock,StrLiteral,Colon,Separator,Keyword,Separator,Operator,Qualifier,Name>;
        using Unit = StructTree<Base,Root,OneLineComment,MultiLineComment,Directive,Instruction,Control,TBlock,LBlock,SBlock,PBlock,CBlock,Operator,Colon,QuestionMark,Marker,Separator,Qualifier,Keyword,LangType,Name>;
                                    //Annotation,PBlock,CBlock,SBlock,LBlock,TBlock>;//,StrLiteral,Colon,Separator,Keyword,Separator,Operator,Qualifier,Name>;

        /*struct Unit : public BaseUnit
        {
            Unit(Base*);
            Unit();
        };*/

        //using Units = vector<Unit>;
        using Model = ParseModel<Unit>;
        using Tree = jch::est::tree<Model>;

        struct AddChar{
            void operator()(Model& s,char c){  }
        };

        struct Parser : public jch::est::TreeParser<Model,AddChar>
        {
            ParserHelper& helper;
            void onOpen(Model& t,size_t pos);
            void onChild(Model& t,Model& parent,size_t pos);
            void onElderBrother(Model& t,size_t pos);
            void onYoungerBrother(Model& t,Model& parent,size_t pos);
            void onClose(Model& t,size_t pos);
            void onReturn(Model& t,size_t pos);
            void build(Model&);
            void with_syntax_context(syntax::Tree&);
            void start(const str&);
            Parser();
            Parser(ParserHelper&);
            bool checkEmptyElement(Model& t);
        };
    }

    namespace semantics
    {
        using namespace est::variadic::grammar::helpers;

        struct Unresolved : Base{
            str representation;
            Unresolved(const str& s);
            str to_str() const;
        };

        void init_types_conditions();

        struct Model;

        using Getter = std::function<Base*(const Model&)>;


        using FullTypeP   =  bparse::GrammarUnit< Base, Getter, _s_<List,     syntax::Qualifier>,
                                                               _s_<Required, syntax::TypeName>,
                                                               _s_<Optional, syntax::TBlock>,
                                                               _s_<Optional, syntax::Operator> >;

        using NamespaceP =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Keyword>,
                                                                    _s_<Optional, syntax::Name>,
                                                                    _s_<Required, syntax::CBlock> >;

        using ClassP     =  bparse::GrammarUnit< Base, Getter, _s_<Optional, syntax::TBlock>,
                                                               _s_<Required, syntax::Keyword>,
                                                               _s_<Required, syntax::Name>,
                                                               _s_<Optional, syntax::LBlock>,
                                                               _s_<Required, syntax::CBlock> >;

        using StructP     =  bparse::GrammarUnit< Base, Getter, _s_<Optional, syntax::TBlock>,
                                                                _s_<Required, syntax::Keyword>,
                                                                _s_<Optional, syntax::Name>,
                                                                _s_<Optional, syntax::LBlock>,
                                                                _s_<Required, syntax::CBlock> >;

        struct Namespace : public NamespaceP
        {
            syntax::Keyword& keyword;
            vector<syntax::Name*>& name;
            syntax::CBlock& cblock;
            Namespace(syntax::Keyword* k,vector<syntax::Name*> n,syntax::CBlock* c) : NamespaceP(k,n,c), keyword(*reftoken<0>()),
                                                                                                 name(reftoken<1>()),
                                                                                                 cblock(*reftoken<2>()){}
            str to_str() const; static void init();
        };

        struct Class : public ClassP
        {
            vector<syntax::TBlock*>& template_block;
            syntax::Keyword& keyword;
            syntax::Name& name;
            syntax::CBlock& cblock;
            vector<syntax::LBlock*>& lblock;
            Class(vector<syntax::TBlock*> t,syntax::Keyword* k,syntax::Name* n,vector<syntax::LBlock*> l,syntax::CBlock* c) : ClassP(t,k,n,l,c),
                                                                                                 template_block(reftoken<0>()),
                                                                                                 keyword(*reftoken<1>()),
                                                                                                 name(*reftoken<2>()),
                                                                                                 lblock(reftoken<3>()),
                                                                                                 cblock(*reftoken<4>()){}

            str to_str() const; static void init();
        };

        struct Struct : public StructP
        {
            vector<syntax::TBlock*>& template_block;
            syntax::Keyword& keyword;
            vector<syntax::Name*>& name;
            syntax::CBlock& cblock;
            vector<syntax::LBlock*>& lblock;
            //vector<syntax::Name*>& declnames;
           /* Struct(syntax::Keyword* k,vector<syntax::Name*> n,vector<syntax::LBlock*> l,syntax::CBlock* c,vector<syntax::Name*> dn) : StructP(k,n,l,c,dn), keyword(*reftoken<0>()),
                                                                                                 name(reftoken<1>()),
                                                                                                 lblock(reftoken<2>()),
                                                                                                 cblock(*reftoken<3>()),
                                                                                                 declnames(reftoken<4>()){}*/
            Struct(vector<syntax::TBlock*> t,syntax::Keyword* k,vector<syntax::Name*> n,vector<syntax::LBlock*> l,syntax::CBlock* c) : StructP(t,k,n,l,c), 
                                                                                                 template_block(reftoken<0>()),
                                                                                                 keyword(*reftoken<1>()),
                                                                                                 name(reftoken<2>()),
                                                                                                 lblock(reftoken<3>()),
                                                                                                 cblock(*reftoken<4>()){}

            str to_str() const; static void init();
        };

        struct FullType : public FullTypeP
        {
            vector<syntax::Qualifier*>& qualifiers;
            syntax::TypeName& name;
            vector<syntax::Operator*>& moperator;
            vector<syntax::TBlock*>& template_block;
            FullType(vector<syntax::Qualifier*> q,syntax::TypeName* n,vector<syntax::TBlock*>t,vector<syntax::Operator*> o) : FullTypeP(q,n,t,o),
                                                                                                 qualifiers(reftoken<0>()),
                                                                                                 name(*reftoken<1>()),
                                                                                                 template_block(reftoken<2>()),
                                                                                                 moperator(reftoken<3>()) {}

            str to_str() const; static void init();
        };

        using LambdaP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::SBlock>,
                                                               _s_<Required, syntax::PBlock>,
                                                               _s_<Optional, syntax::Operator>,
                                                               _s_<Optional, FullType>,
                                                               _s_<Required, syntax::CBlock> >;

        struct Lambda : public LambdaP
        {
            syntax::SBlock& capture;
            syntax::PBlock& arguments;
            syntax::CBlock& body;
            vector<FullType*>& returnType;
            Lambda(syntax::SBlock* c,syntax::PBlock* a,vector<syntax::Operator*> o,vector<FullType*> r,syntax::CBlock* b)
             : LambdaP(c,a,o,r,b), capture(*reftoken<0>()), arguments(*reftoken<1>()), returnType(reftoken<3>()), body(*reftoken<4>()) {}
            str to_str() const;
            static void init();
        };

        using VarDeclarationP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, FullType>,
                                                                    _s_<Required, syntax::Name> >;

        struct VarDeclaration : public VarDeclarationP
        {
            FullType& type;
            syntax::Name& name;
            VarDeclaration(FullType* t,syntax::Name* n) : VarDeclarationP(t,n), type(*reftoken<0>()), name(*reftoken<1>()) {}
            str to_str() const;
        };

        using FunctionP     =  bparse::GrammarUnit< Base, Getter, _s_<Optional, syntax::TBlock>,
                                                                  _s_<Required, FullType>,
                                                                  _s_<Required, syntax::Name>,
                                                                  _s_<Required, syntax::PBlock>,
                                                                  _s_<Optional, syntax::Qualifier>,
                                                                  _s_<Optional, syntax::CBlock> >;

        struct Function : public FunctionP
        {
            vector<syntax::TBlock*>& template_block;
            FullType& returnType;
            syntax::Name& name;
            syntax::PBlock& arguments;
            vector<syntax::CBlock*>& body;
            vector<syntax::Qualifier*>& qualifier;
            Function(vector<syntax::TBlock*> tb,FullType* t,syntax::Name* n,syntax::PBlock* a,vector<syntax::Qualifier*> q,vector<syntax::CBlock*> b) : FunctionP(tb,t,n,a,q,b), 
                template_block(reftoken<0>()), returnType(*reftoken<1>()), name(*reftoken<2>()),arguments(*reftoken<3>()),qualifier(reftoken<4>()),body(reftoken<5>()) {}
            str to_str() const;
        };

        using OperatorOLP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, FullType>,
                                                                     _s_<Required, syntax::Keyword>,
                                                                     _s_<Required, syntax::Operator>,
                                                                     _s_<Required, syntax::PBlock>,
                                                                     _s_<Optional, syntax::Qualifier>,
                                                                     _s_<Optional, syntax::CBlock> >;

        struct OperatorOL : public OperatorOLP
        {
            FullType& returnType;
            syntax::Operator& name;
            syntax::PBlock& arguments;
            vector<syntax::CBlock*>& body;
            vector<syntax::Qualifier*>& qualifier;
            OperatorOL(FullType* t,syntax::Keyword* n,syntax::Operator* o,syntax::PBlock* a,vector<syntax::Qualifier*> q,vector<syntax::CBlock*> b) : OperatorOLP(t,n,o,a,q,b), 
                returnType(*reftoken<0>()), name(*reftoken<2>()),arguments(*reftoken<3>()),qualifier(reftoken<4>()),body(reftoken<5>()) {}
            str to_str() const;
            static void init();
        };

        using OperatorCVP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Keyword>,
                                                                    _s_<Required, FullType>,
                                                                     _s_<Required, syntax::PBlock>,
                                                                     _s_<Optional, syntax::Qualifier>,
                                                                     _s_<Optional, syntax::CBlock> >;

        struct OperatorCV : public OperatorCVP
        {
            FullType& type;
            vector<syntax::CBlock*>& body;
            vector<syntax::Qualifier*>& qualifier;
            OperatorCV(syntax::Keyword* n,FullType* t,syntax::PBlock* a,vector<syntax::Qualifier*> q,vector<syntax::CBlock*> b) : OperatorCVP(n,t,a,q,b), 
                type(*reftoken<1>()), qualifier(reftoken<3>()),body(reftoken<4>()) {}
            str to_str() const;
            static void init();
        };

        using ColonExpansionP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, FullType>,
                                                                        _s_<Required, syntax::Name>,
                                                                        _s_<Required, syntax::Colon>,
                                                                        _s_<Required, syntax::Name> >;

        struct ColonExpansion : public ColonExpansionP
        {
            FullType& type;
            syntax::Name& iterator;
            syntax::Name& expanded;
            ColonExpansion(FullType* t,syntax::Name* i,syntax::Colon* c,syntax::Name* e) : ColonExpansionP(t,i,c,e), 
                type(*reftoken<0>()), iterator(*reftoken<1>()), expanded(*reftoken<3>()) {}
            str to_str() const;
        };

        using ConstructorP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, FullType>,
                                                                     _s_<Required, syntax::PBlock>,
                                                                     _s_<Optional, syntax::LBlock>,
                                                                     _s_<Optional, syntax::CBlock> >;

        struct Constructor : public ConstructorP
        {
            FullType& name;
            syntax::PBlock& arguments;
            vector<syntax::LBlock*>& initializer_list;
            vector<syntax::CBlock*>& body;
            Constructor(FullType* n,syntax::PBlock* a,vector<syntax::LBlock*> l,vector<syntax::CBlock*> c) : ConstructorP(n,a,l,c),
                name(*reftoken<0>()), arguments(*reftoken<1>()), initializer_list(reftoken<2>()),body(reftoken<3>()) {}
            str to_str() const; 
            static void init();
        };

        using NamedCallP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Name>,
                                                                   _s_<Optional, syntax::TBlock>,
                                                                   _s_<Required, syntax::PBlock> >;

        struct NamedCall : public NamedCallP
        {
            syntax::Name& name;
            syntax::PBlock& arguments;
            vector<syntax::TBlock*>& template_block;
            NamedCall(syntax::Name* n,vector<syntax::TBlock*> t,syntax::PBlock* a) : NamedCallP(n,t,a),
                name(*reftoken<0>()), template_block(reftoken<1>()), arguments(*reftoken<2>()){}
            str to_str() const; 
        };

        using TypedCallP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, FullType>,
                                                                   _s_<Required, syntax::PBlock> >;

        struct TypedCall : public TypedCallP
        {
            FullType& name;
            syntax::PBlock& arguments;
            TypedCall(FullType* n,syntax::PBlock* a) : TypedCallP(n,a),
                name(*reftoken<0>()), arguments(*reftoken<1>()){}
            str to_str() const; 
        };

        using IndexP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Name>,
                                                                   _s_<Required, syntax::SBlock> >;

        struct Index : public IndexP
        {
            syntax::Name& name;
            syntax::SBlock& arguments;
            Index(syntax::Name* n,syntax::SBlock* a) : IndexP(n,a),
                name(*reftoken<0>()), arguments(*reftoken<1>()){}
            str to_str() const; 
        };

        template<typename...Specs>
        using _g = bparse::GrammarUnit <Base,Getter,Specs...>;

        using EnumClassP     =  _g< _s_< Required, syntax::Keyword >,
                                    _s_< Required, syntax::Keyword >,
                                    _s_< Required, syntax::Name    >,
                                    _s_< Required, syntax::CBlock  >,
                                    _s_< Optional, syntax::Name    > >;

        struct EnumClass : public EnumClassP
        {
            syntax::Name&   name;
            vector<syntax::Name*>&   declname;
            syntax::CBlock& values;

            EnumClass (syntax::Keyword* k1,syntax::Keyword* k2,syntax::Name* n,syntax::CBlock* a,vector<syntax::Name*> d) : EnumClassP(k1,k2,n,a,d),
                name(*reftoken<2>()), values(*reftoken<3>()), declname(reftoken<4>()){}
            str to_str() const;
            static void init();
        };

        using ForwardDeclarationP =  _g< _s_< Required, syntax::Keyword >,
                                        _s_< Required, syntax::Name    > >;

        struct ForwardDeclaration : public ForwardDeclarationP
        {
            syntax::Keyword& keyword;
            syntax::Name&   name;

            ForwardDeclaration (syntax::Keyword* k,syntax::Name* n) : ForwardDeclarationP(k,n),
                keyword(*reftoken<0>()), name(*reftoken<1>()){}
            str to_str() const;
            static void init();
        };

        using TypenameP = _g< _s_< Required, syntax::Keyword >,
                              _s_< Optional, syntax::Marker >,
                              _s_< Required, syntax::Name    > >;

        struct Typename : public TypenameP
        {
            syntax::Keyword& keyword;
            syntax::Name&   name;

            Typename (syntax::Keyword* k,vector<syntax::Marker*> m,syntax::Name* n) : TypenameP(k,m,n),
                keyword(*reftoken<0>()), name(*reftoken<2>()){}
            str to_str() const;
            static void init();
        };

        using ConditionalControlWithBodyP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Control>,
                                                                                  _s_<Required, syntax::PBlock>,
                                                                                  _s_<Required, syntax::CBlock> >;
        struct ConditionalControlWithBody : public ConditionalControlWithBodyP
        {
            syntax::Control& name;
            syntax::PBlock& arguments;
            syntax::CBlock& body;
            ConditionalControlWithBody(syntax::Control* n,syntax::PBlock* a,syntax::CBlock* b) : ConditionalControlWithBodyP(n,a,b),
                name(*reftoken<0>()), arguments(*reftoken<1>()), body(*reftoken<2>()){}
            str to_str() const; 
        };
        using ConditionalControlNoBodyP     =  bparse::GrammarUnit< Base, Getter, _s_<Required, syntax::Control>,
                                                                                  _s_<Required, syntax::PBlock> >;
        struct ConditionalControlNoBody : public ConditionalControlNoBodyP
        {
            syntax::Control& name;
            syntax::PBlock& arguments;
            ConditionalControlNoBody(syntax::Control* n,syntax::PBlock* a) : ConditionalControlNoBodyP(n,a),
                name(*reftoken<0>()), arguments(*reftoken<1>()){}
            str to_str() const; 
        };

        using Unit = StructTree<Base,Root,
                                        Namespace>;
        /*struct Unit : public BaseUnit
        {
            Unit();
            Unit(Base*);
        };*/

        struct Model : public ParseModel<Unit>
        {
            Model(const syntax::Model&);
            Model();
            str to_str(const str&) const;
            bool is_binary_operator() const;
            bool is_unary_operator() const;
            bool is_separator() const;
            bool is_expression_separator() const;
            bool is_colon() const;
            bool is_question_mark() const;
        };

        using Tree = jch::est::tree<Model>;

        //vector<syntax::Annotation*> collect_annotations(Tree&,Tree::iterator);
    }

    namespace cpp {

       /* syntax::Tree    tokenize(const str& text);
        semantics::Tree find_semantics(const syntax::Tree&,ParseHelper* p = nullptr);
        syntax::Tree    syntax_flatten(const semantics::Tree&);*/

        semantics::Tree parse(const str&,ParserHelper* p = nullptr);
        str flatten(const semantics::Tree&);
    }

}

#endif