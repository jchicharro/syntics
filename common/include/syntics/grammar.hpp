#ifndef __SYNTICS_GRAMMAR_HPP__
#define __SYNTICS_GRAMMAR_HPP__

#include <est/variadic>

namespace bparse
{

    template<class BaseClass,typename Getter, typename...TokenSpecs>
    struct GrammarUnit : public BaseClass, 
                         public est::variadic::grammar::Struct<TokenSpecs...>
    {
        using Grammar = est::variadic::grammar::Struct<TokenSpecs...>;
        
        GrammarUnit(typename TokenSpecs::tuple_type... ts) : Grammar(ts...) {}

        template<int I>
        auto& reftoken()
        {
            return std::get<I>(est::variadic::grammar::Struct<TokenSpecs...>::tokens);
        }

        /*template<int I,typename Iterator,typename TokenSpecs::type...>
        struct TokenToSyntaxIterator
        {
            void operator()(Iterator& it,typename Grammar::Tuple& tuple)
            {
                it.addChild(std::get<I>(tuple));
            }
        };

        void syntax_flatten(SyntaxIterator it)
        {
            est::variadic::looper<tuple_size,TokenToSyntaxIterator>(it);
        }*/

        template<class DesiredClass,typename T,typename...Absorbable>
        //template<typename T,typename...Absorbable>
        static typename jch::est::tree<T>::iterator try_syntax(Getter getter, typename jch::est::tree<T>::iterator::range r, bool force_size_match = true)
        {
            // range here is [beg,end]
            typename jch::est::tree<T>::iterator it_new; // not initialized, will evaluate to false if coerced
            typename Grammar::Tuple constructor_tuple;
            auto rend = r.ends + 1;
            if (!Grammar::match(getter, r.begs,rend,constructor_tuple, force_size_match)){
                //print("sem/no match");
                return it_new; // i.e. false-like
            }
            //print("sem/match");
            // if grammar matches, create new child
         //   auto newelem = std::make_from_tuple<GrammarUnit<TokenSpecs...>> ( constructor_tuple ); // TODO: improve, unnecessary copy. new + make_from_tuple?
            r.begs.addBeforeBrother( T() );
            it_new = r.begs;
            it_new.prev();
            //it_new = r.begs-1;
            //it_new->model.ptr = new GrammarUnit<BaseClass, Getter, TokenSpecs...> (std::make_from_tuple<GrammarUnit<BaseClass, Getter, TokenSpecs...>> ( constructor_tuple ));
            it_new->model.ptr = new DesiredClass (std::make_from_tuple<DesiredClass> ( constructor_tuple ));
            //print("lastit");
            //print(r.ends->model()->to_str());
            //print("----created");
            //print(it_new->model()->to_str());
            while (r.begs != rend) // +1 here bc loops are [) ranges
            {
                //print("checkitdel");
                //print(bool(r.begs));
                //print(r.begs->model());
                //print(r.begs->model()->to_str());
                auto to_check = r.begs;
                ++r.begs;
                if (to_check->model.template is_any_of<Absorbable...>()){
                    //print("absorbing");
                    it_new.absorbChild(to_check);
                }
                else{
                    //print("deleting");
                    to_check.erase();
                }
            }
            return it_new; // i.e. true-like
        }
    };
}

#endif