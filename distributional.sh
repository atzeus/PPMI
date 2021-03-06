
if [ "$#" -lt 5 ]; then
    echo "Usage: distributional.sh <corpus> <vocab> <pairs> <ppmi-rep-base> <svd-rep-base> (extra args)"
    echo ""
    echo "Corpus must already exists, the rest of the files are generated by this script."
    echo 
    echo "This will generate: "
    echo ""
    echo "<vocab>"
    echo "<pairs>"
    echo 
    echo "<ppmi-rep-base>.contexts.vocab"
    echo "<ppmi-rep-base>.words.vocab"
    echo "<ppmi-rep-base>.npz"
    echo
    echo "<svd-rep-base>.contexts.vocab"
    echo "<svd-rep-base>.words.vocab"
    echo "<svd-rep-base>.vt.npy"
    echo "<svd-rep-base>.ut.npy"
    echo "<svd-rep-base>.s.npy"
    echo
    echo "These can be used by hyperword"
    echo
    echo
    echo "Extra arguments are similar to hyperword:"
    echo
    echo "--thr (default: 100)"
    echo "--rank (default: 500)"
    echo "--win (default: 2)"
    echo "--sub (default: 0)"
    echo "--cds (default: 0.75f)"
    echo "--pos (default: false)"
    echo "--dyn (default: false)"
    echo "--del (default: false)"
    echo "--neg (default: 0)"
    exit 0
fi

corpus=$1
vocab=$2
pairs=$3
ppmi=$4
svd=$5
echo $corpus
echo $@
shift 5
./build/default/test/count $corpus $vocab $pairs $ppmi.mm $svd $@

if [ $? -eq 0 ]; then
echo "Done with calculation, massaging files for hyperword"
else
echo "Something went wrong"
exit 1
fi

python convertTXT.py $svd.ut.txt $svd.ut.npy
rm $svd.ut.txt
python convertTXT.py $svd.vt.txt $svd.vt.npy
rm $svd.vt.txt
python convertTXT.py $svd.s.txt $svd.s.npy
rm $svd.s.txt
python convertMM.py $ppmi.mm $ppmi.npz
#rm $ppmi.mm

cp $vocab $svd.contexts.vocab
cp $vocab $svd.words.vocab
cp $vocab $ppmi.contexts.vocab
cp $vocab $ppmi.words.vocab


