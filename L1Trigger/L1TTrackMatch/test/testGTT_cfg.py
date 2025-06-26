# this runs L1 tracking + GTT emulators
import FWCore.ParameterSet.Config as cms

process = cms.Process( "Demo" )
process.load( 'FWCore.MessageService.MessageLogger_cfi' )
process.load( 'Configuration.EventContent.EventContent_cff' )
process.load( 'Configuration.Geometry.GeometryExtendedRun4D110Reco_cff' ) 
process.load( 'Configuration.Geometry.GeometryExtendedRun4D110_cff' )
process.load( 'Configuration.StandardSequences.MagneticField_cff' )
process.load( 'Configuration.StandardSequences.FrontierConditions_GlobalTag_cff' )
process.load( 'L1Trigger.TrackTrigger.TrackTrigger_cff' )

from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

# load code that associates stubs with mctruth
process.load( 'SimTracker.TrackTriggerAssociation.StubAssociator_cff' )
# load code that produces DTCStubs
#process.load( 'L1Trigger.TrackerDTC.DTC_cff' )
# L1 tracking => hybrid emulation 
process.load( 'L1Trigger.TrackFindingTracklet.L1HybridEmulationTracks_cff' )
process.l1tTTTracksFromTrackletEmulation.readMoreMcTruth = False
# load code that fits hybrid tracks
process.load( 'L1Trigger.TrackFindingTracklet.Producer_cff' )
from L1Trigger.TrackFindingTracklet.Customize_cff import *
fwConfig( process )
# load code that emulates gtt
process.load( 'L1Trigger.L1TTrackMatch.Producer_cff' )
# load code that analyses gtt
process.load( 'L1Trigger.L1TTrackMatch.Analyzer_cff' )

# build schedule
process.mc  = cms.Sequence( process.StubAssociator )
#process.dtc = cms.Sequence( process.ProducerDTC )
process.tfp = cms.Sequence( process.L1THybridTracks + process.ProducerTM + process.ProducerDR + process.ProducerKF + process.ProducerTQ + process.ProducerTFP )
process.gtt = cms.Sequence( process.ProducerTF + process.AnalyzerTF + process.ProducerTR + process.AnalyzerTR + process.ProducerVF + process.AnalyzerVF )
#process.tt  = cms.Path( process.mc + process.dtc + process.tfp + process.gtt )
process.schedule = cms.Schedule( process.tt )

# create options
import FWCore.ParameterSet.VarParsing as VarParsing
options = VarParsing.VarParsing( 'analysis' )
# specify input MC
Samples = ["/store/mc/Phase2Spring24DIGIRECOMiniAOD/TT_TuneCP5_14TeV-powheg-pythia8/GEN-SIM-DIGI-RAW-MINIAOD/PU200_Trk1GeV_140X_mcRun4_realistic_v4-v2/130000/00c7f40e-b44e-4eea-a86b-def8f7d82b0e.root"]
options.register( 'inputMC', Samples, VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.string, "Files to be processed" )
# specify number of events to process.
options.register( 'Events',100,VarParsing.VarParsing.multiplicity.singleton, VarParsing.VarParsing.varType.int, "Number of Events to analyze" )
options.parseArguments()

process.options = cms.untracked.PSet( wantSummary = cms.untracked.bool(False) )
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(options.Events) )
process.source = cms.Source(
  "PoolSource",
  fileNames = cms.untracked.vstring( options.inputMC ),
  #skipEvents = cms.untracked.uint32( 3537 ),
  secondaryFileNames = cms.untracked.vstring(),
  duplicateCheckMode = cms.untracked.string( 'noDuplicateCheck' )
)
process.Timing = cms.Service( "Timing", summaryOnly = cms.untracked.bool( True ) )
process.MessageLogger.cerr.enableStatistics = False
process.MessageLogger.L1track = dict(limit = -1)
process.TFileService = cms.Service( "TFileService", fileName = cms.string( "Hist.root" ) )

if ( False ):
  process.out = cms.OutputModule (
    "PoolOutputModule",
    fileName = cms.untracked.string("L1Tracks.root"),
    fastCloning = cms.untracked.bool( False ),
    outputCommands = cms.untracked.vstring('drop *', 'keep *_TTTrack*_*_*' )
  )
  process.FEVToutput_step = cms.EndPath( process.out )
  process.schedule.append( process.FEVToutput_step )
