#!/usr/bin/env bash
#script to automatically set up CMSSW to obtain emulated track finder output

source /cvmfs/cms.cern.ch/cmsset_default.sh
cmsrel CMSSW_13_3_0_pre2
cd CMSSW_13_3_0_pre2/src/
cmsenv
git cms-checkout-topic -u cms-L1TK:fw_synch_231205

#update Settings.h for comparison with fw
sed -i 's/{"IR", 156}/{"IR", 108}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h
sed -i 's/writeMem_{false}/writeMem_{true}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h
sed -i 's/writeTable_{false}/writeTable_{true}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h
sed -i 's/writeConfig_{false}/writeConfig_{true}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h
sed -i 's/writeHLSInvTable_{false}/writeHLSInvTable_{true}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h
sed -i 's/doMultipleMatches_{true}/doMultipleMatches_{false}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h #check this?
sed -i 's/combined_{false}/combined_{true}/g' L1Trigger/TrackFindingTracklet/interface/Settings.h

#update KFin to skip duplicated removal for comparison with FW
sed -i 's/const string& label = iConfig\.getParameter<string>("LabelDR");/const string\& label = iConfig\.getParameter<string>("LabelDRin");/g' L1Trigger/TrackFindingTracklet/plugins/ProducerKFin.cc

#copy new driver/configuration
cd -
cp AnalyzerDemonstrator2.cc CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/test/
cp demonstrator2_cfg.py CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/test/

#copy reduced and barrel memory modules
git clone --recurse-submodules https://github.com/cms-L1TK/firmware-hls.git
cd firmware-hls/emData/
./download.sh
cd -
mkdir CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/
cp firmware-hls/emData/project_generation_scripts/reducedcm_memorymodules.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/reduced_memorymodules.dat
cp firmware-hls/emData/project_generation_scripts/reducedcm_processingmodules.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/reduced_processingmodules.dat
cp firmware-hls/emData/project_generation_scripts/reducedcm_wires.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/reduced_wires.dat
cp firmware-hls/emData/project_generation_scripts/cmbarrel_memorymodules.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/
cp firmware-hls/emData/project_generation_scripts/cmbarrel_processingmodules.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/
cp firmware-hls/emData/project_generation_scripts/cmbarrel_wires.dat CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/data/
rm -rf firmware-hls

#changes for barrel project
if [ "$1" == "barrel" ];
then
  rm CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/plugins/L1FPGATrackProducer.cc
  cp L1FPGATrackProducer_barrel.cc CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/plugins/L1FPGATrackProducer.cc
  sed -i 's/barrel_config = False/barrel_config = True/g' CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/test/demonstrator2_cfg.py
fi

#compile
cd CMSSW_13_3_0_pre2/src/
scram b -j8

#then run with
cd L1Trigger/TrackFindingTracklet/test/
cmsRun demonstrator2_cfg.py
