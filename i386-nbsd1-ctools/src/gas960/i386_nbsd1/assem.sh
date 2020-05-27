
if [ "$2" = "" ]; then
	GAS=gas960c
else
	GAS=gas960
fi

echo I am using: $G960BASE/bin/$GAS

for i in *.s ; do
	j=`basename $i .s`.o
	$G960BASE/bin/$GAS -z $i -o $1/$j
	echo $i
done
