#!/bin/sh

curl -s 'https://data.iana.org/TLD/tlds-alpha-by-domain.txt' | sed -e '1d' -e 's/\(.*\)/\L\1/' > tlds.txt
sed -n -e '/^xn--/p' tlds.txt | idn2 -d >> tlds.txt
mv tlds.txt ../resources/tlds.txt
