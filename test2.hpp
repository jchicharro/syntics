
template<typename T>
class Class1 : Parent {
    T caracola;
    int number = 5;
    int operator++() const {}

    Class1(const Class1<T>* bro)
    : number(6)
    , name("Joe")
    , brother(bro)
    {}

    void operator()() const {}
    void operator--() const {}
    operator bool() const {}

    void intit(){
        for(int i : intvector){}
    }
    const Class1* brother;
    operator str() const {}
}