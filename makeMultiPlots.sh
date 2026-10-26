pushd ./Plots
CSVFILE="out_yields_$(date +%Y%m%d_%H%M%S).csv"
for cat in ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5; do
  python3 makeMultipdfPlot.py \
    --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --nBins 160 \
    --mass 125.38 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done

for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 VBFcat6 VBFcat7; do
  python3 makeMultipdfPlot.py \
    --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --mass 125.38 \
    --nBins 80 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root \
    --outCSV ${CSVFILE}
  
done
popd