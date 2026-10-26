pushd Plots
for cat in ggHcat6; do
  python3 makeMultipdfPlot.py \
    --inputWSFile ../Background/outdir_tutorial/CMS-HGG_multipdf_${cat}.root \
    --cat ${cat} \
    --ext tutorial \
    --mass 125.38 \
    --inputSignalWSFile ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged_${cat}.root
  
done
popd