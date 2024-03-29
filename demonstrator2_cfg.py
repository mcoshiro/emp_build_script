# this compares event by event the output of the C++ emulation with the ModelSim simulation of the firmware
import FWCore.ParameterSet.Config as cms

process = cms.Process( "Demo" )
process.load( 'FWCore.MessageService.MessageLogger_cfi' )
process.load( 'Configuration.EventContent.EventContent_cff' )
process.load( 'Configuration.Geometry.GeometryExtended2026D88Reco_cff' ) 
process.load( 'Configuration.Geometry.GeometryExtended2026D88_cff' )
process.load( 'Configuration.StandardSequences.MagneticField_cff' )
process.load( 'Configuration.StandardSequences.FrontierConditions_GlobalTag_cff' )
process.load( 'L1Trigger.TrackTrigger.TrackTrigger_cff' )

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

# load code that produces DTCStubs
process.load( 'L1Trigger.TrackerDTC.ProducerED_cff' )
# L1 tracking => hybrid emulation 
process.load("L1Trigger.TrackFindingTracklet.L1HybridEmulationTracks_cff")
# load code that fits hybrid tracks
process.load( 'L1Trigger.TrackFindingTracklet.Producer_cff' )
# create demonstrator
TrackTriggerDemonstrator_params = cms.PSet (
  LabelOut = cms.string( "TrackFindingTrackletProducerKFout"   )
)
from L1Trigger.TrackFindingTracklet.Producer_cfi import TrackFindingTrackletProducer_params
process.TrackerTFPDemonstrator = cms.EDAnalyzer("trklet::AnalyzerDemonstrator2", TrackTriggerDemonstrator_params, TrackFindingTrackletProducer_params)


# build schedule
process.tt = cms.Sequence (  process.TrackerDTCProducer
                           + process.L1THybridTracks
                           + process.TrackFindingTrackletProducerIRin
                           + process.TrackFindingTrackletProducerTBout
                           + process.TrackFindingTrackletProducerDRin
#                           + process.TrackFindingTrackletProducerDR
                           + process.TrackFindingTrackletProducerKFin
                           + process.TrackFindingTrackletProducerKF 
                           + process.TrackFindingTrackletProducerTT 
                           + process.TrackFindingTrackletProducerAS 
                           + process.TrackFindingTrackletProducerKFout
                          )




from L1Trigger.TrackFindingTracklet.Customize_cff import *

barrel_config = False
if (barrel_config):
  fwConfig(process)
  process.l1tTTTracksFromTrackletEmulation.memoryModulesFile = 'L1Trigger/TrackFindingTracklet/data/cmbarrel_memorymodules.dat'
  process.l1tTTTracksFromTrackletEmulation.processingModulesFile = 'L1Trigger/TrackFindingTracklet/data/cmbarrel_processingmodules.dat'
  process.l1tTTTracksFromTrackletEmulation.wiresFile = 'L1Trigger/TrackFindingTracklet/data/cmbarrel_wires.dat'
else:
  reducedConfig( process )

process.demo = cms.Path( process.tt + process.TrackerTFPDemonstrator )
process.schedule = cms.Schedule( process.demo )

# create options
import FWCore.ParameterSet.VarParsing as VarParsing
options = VarParsing.VarParsing( 'analysis' )
# specify input MC
inputMC = ["/store/mc/CMSSW_12_6_0/RelValTTbar_14TeV/GEN-SIM-DIGI-RAW/PU_125X_mcRun4_realistic_v5_2026D88PU200RV183v2-v1/30000/0959f326-3f52-48d8-9fcf-65fc41de4e27.root"]
#inputMC = ["file:/mnt/scratch1/mco62/CMSSW_13_3_0_pre2/src/L1Trigger/TrackFindingTracklet/test/TTbar_PU200_D88.root"]

options.register( 'inputMC', inputMC, VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.string, "Files to be processed" )
# specify number of events to process.
options.register( 'Events',100 ,VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.int, "Number of Events to analyze" )
options.parseArguments()

process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(options.Events) )
process.source = cms.Source(
  "PoolSource",
  fileNames = cms.untracked.vstring( options.inputMC ),
  #skipEvents = cms.untracked.uint32( 1 ),
  secondaryFileNames = cms.untracked.vstring(),
  duplicateCheckMode = cms.untracked.string( 'noDuplicateCheck' )
)
process.Timing = cms.Service( "Timing", summaryOnly = cms.untracked.bool( True ) )
