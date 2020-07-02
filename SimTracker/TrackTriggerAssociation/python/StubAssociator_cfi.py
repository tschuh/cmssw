import FWCore.ParameterSet.Config as cms

StubAssociator_params = cms.PSet (

  InputTagTTStubDetSetVec = cms.InputTag( "TTStubsFromPhase2TrackerDigis",     "StubAccepted"     ), #
  InputTagTTClusterAssMap = cms.InputTag( "TTClusterAssociatorFromPixelDigis", "ClusterAccepted"  ), #
  BranchReconstructable   = cms.string  ( "Reconstructable" ),                                       #
  BranchSelection         = cms.string  ( "UseForAlgEff"    )                                        #

)