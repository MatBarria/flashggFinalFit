# Add tools dir to PYTHONPATH
eval `scramv1 runtime -sh`
export ROOT_IO=/uscms_data/d3/mbarrial/Hmumu/CMSSW_15_0_19/src/HToMuMu/root_io/

export PYTHONPATH=$PYTHONPATH:${CMSSW_BASE}/src/flashggFinalFit/tools
