#! /bin/bash
for f in $(find . -name '*.c' -or -name '*.cpp'  -or -name  '*.h' -type f)
do
	astyle --style=allman --indent=spaces=4 --indent-preproc-block --pad-oper --pad-header --unpad-paren --suffix=none --align-pointer=name --lineend=linux --convert-tabs --verbose  --indent-switches $f
done

for f in $(find . -name  '*.orig' -type f)
do
	rm $f
done