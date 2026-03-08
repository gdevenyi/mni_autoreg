#! /bin/sh
set -e

corr_before=`xcorr_vol object1.mnc object2.mnc | tr -d ' ' | cut -c 1-6`
echo $0 xcorr before: $corr_before

if [ $corr_before != 0.7231 ];then
  echo $0 Corr test before failed
  exit 1
fi

minctracc -iterations 10 \
    -identity object1_dxyz.mnc object2_dxyz.mnc \
    -est_center -debug  -step 10 10 10 -nonlin \
    -clobber def.xfm

mincresample object1.mnc -like object2.mnc -transform def.xfm  object1_res.mnc -clob

corr_after=`xcorr_vol object1_res.mnc object2.mnc | tr -d ' ' | cut -c 1-6`
echo $0 xcorr after: $corr_after
tresult=$(echo "$corr_after>=0.9860 && $corr_after<1.0" | bc)
if [ $tresult != 1 ];then
  echo $0 Corr test after failed
  echo corr_after=\"${corr_after}\"
  exit 1
fi
