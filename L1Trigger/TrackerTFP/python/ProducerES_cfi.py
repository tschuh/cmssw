import FWCore.ParameterSet.Config as cms

TrackTriggerDataFormats_params = cms.PSet (

  SeedFilter = cms.PSet (

    WidthZ0  = cms.int32( 5 ),
    WidthCot = cms.int32( 5 )

  ),

  DuplicateRemoval = cms.PSet (
    WidthPhi0    = cms.int32( 12 ), # number of bist used for phi0
    WidthQoverPt = cms.int32( 15 ), # number of bist used for qOverPt
    WidthCot     = cms.int32( 16 ), # number of bist used for cot(theta)
    WidthZ0      = cms.int32( 12 )  # number of bist used for z0
  ),

)