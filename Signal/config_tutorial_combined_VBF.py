# Config file: options for signal fitting
import os

_year = 'combined'

signalScriptCfg = {
  
  # Setup
  # 'inputWSDir':'PATH_TO_INPUTS/workspaces/signal_%s'%_year,
  #'inputWSDir':os.getenv('ROOT_IO') + '/tuples/split_tuples/VBF/ws_VBF',
  'inputWSDir':'/inputs/ws_signal',

  'procs':'auto', # if auto: inferred automatically from filenames
  'cats':'auto', # if auto: inferred automatically from (0) workspace
  'ext':'tutorial_%s'%_year,
  'analysis':'tutorial', # To specify which replacement dataset mapping (defined in ./python/replacementMap.py)
  'year':'%s'%_year, # Use 'combined' if merging all years: not recommended
  # 'massPoints':'120,125,130',
  'massPoints':'125',

  #Photon shape systematics  
  'scales':'Scale', # separate nuisance per year
  'scalesCorr':'', # correlated across years
  'scalesGlobal':'', # affect all processes equally, correlated across years
  'smears':'Smearing', # separate nuisance per year

  # Job submission options
  'batch':'local', # ['condor','SGE','IC','local']
  'queue':'espresso',

}
