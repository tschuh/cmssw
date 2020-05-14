import FWCore.ParameterSet.Config as cms

TrackerTFPProducer_params = cms.PSet (

  LabelDTC         = cms.string( "trackerDTCProducer"    ), #
  LabelGP          = cms.string( "trackerFTPProducerGP"  ), #
  LabelHT          = cms.string( "trackerFTPProducerHT"  ), #
  LabelMHT         = cms.string( "trackerFTPProducerMHT" ), #
  LabelSF          = cms.string( "trackerFTPProducerSF"  ), #
  LabelKF          = cms.string( "trackerFTPProducerKF"  ), #
  LabelDR          = cms.string( "trackerFTPProducerDR"  ), #
  BranchAccepted   = cms.string( "StubAccepted" ),          # label for prodcut with passed stubs
  BranchLost       = cms.string( "StubLost"     ),          # label for prodcut with lost stubs
  CheckHistory     = cms.bool  ( True  ),                   # checks if input sample production is configured as current process
  EnableTruncation = cms.bool  ( True  )                    # enable emulation of truncation, lost stubs are filled in BranchLost

)