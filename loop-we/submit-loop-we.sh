#!/bin/sh


# XXX: hardcoded
data=/storage/brno3-cerit/home/ljocha/foxs
cp loop-we.sh $data

pbsdsh $data/loop-we.sh
