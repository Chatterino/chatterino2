#!/bin/sh

wget 'https://data.iana.org/TLD/tlds-alpha-by-domain.txt' -O tmp1.txt
sed '1d' tmp1.txt > tmp2.txt
sed -e 's/\(.*\)/\L\1/' tmp2.txt > tmp3.txt
sed -n -e '/^xn--/p' tmp3.txt > tmp4.txt
sed 's/^\(xn--\)*//' <tmp4.txt | idn -d > tmp5.txt
cat tmp3.txt tmp5.txt > tmp6.txt
sed -E ':a;N;$!ba;s/\r{0,1}\n/|/g' tmp6.txt > ../resources/tlds.txt
rm tmp*.txt
