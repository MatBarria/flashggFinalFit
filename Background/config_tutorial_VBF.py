# Config file: options for signal fitting
import os
backgroundScriptCfg = {
  
  # Setup
  # 'inputWS':'PATH_TO_INPUTS/workspaces/data/allData_2022preEE.root', # location of 'allData.root' file
#/uscms/home/christiw/nobackup/Hmm/CMSSW_14_0_14/src/HToMuMu/root_io_matias
  'inputWS':os.getenv('ROOT_IO') + '/tuples/split_tuples/before2sigsources/VBF/ws/allData_combined.root', # location of 'allData.root' file
#  'inputWS':'/uscms_data/d3/mbarrial/higgsAnalysis/CMSSW_14_0_14/src/HToMuMu/root_io//tuples/split_tuples/VBF/ws/allData_combined.root', # location of 'allData.root' file

  'cats':'auto', # auto: automatically inferred from input ws
  'catOffset':0, # add offset to category numbers (useful for categories from different allData.root files)  
  'ext':'tutorial', # extension to add to output directory
  'year':'combined', # Use combined when merging all years in category (for plots)
  'nBinsPerGeV':4, #number of bins used in the fitting

  # Job submission options
  'batch':'local', # [condor,SGE,IC,local]
  'queue':'espresso' # for condor e.g. microcentury
  
}
