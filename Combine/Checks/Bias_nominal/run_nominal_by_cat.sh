#!/bin/bash
#set -euo pipefail
echo "running hello!"
voms-proxy-info --all
ls -l

echo "running hi!"
cat=$1
cmssw_version=$2

echo "Categroy: $cat"
echo "CMSSW version: $cmssw_version"


workDir=`pwd`
echo "Decompressing CMSSW"
source /cvmfs/cms.cern.ch/cmsset_default.sh
export SCRAM_ARCH=el9_amd64_gcc12
tar -xf $cmssw_version.tgz
#scramv1 project CMSSW $cmssw_version

mkdir -p BiasResults/
DATA_FOLDER="$cmssw_version/src/data/"


###########################################
# Copy input list and exec to cmssw folder
###########################################
echo "Copying inputs to CMSSW/src"
cp Datacard_${cat}.root $cmssw_version/src/.
cp RunBiasStudy.py $cmssw_version/src/.
cp biasUtils.py $cmssw_version/src/.

echo "Making output directories"
mkdir -p ${cmssw_version}/src/BiasToys/
mkdir -p ${cmssw_version}/src/BiasFits/
mkdir -p ${cmssw_version}/src/BiasPlots/

###########################
# Get cmssw environment
###########################
echo "Seting up CMSSW"
cd $cmssw_version/src/
scramv1 b ProjectRename
eval `scram runtime -sh`
which combine

CSVFILE="bias_results_${cat}_$(date +%Y%m%d_%H%M%S).csv"

echo "running bias study"
python3 RunBiasStudy.py \
  -d Datacard_${cat}.root -t \
  --generateCSV \
  -b ${cat} \
  -f -c "--setParameterRanges r=-5,5 --cminDefaultMinimizerStrategy 0 --X-rtd MINIMIZER_freezeDisassociatedParams --X-rtd MINIMIZER_multiMin_hideConstants --X-rtd MINIMIZER_multiMin_maskConstraints" \
  -p --gaussianFit \
  -n 2000 \
  --outCSV ${CSVFILE}

################################################################
# Copy output files 
################################################################
cp $CSVFILE ${workDir}/BiasResults/.
cp -r BiasFits/ ${workDir}/BiasResults/
cp -r BiasToys/ ${workDir}/BiasResults/
cp -r BiasPlots/ ${workDir}/BiasResults/