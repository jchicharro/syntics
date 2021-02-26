#include <syntics/cpp.hpp>

namespace syntics::semantics
{

    
    Model::Model(const syntax::Model& sm) : ParseModel<Unit>(sm.beg,sm.end,sm.opening,sm.closing,sm.model()) {} // TODO i.e. copy pointer. need to consider ownership here
    Model::Model() : ParseModel<Unit>(new Base()) {}

    str Model::to_str(const str& text) const{
        return model()->to_str();
    }
    
    bool Model::is_binary_operator() const { if (auto* o = cast<syntax::Operator>()) return syntax::Lexems::b_operators.has(o->token); return false; }
    bool Model::is_unary_operator() const  { if (auto* o = cast<syntax::Operator>()) return syntax::Lexems::u_operators.has(o->token); return false; }
    bool Model::is_separator() const  { return cast<syntax::Separator>(); }
    bool Model::is_expression_separator() const  { return cast<syntax::Separator>() || cast<syntax::CBlock>() ; }
    bool Model::is_colon() const  { return cast<syntax::Colon>(); }
    bool Model::is_question_mark() const  { return cast<syntax::QuestionMark>(); }

    using Semit = semantics::Tree::iterator;

    using Itrange = semantics::Tree::iterator::range;

    Semit binary_expression_match(const str::list& operators,Itrange& range,Semit& beg_right)
    {
       // return false;
        //auto [left,right] = range.split(&Model::is_binary_operator);

        //if (range.begs+1==range.ends) return false;

        Itrange left,right;
        bool found(false);
        for(const str& op : operators){
            //auto [left,right] = range.split(&Model::is_binary_operator);
            std::tie(left, right) = range.split([&op](const Model& m)->bool{ if (syntax::Operator* ope = m.cast<syntax::Operator>()) return ope->token == op; return false; });
            if (left.begs) {
                found = true;
                break;
            }
        }

        if (!found) return false;
        if (left.ends == range.ends - 1 || right.begs - 1 == range.begs) return false;

        //if (((left.ends+1)->cast<syntax::Operator>())->token != op) return false;

        beg_right = right.begs;

        auto it_ope = left.ends+1;
        range.begs.addBeforeBrother(Model((beg_right-1)->model()));
        auto it_new = range.begs-1;
        it_new.absorb(range);
        //it_new.erase_children_if(&Model::is_binary_operator); // TODO: improve, should remain only 1 
        it_ope.erase();
        return it_new;
    }

    Semit instruction_expression_match(Itrange& range,Semit& member,ParserHelper& helper)
    {
        //if (range.begs + 1 == range.ends) return false;

        syntax::Instruction* ins = range.begs->cast<syntax::Instruction>();
        if (!ins) return false;

        range.begs.addBeforeBrother(Model(range.begs->model()));

        auto it_new = range.begs-1;
        auto it_ins = range.begs;
        it_new.absorb(range);
        (it_new / 0).erase();
        member = it_new / 0;

        if (ins->token == "using")
        {
            if (syntax::Name* name = member->cast<syntax::Name>())
                helper.add_struct(name->token);
            // TODO: consider else exception here
        }

        return it_new;
    }
    Semit control_expression_match(semantics::Getter gtr, Itrange& range,Semit& member)
    {
        //if (range.begs + 1 == range.ends) return false;

        syntax::Control* ins = range.begs->cast<syntax::Control>();
        if (!ins) return false;
        Semit it_new;
        auto it_last_end = range.ends;
        print(it_last_end->model()->to_str());
        if ( syntax::PBlock* condition = (range.begs+1)->cast<syntax::PBlock>())
        {
            range.begs.addBeforeBrother(Model());
            it_new = range.begs-1;
            it_new->model.set<ConditionalControlNoBody>(ins,condition);
            auto it_ins = range.begs;
            it_new.absorb(range);
            (it_new / 0).erase();
            member = it_new / 1;
            return it_new;
            /*print(it_new->model()->to_str());
            print((it_new/-1)->model()->to_str());
            print(range.ends->model()->to_str());
            if ((it_new / -1) != range.ends)
            {
                auto remfirst = range.ends;
                remfirst.gofirst();
                print(remfirst->model()->to_str());
                Itrange remaining { remfirst, range.ends };
                it_new.absorb(range);
            }
            return it_new;*/
        }

        range.begs.addBeforeBrother(Model(range.begs->model()));

        it_new = range.begs-1;
        auto it_ins = range.begs;
        it_new.absorb(range);
        (it_new / 0).erase();
        member = it_new / 0;
        return it_new;
        /*syntax::Control* ins = range.begs->cast<syntax::Control>();
        if (!ins) return false;
        Semit it_new;
        /*if ( it_new = ConditionalControlNoBody  ::try_syntax<ConditionalControlNoBody,Model,syntax::PBlock>( gtr, range, false))
        {
            if ((it_new / -1) != range.ends)
            {
                auto remfirst = range.ends;
                remfirst.gofirst();
                Itrange remaining { remfirst, range.ends };
                it_new.absorb(range);
            }
            return it_new;
        }else{
            *//*print("hola");
            range.begs.addBeforeBrother(Model(range.begs->model()));
            it_new = range.begs-1;
            auto it_ins = range.begs;
            it_new.absorb(range);
            (it_new / 0).erase();
            member = it_new / 0;
            print("bye");
            return it_new;
        //}
        return false;*/

    }
    Semit unary_expression_match(Itrange& range,Semit& member)
    {
      //  if ( range.size() != 2 ) return false;
        
        bool is_left  = range.begs->is_unary_operator();
        bool is_right = range.ends->is_unary_operator();
        if (!is_left && !is_right) return false;
        //if (!is_left && !is_right) return false;

        //member = is_left? range.ends : range.begs;
        
        auto ope = is_left? range.begs : range.ends;
        range.begs.addBeforeBrother(Model(ope->model()));

        auto it_new = range.begs-1;
        it_new.absorb(range);
        if (is_left)
            (it_new / 0).erase();
        else
            (it_new / -1).erase();
        member = it_new / 0;

        return it_new;

    }
    Semit boolean_assignment(Itrange& range,Semit& out1,Semit& out2)
    {
      //  if ( range.size() != 2 ) return false;
      
        Semit question_mark = range.begs.find_next_max(&Model::is_question_mark,range.ends);
        if (!question_mark || question_mark == range.begs) return false;
        Semit colon = question_mark.find_next_max(&Model::is_colon,range.ends);
        if (!colon) return false;

        range.begs.addBeforeBrother(Model(question_mark->model()));
        auto it_new = range.begs-1;
        (range.begs-1).absorb(range);
        out1 = question_mark-1;
        out2 = colon -1;

        question_mark.erase();
        colon.erase();

        return it_new;
    }

    void solve_control_structures(Itrange& range){
     
        syntax::Control* ins = range.begs->cast<syntax::Control>();
        if (!ins) return;
        auto itn = range.begs;
        if (!itn.next()) return;

    }

    size_t solve_expression(ParserHelper& helper,Itrange& range)
    {
        using namespace semantics;
        using namespace syntax;
        size_t absorbed_children(0);
        str::list s;
        
        for (const auto& m : range)
            s.push_back(m.model()->to_str());
        bool solved(true);
        if (range.begs == range.ends || s.empty()){
            //print("->empty");
            return 0;
        }
        print(mkstr("attempting to solve ",join(s,'|')));
        semantics::Tree::iterator it_aux, it_aux2;
        const bool debug(false);

        semantics::Getter gtr  = [](const semantics::Model& model)->Base* { return model.model(); };

        solve_control_structures(range);
        /*if (Semit it_new = ConditionalControlWithBody::try_syntax<ConditionalControlWithBody,Model,PBlock,CBlock>(gtr, range)){}
        else if (Semit it_new = control_expression_match(gtr, range, it_aux)){
            Itrange expressee = {it_aux, it_new / -1};
            solve_expression(helper, expressee);
        }*/
        if (Semit it_new = instruction_expression_match(range, it_aux, helper)){
            Itrange expressee = {it_aux, it_new / -1};
            solve_expression(helper, expressee);
        }
        else if (Semit it_new = Lambda::try_syntax<Lambda,Model,CBlock,PBlock,SBlock>(gtr, range)) {  }
        else if (Semit it_new = OperatorOL::try_syntax<OperatorOL,Model,PBlock,CBlock>(gtr, range)) {  }
        else if (Semit it_new = OperatorCV::try_syntax<OperatorCV,Model,PBlock,CBlock>(gtr, range)) {  }
        
        else if (Semit it_new = binary_expression_match(str::list{"="},range, it_aux))
        {
            Itrange left_member  ( it_new/0, it_aux-1 );
            Itrange right_member ( it_aux,   it_new/-1 );
            
            solve_expression(helper, left_member);
            solve_expression(helper, right_member);
        }
        else if (Semit it_new = boolean_assignment(range, it_aux, it_aux2)){
            print("is bollass");
            print(it_aux);
            print(it_aux2);
            print(it_new);
            print(it_aux->model()->to_str());
            print(it_aux2->model()->to_str());
            print(it_new->model()->to_str());
            print("--");
            Itrange boolean_expression = {it_new / 0, it_aux};
            Itrange true_expression    = {it_aux + 1, it_aux2};
            Itrange false_expression   = {it_aux2 + 1, it_new / -1};
            print("done");
            solve_expression(helper, boolean_expression);
            solve_expression(helper, true_expression);
            solve_expression(helper, false_expression);
        }
        else if (Semit it_new = binary_expression_match(Lexems::b_operators,range, it_aux))
        {
            Itrange left_member  ( it_new/0, it_aux-1 );
            Itrange right_member ( it_aux,   it_new/-1 );
            
            solve_expression(helper, left_member);
            solve_expression(helper, right_member);
        }
        else if (Semit it_new = unary_expression_match(range, it_aux))
        {
            Itrange member  ( it_aux, it_new /-1 );
            solve_expression(helper, member );
        }

        else if (Semit it_new = Namespace::try_syntax<Namespace,Model,CBlock>(gtr, range)) { if (debug) print("namespace"); }
        else if (Semit it_new = Class::try_syntax<Class,Model,TBlock,CBlock,LBlock>(gtr, range)) { helper.add_class(reinterpret_cast<Class*>(it_new->model())->name.token); }
        else if (Semit it_new = Struct::try_syntax<Struct,Model,TBlock,CBlock,LBlock>(gtr, range)) { 
            Struct * st = reinterpret_cast<Struct*>(it_new->model());
            if (!st->name.empty()) helper.add_struct(st->name[0]->token);
            }
        else if (Semit it_new = ColonExpansion::try_syntax<ColonExpansion,Model,PBlock,CBlock>(gtr, range)) {  }
        else if (Semit it_new = Function::try_syntax<Function,Model,PBlock,CBlock,TBlock>(gtr, range)) {  }
        else if (Semit it_new = VarDeclaration::try_syntax<VarDeclaration,Model>(gtr, range)) {  }
        else if (Semit it_new = NamedCall::try_syntax<NamedCall,Model,PBlock,TBlock>(gtr, range)) {  }
        else if (Semit it_new = TypedCall::try_syntax<TypedCall,Model,PBlock>(gtr, range)) {  }
        else if (Semit it_new = Index::try_syntax<Index,Model,SBlock>(gtr, range)) {  }
        else if (Semit it_new = Constructor::try_syntax<Constructor,Model,PBlock,LBlock,CBlock>(gtr, range)) {  }
        else if (Semit it_new = ForwardDeclaration::try_syntax<ForwardDeclaration,Model>(gtr, range)) { }
        else if (Semit it_new = Typename::try_syntax<Typename,Model,TBlock>(gtr, range)) { helper.add_class(reinterpret_cast<Typename*>(it_new->model())->name.token); }
        else if (Semit it_new = EnumClass::try_syntax<EnumClass,Model,CBlock>(gtr, range)) { 
            EnumClass * ec = reinterpret_cast<EnumClass*>(it_new->model());
            helper.add_class(ec->name.token);
        }
        else{
           //print("->unresolved");
           solved = false;

           str representation = mkstr(s.size()," : [",join(s,'|'),"]");
           range.begs.addBeforeBrother(semantics::Model());
           (range.begs-1)->model.set<Unresolved>(representation);
           Semit::erase(range);
           // it_toset().model.set<Unresolved>(representation); // throw Exception(unknown expression: expression)
           helper.ko(representation);
        }
        if (solved) helper.ok();
        //if (solved) print("->solved");
      //  print(mkstr("->absc: ",absorbed_children));
        return absorbed_children;
    }

    /*namespace jch::est::unils
    {
        template<class Class,template <typename...> (Class::*ismemfun),typename...IsArgs>
        std::function<bool
    }
    template<class SemanticClass>
    std::function<bool(const Model& m)> is()
    {
        return [](const Model& m)->bool{ return m.is_cross<SemanticClass>(); };
    }*/

    void solve_control_structure(ParserHelper& helper, Itrange& range)
    {
        Semit end = range.ends + 1; // pass-the-element iterator;
        semantics::Tree control_structure;
        str::list s;
        for (const auto& m : range)
            s.push_back(m.model()->to_str());
        //print(mkstr("attempting to solve control ",join(s,'|')));

        auto is_control = [](const Model& m,const str& s)->bool{ if (syntax::Control* co = m.cast<syntax::Control>()) return co->token == s; };

        auto _if     = [is_control](const Model& m) { return is_control(m,"if"); };
        auto _else   = [is_control](const Model& m) { return is_control(m,"else"); };
        auto _cblock = [](const Model& m) { return m.cast<syntax::CBlock>(); };
        auto _sep    = [](const Model& m) { if (syntax::Separator* sep = m.cast<syntax::Separator>()) return sep->token == ";"; return false; };

        auto _unknown = [_if,_else,_cblock,_sep](const Model& m){ return !_if(m) && !_else(m) && !_cblock(m) && !_sep(m); };

        auto open    = [_if,_else](const Semit& it,const Semit& ito)->bool { return _if(*it) || _else(*it); };//t == "if" || *it == "else" || *it == "elif"; };
        auto dclose  = [_else,_cblock,_sep](const Semit& it,const Semit& ito)->bool {
            auto itp = ito;
            itp.gofirst();
            return (_else(*itp) && ( _cblock(*it) || _sep(*it)));
        };
        auto close   = [_cblock,_sep](const Semit& it,const Semit& ito)->bool { return _cblock(*it) || _sep(*it); };
        auto brother = [_unknown](const Semit& it,const Semit& ito)->bool { return _unknown(*it); };

        auto onbrother = [](Semit& it_new,const Semit& it_old)
        {
            if (!it_old.has_children()) return;
            //print(mkstr("absorbing... ",it_old->model()->to_str()," ",it_old.node->children.size()));
            it_new.absorb(it_old.children_range());
        };

        control_structure.deserialize<Semit>(range.begs,end,open,dclose,close,brother,onbrother);
        range.begs.addBeforeBrother(Model());
        (range.begs-1).substitute(control_structure.root());
        (range.begs-1).children_up();
        Semit::erase(range);
    }


    void solve_expression_set(ParserHelper& helper, Semit it)
    {
        // assuming a bunch of syntax children, which we will solve.

        if (!it.has_children()) return; // single syntax elements don't have semantic meaning
        // TOOD: revise above comment. no reason why single syntax elements wouldn't have sementic meaning

        //print("spliting...");
        //print(mkstr("children range ",bool(it.children_range().begs),bool(it.children_range().ends)));
        //print(it.children_range().begs->model()->to_str());
        //print(it.children_range().ends->model()->to_str());
        //print(bool(it.children_range().ends+1));

        auto split_here = [](semantics::Tree::iterator it)->bool{
            auto itn = it;

            bool next_is_else = false;
            if (itn.next()){

                syntax::Control* controln = itn->cast<syntax::Control>();
                next_is_else = controln && controln->token == "else";
            }
            return !next_is_else && (it->cast<syntax::Separator>() || it->cast<syntax::CBlock>() );
        };
        //vector<Itrange> units_split_by_separators = it.children_range().split_groups( &Model::is_expression_separator );
        vector<Itrange> units_split_by_separators = it.children_range().split_groups( split_here );
        //print(mkstr("splitted ",units_split_by_separators.size()));
        for(const Itrange& range : units_split_by_separators)
        {
            Itrange expression_range { range.begs, range.ends->model.is_cross<syntax::Separator>()? range.ends-1 : range.ends };
            if (range.begs->cast<syntax::Control>())
            {
                //print("control structure detected...");
                solve_control_structure(helper, expression_range);
            }
            else solve_expression(helper, expression_range);
        }
        it.erase_children_if( &Model::is_separator );
    }
        
        /*size_t ic(0);
        do
        {
            // TODO: revise. need to come up with something more consistent
            const bool is_sep  = is_separator(itc().model());
            const bool is_pure = is_pure_separator(itc().model());
            const bool is_app  = is_appendable(itc().model());
            if (is_sep)
            {
                if (is_app)
                    separator_blocks.back().second = itc;
                else
                    separator_blocks.back().second = itc-1;

                if (!is_app && !is_pure)
                    separator_blocks.push_back({itc,itc});
                separator_blocks.push_back({itc+1,iterator()});
            }
            ++ic;
        }
        while( itc.next() );

        separator_blocks.back().second = itc-1;
        
        itc = it;
        //print(itc.down());
        auto itf = itc;
        itf.down();
        size_t added_children(0);
        size_t acc(0);
        for(const auto& block : separator_blocks)
        {
            if (block.second < block.first) continue; // TODO: at least warning here.
            solve_expression(helper, block.first, block.second);
            // TODO: erase separators
            //print(mkstr("block ",block.first," ",block.second));
            /*if (block.second < block.first) continue; // TODO: at least warning here.
            if (itc == it){
                itc.addFirstChild(semantics::Model());
                itc.down();
            }
            else
            {
                itc.addBrother(semantics::Model());
                itc.next();
            }
            
            ++added_children;

           // print(mkstr("isep/",acc," ",block.first," ",block.second));
            //print("A");
            acc += solve_expression(helper,itc,itf + (block.first-acc) ,itf + (block.second-acc) );

            //print("B");
            //solve_expression(itc,itf + block.first ,itf + block.second);
            str res;*/
            
            //for(int i=block.first;i<=block.second;++i){
            //    auto itg = itf + i ;
               // print(mkstr("addchild ",));
            //    itc.absorb_children(itg);
           // }

       // }
        
        //return added_children;
    //}
}

namespace syntics::cpp
{
    
    /*void fix_elses(syntax::Tree& tree)
    {
        auto _if = [](const syntax::Tree::iterator& it)->bool { if (syntax::Control* c = it->cast<syntax::Control>()) return c->token == "if"; return false; };
        auto _block = [](const syntax::Tree::iterator& it)->bool { return it->cast<syntax::CBlock>(); };
        auto _else = [](const syntax::Tree::iterator& it)->bool { if (syntax::Control* c = it->cast<syntax::Control>()) return c->token == "else"; return false; };
       // auto _else = [_else,_if](const syntax::Tree::iterator& it)->bool { if (Control* c = it->cast<Control>()) return c->token == "else"; return false; };

        auto move_else = [_if,_else,_block](syntax::Tree::iterator it)->void
        {
            if (!_else(it)) return ;
            auto p_if = it;
            p_if.prev();
            if (!_if(p_if)) throw est::Exception(mkstr("Expected if here: ",p_if->model()->to_str()));
            if ( !_block(p_if + 2) && _if(p_if / 0))
                p_if.absorbChild(it);
        };

        tree.iterate_it(move_else);
    }*/

    vector<syntax::Annotation*> collect_annotations(semantics::Tree& tree,semantics::Tree::iterator it)
    {
        auto ito = it;
        if (!it.down()) return {};

        vector<syntax::Annotation*> to_return;
        vector<semantics::Tree::iterator> to_remove;
        do
        {
            if (it->model.is_cross<syntax::Annotation>()){
                to_return.push_back(reinterpret_cast<syntax::Annotation*>(it->model()));
                to_remove.push_back(it);
            }
        } while (it.next());
        
        for(auto itr : to_remove)
            tree.erase(itr);
        
      //  print(mkstr("Annotations for ",ito->model()->to_str(),": ",to_remove.size()));
        return to_return;
    }

    bool is_separator(const Base* b){
        return dynamic_cast<const syntax::Separator*>(b) || dynamic_cast<const syntax::CBlock*>(b);
    }

    bool is_pure_separator(const Base* b){
        return dynamic_cast<const syntax::Separator*>(b);
    }
    bool is_appendable(const Base* b){
        return dynamic_cast<const syntax::CBlock*>(b);
    }

    str flatten(const syntax::Tree& tree)
    { return tree.toStr2([](const semantics::Model& t){ return t.model()->to_str(); }); }

    /*syntax::Tree syntax_flatten(const semantics::Tree& semantic_tree)
    {

        semantics::Tree semantics_tree = syntax_tree.copy_structure<syntax::Model>();

        semantics_tree.reverse_iterate

        return syntax_tree;
    }*/

    semantics::Tree parse(const str& text,ParserHelper* helper)
    {
        const bool externalParser = helper != nullptr;
        if (!helper) helper = new ParserHelper();

        syntax::Tree syntax_tree;
        syntax::Parser parser(*helper);
        
        syntax_tree.deserialize<syntax::Parser>(text,parser);
       // fix_elses(syntax_tree);
        parser.with_syntax_context(syntax_tree);
        //print("tree parsed");
        //print(flatten(syntax_tree));
        //print("///////////////////////////////");
        semantics::Tree semantics_tree = syntax_tree.copy_structure<semantics::Model>();
        //print("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
        //auto root = semantics_tree.root();
        semantics_tree.reverse_iterate([&semantics_tree,helper](semantics::Tree::iterator it){
            auto annotations = collect_annotations(semantics_tree,it);
            
            syntics::semantics::solve_expression_set(*helper, it);
            //print(mkstr(it->model()->to_str(),": elements created ",new_elements));
           /* if (new_elements)
            {
                Semit itc = it;
                int to_erase = itc.node->children.size() - new_elements;
                itc.down();
                for(int i=0;i<to_erase;++i){
                // print("erasing");
                    semantics_tree.erase(itc + new_elements);
                }
            }*/
        });

        //print("done converting");
        if (!externalParser) delete helper;
        return semantics_tree;
    }

    str flatten(const semantics::Tree& tree)
    { return tree.toStr2([](const semantics::Model& t){ return t.model()->to_str(); }); }
    
}