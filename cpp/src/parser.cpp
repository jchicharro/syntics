#include <syntics/cpp.hpp>

namespace syntics
{
    
    ParserHelper::ParserHelper() : total(0), total_ok(0), total_ko(0) {}

    void ParserHelper::ok() { ++total; ++total_ok; }
    void ParserHelper::ko(const str& s) { ++total; ++total_ko; failures.push_back(s); }
    void ParserHelper::reset() { total = 0; total_ok = 0; total_ko = 0;failures.clear(); }

    bool ParserHelper::is_type(const str& name){ return user_classes.has(name) || user_structs.has(name); }
    void ParserHelper::add_class(const str& s){ user_classes.push_back(s); }
    void ParserHelper::add_struct(const str& s){ user_structs.push_back(s); }

    str::list ParserHelper::types() const { return user_classes + user_structs; }

    Root* Root::make(const str& s){ return nullptr; }
}

namespace syntics::syntax
{
    

    void Parser::onOpen(Model& t,size_t pos) { t.end = pos-1; }

    void Parser::onChild(Model& t,Model& parent,size_t pos)
    {
        t.beg = pos;
        //parent.model.add_child(t.model());
    }
    void Parser::onElderBrother(Model& t,size_t pos) { t.end = pos; build(t); }
    void Parser::onYoungerBrother(Model& t,Model& parent,size_t pos)
    {
        t.beg = pos; 
        //parent.model.add_child(t.model());
    }
    void Parser::onClose(Model& t,size_t pos) { t.end = pos; build(t); }
    void Parser::onReturn(Model& t,size_t pos) {t.end = pos; }

    void Parser::build(Model& t)
    {
        if (t.end >= text->size()) return; // TODO: revise why this happens
        if (t.beg == -1 || t.end == -1) return; // TODO: same as above
        //print(mkstr("building .",text->substr(t.beg,t.end-t.beg+1),". ",t.beg,":",t.end));

        t.model.try_make<str>(text->substr(t.beg,t.end-t.beg+1));
        //print("built");
    }

    void Parser::start(const str& text){
        jch::est::TreeParser<Model,AddChar>::start(text);
        semantics::Namespace::init();
        semantics::Class::init();
        semantics::Constructor::init();
        semantics::FullType::init();
        semantics::Struct::init();
        semantics::ForwardDeclaration::init();
        semantics::Typename::init();
        semantics::Lambda::init();
        semantics::OperatorOL::init();
        semantics::OperatorCV::init();
    }

    Parser::Parser(ParserHelper& h) : jch::est::TreeParser<Model,AddChar>(), helper(h) {
        //tree_delimiters = { {"(",")"} ,{"{","}"},{"[","]"} };
        str::pair_list delims;
        for(const str& pair : Lexems::block_markers)
            tree_delimiters.push_back( { str(pair[0]),str(pair[1]) } );
        //tree_delimiters = syntics::Lexblockers;
        //separators = {";",",","=","*","-","&&"};
        //this->separators = all_tokens;
        
        //cpp_known_types     = built_in_types;
        this->separators = Lexems::separators + Lexems::operators + Lexems::syntax_markers;
        this->known_tokens = Lexems::separators + Lexems::operators + Lexems::syntax_markers;//all_tokens;
        parse_delimiters = {};
        this->stoppers = Lexems::stoppers;
        this->gluers = {"::"};
    }
    bool Parser::checkEmptyElement(Model& t)
    {
        if (t.end == -1 || t.beg == -1) return false;
        if (t.beg > t.end) return true;
        str stre = text->substr(t.beg,t.end-t.beg+1);
        stre.erase(std::remove(stre.begin(),stre.end(),' '),stre.end());
        return stre.empty();
    }

    void Parser::with_syntax_context(syntax::Tree& syntax_tree)
    {
        auto is_inhericante_qualifier = [](const syntax::Model& m)->bool{
            if (syntax::Keyword* k = m.cast<Keyword>())
                return str::list{"public","private","protected"}.has(k->token);
            return false;
        };
        auto inheritance_qualifiers_list = [&syntax_tree,is_inhericante_qualifier](syntax::Tree::iterator it)->bool
        {
            //print(mkstr("propcheck colon: ",it->model()->to_str()));
            if (!it.down()) return false;
            //print(mkstr("propcheck colon: ",it->model()->to_str()));
            auto it_hq = it.find_next(is_inhericante_qualifier);

            if (!it_hq) return false;

            const str qualifier = reinterpret_cast<Keyword*>(it_hq->model())->token;

            if (!it_hq.next()) return false; // TODO: consider malformed

            Colon* colon = it_hq->cast<Colon>();
            if (!colon) return false; // TODO: consider malformed

            it_hq.prev();
            it_hq.addBeforeBrother(syntax::Model(new LBlock(qualifier)));
            
            if (!(it_hq + 2)) return true;
            auto it_last = it.find_next(is_inhericante_qualifier,it_hq+2);
            if (it_last) --it_last;
            else{
                it_last = it_hq;
                it_last.golast();
            }
            (it_hq-1).absorbChildren(it_hq+2,it_last); // +2 to exclude "public :"

            syntax_tree.erase(it_hq+1);
            syntax_tree.erase(it_hq);
            return true;
        };
        auto check_colon_property_list = [&syntax_tree](syntax::Tree::iterator it)->bool
        {
            //print(mkstr("propcheck colon: ",it->model()->to_str()));
            if (!it.down()) return false;
            //print(mkstr("propcheck colon: ",it->model()->to_str()));
            auto [it_beg,it_end] = it.find_range(&syntax::Model::is<Colon>,
                                                    &syntax::Model::is<CBlock>);
            if (!it_beg || !it_end) return false;

            auto itc = it_beg;
            while (itc != it_end)
            {
                if (syntax::Separator* sep = itc->cast<Separator>())
                    if (sep->token == ";")
                        return false;
                itc.next();
            }
            it = it_beg;
            it.addBeforeBrother(syntax::Model(new LBlock()));

            if (it_beg == it_end + 1) return true; // TODO: consider exception or error, empty : block, invalid syntax
            //it.addFirstChild(syntax::Model(new Template()));
            it.prev();
            it.absorbChildren(it_beg+1,it_end-1); // +1 to exclude ":"

            syntax_tree.erase(it_beg);
            return true;
        };

        auto is_less_than        = [](const syntax::Model& m)->bool{ if (syntax::Operator* s = m.cast<Operator>()) return s->token == "<"; return false;};
        auto is_greater_than     = [](const syntax::Model& m)->bool{ if (syntax::Operator* s = m.cast<Operator>()) return s->token == ">"; return false;};
        auto is_template_keyword = [](const syntax::Model& m)->bool{ if (syntax::Keyword* s = m.cast<Keyword>()) return s->token == "template"; return false;};
        auto check_templates = [&syntax_tree,is_less_than,is_greater_than,is_template_keyword](syntax::Tree::iterator it)->bool
        {
            if (!it.down()) return false;

            auto [it_beg,it_end] = it.find_range(is_less_than,
                                                 is_greater_than);
            if (!it_beg || !it_end) return false;
            it = it_beg;
            auto itp = it;
            const bool is_template_definition = itp.prev() && is_template_keyword(*itp);
            if (is_template_definition) it.prev();
            it.addBeforeBrother(syntax::Model(new TBlock(is_template_definition)));
            if (it_beg + 1 != it_end){
                it.prev();
                it.absorbChildren(it_beg+1,it_end-1);
            }
            syntax_tree.erase(it_beg);
            syntax_tree.erase(it_end);
            if (is_template_definition)
                syntax_tree.erase(it+1);
            return true;
        };

        ParserHelper& hhelper = helper;

        auto is_type_qualifier = [](const Qualifier& q) { return false; };//return q.token == "const" || q.token == "static" || q.token == "virtual"; };
        auto is_type_operator  = [](const Operator&  o) { return false; };//return o.token == "*" || o.token == "&"; };
        //auto known_type        = [&hhelper](const Name&      n) {return false; };// return built_in_types.has(n.token) || hhelper.is_type(n.token); };
        auto is_class          = [&hhelper](const str& n){return false; };// return hhelper.user_classes.has(n); }; 
        auto is_struct         = [&hhelper](const str& n){return false; };// return hhelper.user_classes.has(n); }; 

        semantics::Getter gtr  = [](const semantics::Model& model)->Base* { return model.model(); };

        auto check_full_types = [&gtr, &syntax_tree,itq = is_type_qualifier,/*kt = known_type,*/ito = is_type_operator,is_class,is_struct](syntax::Tree::iterator it)->bool
        {
            auto itp = it;
            if (!it.down()) return false;
            
            auto it_end = itp / -1;

            do
            {
                syntax::Tree::iterator::range range ( it, it_end );
                if (semantics::FullType::try_syntax<semantics::FullType,syntax::Model,syntax::TBlock>( gtr, range, false )) return true;
            }
            while (it.next());
            return false;
        };

        auto is_operator_keyword = [](const syntax::Model& m)->bool { if (syntax::Keyword* k = m.cast<Keyword>()) return k->token == "operator"; return false; }; 

        auto collapse_operators = [is_operator_keyword](syntax::Tree::iterator it)->void
        {    
            if (!it.down()) return;

            do 
            {
                it = it.find_next(is_operator_keyword);
                if (!it) break;
                if (!it.next()) break;
                print(it->model()->to_str());
                if (it->cast<syntax::SBlock>())
                    it->model.set<Operator>("[]");
                else if (it->cast<syntax::PBlock>())
                    it->model.set<Operator>("()");
                
            } while (it.next());
        };

        auto check_known_types = [&hhelper](syntax::Tree::iterator it)->void
        {
            if (!it.down()) return;

            do
            {
                if (Name* name = dynamic_cast<Name*>(it->model()))
                {
                    str typenam  = name->token;
                    int colcol = typenam.rfind("::");
                    str lastname;
                    if (colcol == -1) lastname = typenam;
                    else{
                        //print(mkstr("colons found in ",typenam));
                        lastname = typenam.substr(colcol+2);
                        //print(mkstr("lastname ",lastname));
                    }
                    if (hhelper.is_type(lastname)){
                        bool is_class  = hhelper.user_classes.has(lastname);
                        bool is_struct = hhelper.user_structs.has(lastname);
                        it->model.set<UserType>(typenam,is_class,is_struct);
                    }

                    else name->semantic = true;
                }
                else if (Instruction* ins = dynamic_cast<Instruction*>(it->model()))
                {
                    ins->semantic = true;
                }
                else if (Control* ins = dynamic_cast<Control*>(it->model()))
                {
                    ins->semantic = true;
                }
            }
            while (it.next());
        };

        auto convert_types = [is_operator_keyword](syntax::Tree::iterator it)->void
        {
            if (!it.down()) return;
            do
            {
                if (UserType* ut = dynamic_cast<UserType*>(it->model()))
                {
                    auto itp = it;
                    if (itp.prev() && itp->model.is_cross<Keyword>() && !is_operator_keyword(*itp)) continue;
                    it->model.set<TypeName>(*ut);
                }
                else if (LangType* lt = dynamic_cast<LangType*>(it->model()))
                    it->model.set<TypeName>(*lt);
            }
            while (it.next());
        };

        //print("PARSING CONTEXT");
        //print(join(hhelper.user_classes,' '));
        syntax_tree.iterate_it
        (
            [check_colon_property_list,check_templates,check_full_types,check_known_types,convert_types,inheritance_qualifiers_list,collapse_operators]
            (syntax::Tree::iterator it)
            {
                check_known_types(it);
                convert_types(it);
                while (check_templates(it)) {}
                while (check_full_types(it)) {}
                collapse_operators(it);
                while (inheritance_qualifiers_list(it)) {}
                while (check_colon_property_list(it)) {}
            }
        );
    }
}