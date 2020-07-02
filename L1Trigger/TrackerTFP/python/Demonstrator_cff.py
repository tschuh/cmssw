import FWCore.ParameterSet.Config as cms

from L1Trigger.TrackerTFP.Demonstrator_cfi import TrackerTFPDemonstrator_params

TrackerTFPDemonstrator = cms.EDAnalyzer( 'trackerTFP::Demonstrator', TrackerTFPDemonstrator_params )