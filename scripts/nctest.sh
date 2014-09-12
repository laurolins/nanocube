#!/bin/bash

/bin/rm -f out.txt
touch out.txt

wget -q -O - 'http://localhost:29512/query/Date=484:1971:1/Primary_Type=<2>/src=<qaddr(135337,329929,19),qaddr(133938,329929,19),qaddr(133938,328791,19),qaddr(135337,328791,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Primary_Type=<2>/src=<qaddr(135337,329929,19),qaddr(133938,329929,19),qaddr(133938,328791,19),qaddr(135337,328791,19)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Primary_Type=<2>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=484:1971:1/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Primary_Type=255+1' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=484:1971:1/Primary_Type=<2>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=484:1971:1/Primary_Type=<2,29>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Primary_Type=<2,29>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/Primary_Type=<2,29>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Primary_Type=255+1' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/Primary_Type=<29>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Primary_Type=<29>/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/src=<qaddr(135394,329870,19),qaddr(133995,329870,19),qaddr(133995,328732,19),qaddr(135394,328732,19)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(269953,659212,20),qaddr(268554,659212,20),qaddr(268554,658073,20),qaddr(269953,658073,20)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(269953,659212,20),qaddr(268554,659212,20),qaddr(268554,658073,20),qaddr(269953,658073,20)>/@Primary_Type=255+1' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/src=<qaddr(269953,659212,20),qaddr(268554,659212,20),qaddr(268554,658073,20),qaddr(269953,658073,20)>/@Date=484:7:282' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(269900,659369,20),qaddr(268501,659369,20),qaddr(268501,658231,20),qaddr(269900,658231,20)>' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/Date=1174:727:1/src=<qaddr(269900,659369,20),qaddr(268501,659369,20),qaddr(268501,658231,20),qaddr(269900,658231,20)>/@Primary_Type=255+1' >> out.txt
echo >> out.txt

wget -q -O - 'http://localhost:29512/query/src=<qaddr(269900,659369,20),qaddr(268501,659369,20),qaddr(268501,658231,20),qaddr(269900,658231,20)>/@Date=484:7:282' >> out.txt
echo >> out.txt


diff out.txt nctest_output_expected.txt

tmp=$?
if [ $tmp -ne 0 ]; then
echo "***********"
echo "FAILURE: Test output does not match expected results.  Something is wrong with the setup."
echo "***********"
else
echo "SUCCESS"
/bin/rm -f out.txt
fi

