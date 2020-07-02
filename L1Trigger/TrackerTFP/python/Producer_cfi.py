import FWCore.ParameterSet.Config as cms

TrackerTFPProducer_params = cms.PSet (

  LabelDTC         = cms.string( "TrackerDTCProducer"    ), #
  LabelGP          = cms.string( "TrackerTFPProducerGP"  ), #
  LabelHT          = cms.string( "TrackerTFPProducerHT"  ), #
  LabelMHT         = cms.string( "TrackerTFPProducerMHT" ), #
  LabelLR          = cms.string( "TrackerTFPProducerLR"  ), #
  #LabelSF          = cms.string( "TrackerTFPProducerSF"  ), #
  #LabelKF          = cms.string( "TrackerTFPProducerKF"  ), #
  #LabelDR          = cms.string( "TrackerTFPProducerDR"  ), #
  BranchAccepted   = cms.string( "StubAccepted"  ),         # branch for prodcut with passed stubs
  BranchLost       = cms.string( "StubLost"      ),         # branch for prodcut with lost stubs
  BranchTracks     = cms.string( "TrackAccepted" ),         # branch for prodcut with passed track information
  CheckHistory     = cms.bool  ( True  ),                   # checks if input sample production is configured as current process
  EnableTruncation = cms.bool  ( True  )                    # enable emulation of truncation, lost stubs are filled in BranchLost

)