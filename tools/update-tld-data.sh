#!/bin/sh

wget 'https://data.iana.org/TLD/tlds-alpha-by-domain.txt' -O tlds.txt
sed '1d' tlds.txt | sed -e 's/\(.*\)/\L\1/' > tlds.txt
sed -n -e '/^xn--/p' tlds.txt | sed 's/^\(xn--\)*//'  | idn -d >> tlds.txt
mv tlds.txt ../resources/tlds.txt
