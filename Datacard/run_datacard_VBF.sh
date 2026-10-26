echo "---------------------------- NOT USED IN MAIN PIPELINE -----------------------------------"
echo "---------------------------- RUNNING RUN DATACARD VBF -----------------------------------"

python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF --cats VBFcat1 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

cp Datacard_tutorial.txt Datacard_tutorial_VBFcat1.txt
cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat1.pkl
rm yields_tutorial/VBFcat1.pkl



python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF  --cats VBFcat2 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

cp Datacard_tutorial.txt Datacard_tutorial_VBFcat2.txt
cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat2.pkl
rm yields_tutorial/VBFcat2.pkl

python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBFF/ws_VBF  --cats VBFcat3 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

cp Datacard_tutorial.txt Datacard_tutorial_VBFcat3.txt
cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat3.pkl
rm yields_tutorial/VBFcat3.pkl

python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF  --cats VBFcat4 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

cp Datacard_tutorial.txt Datacard_tutorial_VBFcat4.txt
cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat4.pkl
rm yields_tutorial/VBFcat4.pkl

python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF  --cats VBFcat5 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

cp Datacard_tutorial.txt Datacard_tutorial_VBFcat5.txt
cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat5.pkl
rm yields_tutorial/VBFcat5.pkl

#python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF  --cats VBFcat6 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
#python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

#cp Datacard_tutorial.txt Datacard_tutorial_VBFcat6.txt
#cp Datacard_tutorial.pkl Datacard_tutorial_VBFcat6.pkl
#rm yields_tutorial/VBFcat6.pkl

python3 RunYields.py --inputWSDirMap combined=/uscms_data/d3/esledge/nobackup/CMSSW_14_0_14/src/HToMuMu/root_io/tuples/split_tuples/VBF/ws_VBF --cats VBFcat1,VBFcat2,VBFcat3,VBFcat4,VBFcat5 --procs auto --ext tutorial --mergeYears --skipCOWCorr --batch local --queue espresso
python3 makeDatacard.py --ext tutorial --years combined --prune --doTrueYield --skipCOWCorr --doMCStatUncertainty --saveDataFrame --output Datacard_tutorial

rm yields_tutorial/VBFcat1.pkl
rm yields_tutorial/VBFcat2.pkl
rm yields_tutorial/VBFcat3.pkl
rm yields_tutorial/VBFcat4.pkl
rm yields_tutorial/VBFcat5.pkl
#rm yields_tutorial/VBF.pkl
