MPPS
----
This is a search program named MultiPeriodPhotonSearcher v0.2, which can be used to find photons with arbitrarily large periods. It can be set up to search for a specific period (using the feature of forbidden periods) or it can be set up to search all periods less than max period.

You will have to set some variables like max period 
& width in the source code itself. Some parameters like forbidden periods and the rulespace can 
be specified as command line arguments.

You can compile this program on linux it using the command 

    >>> g++ mpps.cpp -O3 -march=native -Wall -Wextra -std=c++20 -o mpps

and run it using

    >>> ./mpps B2/S0 f 4 6

This will do a search in live free or die with forbidden periods 4 and 6.
At width 14 even, this should quickly find p2, p3, p9 and p5 photons.

A lot of the parameters need to be set directly in the part of the code where it says 'SET PARAMETERS HERE' in file [common.h](common.h).

You can also use the program to bulk search rules by feeding them in through stdin after enabling
MULTI_RULE in common.h

You can enable 'parallel evolution' which is much faster as it uses bit tricks to speed up evolution. But the code for each rule must be generated using the program [genrulecode.cpp](genrulecode.cpp) and pasted into the file [processrule.h](processrule.h). Running `genrulecode.cpp` also requires you to install the [optimal5](https://gitlab.com/apgoucher/optimal5) program and set the appropriate include path and path to knuthies.dat in the file.

The core idea used here is similar to that of [photonsrc](https://conwaylife.com/forums/viewtopic.php?f=9&t=4649) by LaundryPizza03, but I came up with most of the ideas used in this program independently.


CHANGE LOG
---------
v0.1
----
- Initial version 

v0.2
----
- Added feature to help reduce memory taken by program to avoid running out of ram.
   Can be enabled by setting MAX_TREE_SIZE.

- Added basic checking to see which cells cannot be on and use this to cleverly reduce the number of extensions checked. Makes this almost 20x faster for some searches

- Added option to enable 'parallel evolution' which is much faster. But the code for each rule must be generated and pasted into the file. Running it requires you to install the [optimal5 library](https://gitlab.com/apgoucher/optimal5) and setting the appropriate include path in the file

- Made command line arguments slightly more convenient to use



