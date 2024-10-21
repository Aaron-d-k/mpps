
string pevolve_rule_name = "B2/S2";

expanded_row parallel_evolve(expanded_row s, expanded_row v)
{
    expanded_row x5 = v, x4 = s&CELL_POSN, x3 = (s>>1)&CELL_POSN, x2 = (s>>2)&CELL_POSN, x1 = (s>>3)&CELL_POSN;
    
    //CODE AUTOMATICALLY GENERATED FOR B2/S2

    expanded_row x6 = x1 | x4;
    expanded_row x7 = x3 &~ x6;
    expanded_row x8 = x7 &~ x2;
    return x8;
   
}

