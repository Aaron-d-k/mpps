#include  "optimal5.h"
#include "processrule.h"

uint32_t gen_optimal5_inp(otca_rule rule)
{
    uint32_t o=0;
    for (size_t i = 0; i < 32; i++)
    {
        o <<= 1;
        if ((i>>1)<=8)
        {
            o+=(rule>>((i>>1)+(i&1)*9))&1;
        }
    }
    return o;
}


string codetemplate = R"(

expanded_row parallel_evolve(expanded_row s, expanded_row v)
{
    expanded_row x5 = v, x4 = s&CELL_POSN, x3 = (s>>1)&CELL_POSN, x2 = (s>>2)&CELL_POSN, x1 = (s>>3)&CELL_POSN;
    
    //CODE AUTOMATICALLY GENERATED FOR <rule>

    <autogenerated>
}
)";


int main(int argc, char *argv[])
{
    if (argc<2) 
    {
        cerr << "Use rulename as command line argument to generate code.\n";
        return EXIT_FAILURE;
    }

    opt5::Optimiser opt("knuthies.dat");

    auto gv = opt.lookup(gen_optimal5_inp(parse_rule(argv[1])));

    string s = opt5::print_gatevec(gv);

    s = regex_replace(s, regex(";"), ";\n    expanded_row");
    s = regex_replace(s, regex("expanded_row result ="), "return");
    s = s.substr(0,s.size()-13);
    s = "expanded_row "+s;

    s = regex_replace(codetemplate, regex("<autogenerated>"), s);
    s = regex_replace(s, regex("<rule>"), argv[1]);

    cout << s << endl;
}