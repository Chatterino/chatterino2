#!/bin/sh

#Download the official list of active TLDs from IANA
#Remove the first line that contains data not needed.
#Put everything that can be into lowercase.
#Output the result to a file.
curl -s 'https://data.iana.org/TLD/tlds-alpha-by-domain.txt' | sed -e '1d' -e 's/\(.*\)/\L\1/' > tlds.txt

#Get the TLDs in punycode format.
#Convert the punycode to Unicode.
#Append the results to the current file.
sed -n -e '/^xn--/p' tlds.txt | idn2 -d >> tlds.txt
mv tlds.txt ../resources/tlds.txt