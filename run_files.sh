#!/bin/bash 
set -e

cp /media/sf_compilers_shared/PA4/cool-tree.h .
cp /media/sf_compilers_shared/PA4/emit.h .
cp /media/sf_compilers_shared/PA4/cgen.cc .
cp /media/sf_compilers_shared/PA4/cool-tree.handcode.h .
cp /media/sf_compilers_shared/PA4/cgen-context.h .
cp /media/sf_compilers_shared/PA4/cgen.h .

rm -f *.o *.d cgen mycoolc

touch cool-tree.h
touch emit.h
touch cgen.cc
touch cool-tree.handcode.h
touch cgen-context.h
touch cgen.h

make cgen

perl pa4-grading.pl