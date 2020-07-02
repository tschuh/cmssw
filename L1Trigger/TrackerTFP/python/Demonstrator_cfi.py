import FWCore.ParameterSet.Config as cms

TrackerTFPDemonstrator_params = cms.PSet (

  LabelInput   = cms.string( "TrackerTFPProducerMHT"           ),
  LabelOutput  = cms.string( "TrackerTFPProducerLR"            ),
  BranchStubs  = cms.string( "StubAccepted"                    ),
  BranchTracks = cms.string( "TrackAccepted"                   ),
  DirIPBB      = cms.string( "/heplnw039/tschuh/work/proj/lr/" ),
  RunTime      = cms.double( 2.0 )

)