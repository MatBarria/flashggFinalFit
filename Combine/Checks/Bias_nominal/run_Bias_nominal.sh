REMOVE_PREVIOUS=false

while getopts "r" opt; do
    case $opt in
        r) REMOVE_PREVIOUS=true ;;
    esac
done

if $REMOVE_PREVIOUS; then
    #echo "Removing previous results..."
    echo "Removing previous datacards"
    rm -rf D*.root

    echo "Removing previous toys"
    rm -rf BiasToys/*.root

    echo "Removing previous fits"
    rm -rf BiasFits/*.root

    echo "Removing previous plots"
    rm -rf BiasPlots/*.p*
    exit 1
fi

CSVFILE="bias_results_$(date +%Y%m%d_%H%M%S).csv"
for cat in VBFcat1 VBFcat2 VBFcat3 VBFcat4 VBFcat5 ggHcat1 ggHcat2 ggHcat3 ggHcat4 ggHcat5; do
  cp ../../Datacard_tutorial_${cat}_mu_inclusive.root .
  python3 RunBiasStudy.py \
  -d Datacard_tutorial_${cat}_mu_inclusive.root -t \
  --generateCSV \
  -b ${cat} \
  -f -c "--setParameterRanges r=-5,5 --cminDefaultMinimizerStrategy 0 --X-rtd MINIMIZER_freezeDisassociatedParams --X-rtd MINIMIZER_multiMin_hideConstants --X-rtd MINIMIZER_multiMin_maskConstraints" \
  -p --gaussianFit \
  -n 1000 \
  --outCSV ${CSVFILE}

done   # -f -c "--setParameterRanges r=-50,50 --cminDefaultMinimizerStrategy 1 --X-rtd MINIMIZER_freezeDisassociatedParams --X-rtd MINIMIZER_multiMin_hideConstants --X-rtd MINIMIZER_multiMin_maskConstraints --verbose 2" \
