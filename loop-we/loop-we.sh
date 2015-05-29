#!/bin/sh

n=`printf %02d $(($PBS_VNODENUM + 1))`

# XXX: hardcoded

data=/storage/brno3-cerit/home/ljocha/foxs
foxs=/storage/brno3-cerit/home/ljocha/soft/foxs/foxs

mkdir $SCRATCHDIR/$n
cd $SCRATCHDIR/$n

feval()
{
	echo "$*" | bc -l
}

ftest()
{
	r=`echo "$*" | bc -l`
	test $r = 1
}

date

# XXX: specificka jmena souboru
cp $data/mod$n.pdb .
cp $data/pokus1.dat .

e=0.95

while ftest "$e <= 1.05"; do
	w=-2
	while ftest "$w <= 4"; do
		$foxs -w $w -e $e mod$n.pdb pokus1.dat
		nn=`printf '%02d_%.2f_%.3f.dat' $n $w $e`
#		mv mod${n}_pokus1.dat ${n}_${w}_${e}.dat
		mv mod${n}_pokus1.dat $nn
		w=`feval $w + 0.05`
	done
	e=`feval $e + 0.005`
done

date

mkdir -p $data/$n
cp ${n}*.dat $data/$n
