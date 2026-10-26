import os
import sys
import json

# cmsswReleaseVersion = "CMSSW_10_6_5"
CMSSW_BASE_DIR = os.getenv('CMSSW_BASE')
SCRAM_ARCH = os.getenv('SCRAM_ARCH')
Condor_BASE_DIR = os.getcwd() + "/"

cmsswReleaseVersion = CMSSW_BASE_DIR.split("/")[-1]
print("Using CMSSW version " + cmsswReleaseVersion)

# Create script to send all of the jobs directly
send_all_jobs = open(Condor_BASE_DIR + "/condor_bias_job_sender.sh", "w+")

os.system("mkdir -p " + Condor_BASE_DIR + "condor/")

categories = ['ggHcat1','ggHcat2','ggHcat3','ggHcat4','ggHcat5','VBFcat1','VBFcat2','VBFcat3','VBFcat4','VBFcat5']
for cat in categories:
    output_dir = Condor_BASE_DIR + "condor/BiasCondor_" + cat
    print(output_dir)

    #make job folders
    os.system("mkdir -p " + output_dir)
    os.system("mkdir -p " + output_dir + "/log/")
    os.system("mkdir -p " + output_dir + "/out/")
    os.system("mkdir -p " + output_dir + "/err/")

    #copy inputs
    os.system("cp Datacard_tutorial_%s_mu_inclusive.root"%(cat) + " %s/Datacard_%s.root"%(output_dir, cat))
    os.system("cp RunBiasStudy.py " + " %s/RunBiasStudy.py"%(output_dir))
    os.system("cp biasUtils.py " + " %s/biasUtils.py"%(output_dir))
    os.system(f"cp {CMSSW_BASE_DIR}/../{cmsswReleaseVersion}.tgz {output_dir}/{cmsswReleaseVersion}.tgz")
    os.system("cp run_nominal_by_cat.sh "+ " %s/run_nominal_by_cat.sh"%(output_dir))

    #make submission file
    jobfile_JDL = open(output_dir + "/task.jdl", "w+")
    jobfile_JDL.write("Universe  = vanilla" + "\n")
    jobfile_JDL.write("Executable = ./run_nominal_by_cat.sh" + "\n")

    jobfile_JDL.write("Arguments = " + cat + " " + cmsswReleaseVersion + "\n")

    jobfile_JDL.write("Log = log/$(cat).log" + "\n")
    jobfile_JDL.write("Output = out/$(cat).out" + "\n")
    jobfile_JDL.write("Error = err/$(cat).err" + "\n")
    jobfile_JDL.write("x509userproxy = $ENV(X509_USER_PROXY)" + "\n")

    transfer_files = output_dir + "/Datacard_%s.root, "%(cat)
    transfer_files += output_dir + "/RunBiasStudy.py, "
    transfer_files += output_dir + "/biasUtils.py, "
    transfer_files += output_dir + "/run_nominal_by_cat.sh, "
    transfer_files += output_dir + f"/{cmsswReleaseVersion}.tgz, "

    jobfile_JDL.write("transfer_input_files = " + transfer_files + "\n")
    jobfile_JDL.write("should_transfer_files = YES" + "\n")
    jobfile_JDL.write("when_to_transfer_output = ON_EXIT" + "\n\n# Resources request\n")
    jobfile_JDL.write("transfer_output_files = BiasResults/\n")
    jobfile_JDL.write("RequestMemory = 4500 \n\n# Jobs selection\n")

    jobfile_JDL.write("Queue cat in " + cat)
    jobfile_JDL.write("\n")
    jobfile_JDL.close()

    print("Send single job with:")
    print(" > cd " + output_dir + "; condor_submit task.jdl")
    send_all_jobs.write("cd " + output_dir + "\n")
    send_all_jobs.write("condor_submit task.jdl" + "\n")

print("\n----- End -----")
print("Run all generated jobs with:")
print(" > bash condor_bias_job_sender.sh")
    
send_all_jobs.close()