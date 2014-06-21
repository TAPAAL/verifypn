Building lpsolve
================

Linux 64 bit
------------

 1. Go to `lp_solve_5.5/lpsolve55/`
 2. Modify `opts` in the `ccc` file to:
    `opts='-O3 -DLoadInverseLib=0 -DLoadLanguageLib=0 -DLoadableBlasLib=0'`
 3. Run `sh ccc`
 4. Steal `lp_solve_5.5/lpsolve55/bin/ux64/liblpsolve55.a`

Linux 32 bit
------------

 1. Go to `lp_solve_5.5/lpsolve55/`
 2. Modify `opts` in the `ccc` file to:
    `opts='-O3 -DLoadInverseLib=0 -DLoadLanguageLib=0 -DLoadableBlasLib=0'`
 3. Modify `c=cc` to `c="gcc -m32"`
 4. Run `sh ccc`
 5. Steal `lp_solve_5.5/lpsolve55/bin/ux32/liblpsolve55.a`


