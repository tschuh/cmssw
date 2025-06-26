# configuartion for L1 GTT Producer
import FWCore.ParameterSet.Config as cms

L1GTTProducer_params = cms.PSet (

  InputLabelTF     = cms.string( "ProducerTFP"   ),  #
  InputLabelTR     = cms.string( "ProducerTF"    ),  #
  Branch           = cms.string( "TrackAccepted" ),  # branch for prodcut with passed tracks

)
