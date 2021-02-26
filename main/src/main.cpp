#include <syntics/cpp.hpp>
//#include <est1>
#include <est/Exception.hpp>
#include <console/input.hpp>
#include <jch/fs>
#include <jch/json>

using namespace jch::json;

/*void translate_cpp_to_js(const str::list args)
{
    str::list hpps = args.select([](const str& s){ return s.ends(".hpp"); });
    str::list cpps = args.select([](const str& s){ return s.ends(".cpp"); });

    file::jch_list hpp_files = hpps.transform<file>([](const str& path)->file{ return file(path); });
    file::jch_list cpp_files = cpps.transform<file>([](const str& path)->file{ return file(path); });

    yaparse::cpp_to_js(hpp_files,cpp_files);

}*/

using namespace syntics;

const str CONFIG_FILE = "syntics.cfg";

struct Config
{
    str::map props;
    str operator[](const str& key){
        if (props.find(key) == props.end()) return "";
        return props[key];
    }
    Config(){
        if (is_file(CONFIG_FILE))
        {
            for(const str& line : file(CONFIG_FILE).read().split('\n'))
            {
                str::list ks = line.split(' ');
                if (ks.size() < 2) continue;
                props[ks[0]] = join(ks,' ',1);
            }
        }
    }
};

const str types_file = "syntics_known_types";

void update_known_types(const str::list types){
    str::list known_types;

    if (is_file(types_file))
    {
        str json_file = file(types_file).read();

        auto* jsonf = jch::json::parse(json_file);
        jch::json::objectizer<vector<str>>()(known_types,*jsonf);
    }

    known_types = (known_types + types).unique();

    auto* jsonc = jch::json::jsonizer<vector<str>>()(known_types);
    file(types_file).write(jsonc->to_str());
}
str::list get_known_types()
{
    if (!is_file(types_file)) return {};

    str json_file = file(types_file).read();
    auto* jsonf = jch::json::parse(json_file);
    str::list known_types;
    jch::json::objectizer<vector<str>>()(known_types,*jsonf);
    return known_types;
}
void parse_cpp(const str::list args)
{

    str::list file_contents = args.transform<str>([](const str& path)->str{ return file(path).read(); });

    ParserHelper parserHelper;
    Config config;
    //parserHelper.user_classes = config["user_classes"].split(' ');
    parserHelper.user_classes = get_known_types();

    for(int i=0;i<args.size();++i){
        print(mkstr("file: ",args[i]));
        print("----------------------");
        print("----------------------");
        print(cpp::flatten(cpp::parse(file_contents[i],&parserHelper)));
        print("......................");
        print(file_contents[i]);
        print("\n");
    }

}

void remove_color(str& string)
{
    str color_start = "\033";
    str color_end = "m";

    int ics = string.find(color_start);
    while (ics != -1)
    {
        int ice = string.find(color_end,ics + color_start.size());
        if (ice == -1) break;
        string = string.substr(0,ics) + string.substr(ice+color_end.size());
        ics = string.find(color_start);
    }

}

struct CheckResult
{
    str::list unresolved_expressions;
    size_t total;
    size_t success;
    str file;
    CheckResult(const str& f,size_t s,size_t t,const str::list& ue) : file(f.replace("\\","\\\\")), success(s), total(t), unresolved_expressions(ue)
    {
        for(str& ue : unresolved_expressions){
            remove_color(ue);
            ue = ue.replace("\t","");
            ue = ue.replace("\"","\\\"");
        }
    }
    JCH_JSON_4(file,success,total,unresolved_expressions);
};

void check_cpp(const str::list iargs)
{
   // print(iargs.size());
   // print(iargs[0]);
    str::list args;
    str::list file_contents;
    if (iargs.empty())
        args = file("input_file").read().split('\n');
    else args = iargs;
    ParserHelper parserHelper;
    Config config;
    //parserHelper.user_classes = config["user_classes"].split(' ');
    parserHelper.user_classes = get_known_types();
    file_contents = args.transform<str>([](const str& path)->str{ return file(path).read(); });
  //  print(args.size());

    vector<CheckResult> results;

    size_t runs(1);
    for(int irun =0;irun<runs;++irun)
    {
        str::list all_failures;
        size_t all_total(0);
        size_t all_ok(0);
        for(int i=0;i<args.size();++i){
            //print(mkstr("file: ",args[i]));
            //print("----------------------");
            parserHelper.reset();
            cpp::flatten(cpp::parse(file_contents[i],&parserHelper));
            size_t ok = parserHelper.total_ok;
            size_t total = parserHelper.total;
            float sr = total == 0? 0 : (ok+0.0f)/total;
            results.push_back({args[i],ok,total,parserHelper.failures});
            if (total)
            {
                float f = (ok + 0.0f)/total;
                print(mkstr(jch::console::color::bf_black_green(f)," : (",ok,"/",total,")"));
                for(const str& s : parserHelper.failures)
                    print(mkstr("  ",s));
                all_failures += parserHelper.failures;
                all_total += parserHelper.total;
                all_ok += parserHelper.total_ok;
            }
            else print(jch::console::color::bf_black_red("<no-semantics>"));
            print("......................");
        }
        print(mkstr("RUN ",irun,"============================================="));
        print(mkstr("known_types: ",join(parserHelper.types(),' ')));
        if (!all_total) print(jch::console::color::bf_black_red("<no-semantics>"));
        else{
            if (all_failures.empty()) print(jch::console::color::bf_black_green("100%: All OK!"));
            else
            {
                str message = mkstr(jch::console::color::bf_black_green(mkstr( int((all_ok+0.0f)/all_total*100),"%"))," ",jch::console::color::bf_black_red("There were failures (^^^)"));
                for(const str& s : all_failures)
                    print(s);
                print(message);
            }
        }
    }

    print(mkstr("known_types: ",join(parserHelper.types(),' ')));
    auto* jsonobj = jch::json::jsonizer<vector<CheckResult>>()(results);
    jch::est::file("itest.output").write(jsonobj->to_str());
    update_known_types(parserHelper.types());
}
/*void try_parse_cpp(const str::list args)
{
    using namespace jch::console;
    for(int i=0;i<args.size();++i){
        file::list files;
        if (is_dir(args[i])) files = file_list((std::string)args[i],"hpp|cpp",true);
        else files = {args[i]};
        for(const file& f : files)
        {
            print(mkstr("file: ",f.filename()));
            try{
                yaparse::cpp_to_agnostic(f.read());
                print(mkstr(" -> ",color::bf_black_green("OK")));
            }
            catch(const std::exception& e)
            {
                print(mkstr(" -> ",color::bf_black_red("Failed:")," ",e.what()));
            }
        }
    }

}*/

void parse_input(int narg,char ** argv,
                 vector<std::pair<str,std::function<void(const str::list&)>>> to_fun)
{
    str::list args;
    if (narg < 2)
        throw est::Exception("need command");
    str cmd = argv[1];
    for(int i=2;i<narg;++i)
        args.push_back(argv[i]);
        
    for(int i=0;i<to_fun.size();++i){
        if (to_fun[i].first == cmd){
            to_fun[i].second(args);
            return;
        }
    }
    throw est::Exception(mkstr("Unsupported command ",cmd));

}

    str blue(const str& s) { return str("\033[40;34m") + s + str("\033[0m"); }

void test_replace(const str::list& args)
{
    str str1 = str("pelos ") + blue(args[0]);
    print(mkstr("testing: ",str1));
    //remove_color(str1);
    print(mkstr("result: ",str1.replace("\033","\\033")));
}
int main(int narg,char ** argv)
{
 //   std::function<void(const str::list&)> tcj  = translate_cpp_to_js;
    std::function<void(const str::list&)> cppp = parse_cpp;
    std::function<void(const str::list&)> cppc = check_cpp;
    std::function<void(const str::list&)> tres = test_replace;
 //   std::function<void(const str::list&)> tcpp = try_parse_cpp;
    try
    {
        parse_input(narg,argv,
                    {
   //                    { "cpp_js" , tcj },
                       { "cpp_parse", cppp},
                       { "cpp_check", cppc},
                       { "test_replace", tres}
     //                  { "try_cpp_parse", tcpp}
                    }
                    );
    }
    catch(const std::exception& e)
    {
        print(mkstr("Failed to execute: ",e.what()));
        return -1;
    }

    return 0;
}