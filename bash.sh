input=$1
file="/Users/mateo2/Documents/Code/output2/fasta/$input.txt";
file2="/Users/mateo2/Documents/Code/output2/structureEmpty/$input.txt";
exec 5<$file
exec 6<$file2
while read line1 <&5 && read line2 <&6; do

if ((i % 2 == 1))  

then
echo $line1 > out1.txt
echo $line2 >> out1.txt
# gtime -o out.txt -f "%e\t%M" ./build/Spark -P "/Users/mateo2/Documents/Code/Spark/src/params/parameters_DP09_Vienna.txt" -d1 -r $line2  $line1;
# gtime -o out.txt -f "%e\t%M" /Users/mateo2/Documents/Code/HFold/HFold -s $line1 -r $line2

# gtime -o out2.txt -f "%e\t%M" /Users/mateo2/Documents/Code/includes/Vienna/bin/RNAfold -d1 --constraint<out1.txt --enforceConstraint out1.txt
# ./build/Spark -P "/Users/mateo2/Documents/Code/Spark/src/params/parameters_DP09_Vienna.txt" -d1 -r $line2  $line1 > "/Users/mateo2/Documents/Code/Spark/out.txt";
# echo $line > out2.txt;
# gtime -o out.txt -f "%e\t%M" ./build/CParty -d2 -r $line2 $line1
./build/CParty -d2 -r $line2 $line1 > "/Users/mateo2/Documents/Code/CParty/out.txt"
# /Users/mateo2/Documents/Code/ViennaRNA-2.6.4/src/bin/RNAfold --noPS --noDP -d2 -p out1.txt > out.txt
# /Users/mateo2/Documents/Code/HFold/HFold -s $line1 -r $line2 > "/Users/mateo2/Documents/Code/Spark/out.txt";
# ../a.out
# cat "/Users/mateo2/Documents/Code/Spark/out.txt" >> "/Users/mateo2/Documents/Code/output/proof/Spark/$input.txt"
# cat "/Users/mateo2/Documents/Code/Spark/out.txt" >> "/Users/mateo2/Documents/Code/output/proof/HFold/$input.txt"
# cat "/Users/mateo2/Documents/Code/CParty/out2.txt" >> "/Users/mateo2/Documents/Code/output2/energies/RNAFold/$input.txt"

cat "/Users/mateo2/Documents/Code/CParty/out.txt" >> "/Users/mateo2/Documents/Code/output2/energies/CParty/$input.txt"

# cat "/Users/mateo2/Documents/Code/Spark/out2.txt" >> "/Users/mateo2/Documents/Code/output/time/RNAFold/$input.txt"

# echo /usr/bin/time -o out.txt -f "%e\t%M" ./build/Spark -P "/home/mgray7/Spark/src/params/parameters_DP09_Vienna.txt" -p -d1 -r \"$line2\"  $line1;
# echo $line2
fi
i=$((i+1));
done
echo "first is $i";
#   echo "${line}";
exec 5</dev/null
exec 6</dev/null
