################################################################################################
# To run execute do
# cmsRun L1Trigger/L1TTrackerTFP/test/gp_cfg.py
# where the arguments take default values if you don't specify them. You can change defaults below.
#################################################################################################

import FWCore.ParameterSet.Config as cms

process = cms.Process( "Demo" )
process.load( 'Configuration.Geometry.GeometryExtended2026D49Reco_cff' )
process.load( 'Configuration.Geometry.GeometryExtended2026D49_cff' )
process.load( 'Configuration.StandardSequences.MagneticField_cff' )
process.load( 'Configuration.StandardSequences.FrontierConditions_GlobalTag_cff' )
process.load( 'Configuration.StandardSequences.L1TrackTrigger_cff' )

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag( process.GlobalTag, 'auto:phase2_realistic', '' )

#--- Load code that produces DTCStubs
process.load( 'L1Trigger.TrackerDTC.Producer_cff' )
process.dtc = cms.Path( process.TrackerDTCProducer )
from L1Trigger.TrackerDTC.Producer_Customize_cff import useTMTT
useTMTT(process)

#--- Load code that produces GPStubs
process.load( 'L1Trigger.TrackerTFP.ProducerGP_cff' )
process.gp = cms.Path( process.TrackerTFPProducerGP )

process.schedule = cms.Schedule( process.dtc, process.gp )

import FWCore.Utilities.FileUtils as FileUtils
import FWCore.ParameterSet.VarParsing as VarParsing
options = VarParsing.VarParsing( 'analysis' )
#--- Specify input MC
#options.register( 'inputMC', 'L1Trigger/TrackerTFP/test/MCsamples/1110/RelVal/SingleMuPt2To100/noPU.txt',
#options.register( 'inputMC', 'L1Trigger/TrackerDTC/test/MCsamples/1110/RelVal/SingleMuPt0p7To10/noPU.txt',
options.register( 'inputMC', 'L1Trigger/TrackerTFP/test/MCsamples/1110/RelVal/TTbar/PU200.txt',
VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.string, "Files to be processed" )
#--- Specify number of events to process.
options.register( 'Events',100,VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.int, "Number of Events to analyze" )
options.parseArguments()

process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(options.Events) )
process.source = cms.Source(
  "PoolSource",
  fileNames = cms.untracked.vstring( *FileUtils.loadListFromFile( options.inputMC ) ),
  #skipEvents = cms.untracked.uint32( 9*72 ),
  secondaryFileNames = cms.untracked.vstring(),
  duplicateCheckMode = cms.untracked.string( 'noDuplicateCheck' )
)
process.Timing = cms.Service( "Timing", summaryOnly = cms.untracked.bool( True ) )