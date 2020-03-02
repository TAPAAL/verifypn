Building lpsolve
================

Linux 64 bit & OSX 64 bit
-------------------------

 1. Go to `lp_solve_5.5/lpsolve55/`
 2. Modify `opts` in the `ccc` file to:
    `opts='-O3 -DLoadInverseLib=0 -DLoadLanguageLib=0 -DLoadableBlasLib=0 -Wno-narrowing'`
 3. Modify `c=cc` to `c="gcc -m64 -std=c11"`
 4. Run `sh ccc`
 5. Steal `lp_solve_5.5/lpsolve55/bin/ux64/liblpsolve55.a`

Windows 64 bit
--------------
A binary should be avalible online via https://sourceforge.net/projects/lpsolve/
