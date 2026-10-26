mkdir -p Models/signal Models/background

cp -v ../Signal/outdir_packaged/CMS-HGG_sigfit_packaged*.root Models/signal/
cp -v ../Background/outdir_tutorial/CMS-HGG_multipdf*.root Models/background/
cp -v ../Datacard/Datacard_tutorial*.txt .
