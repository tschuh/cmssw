import FWCore.ParameterSet.Config as cms

from L1Trigger.TrackerTFP.Analyzer_cfi import TrackerTFPAnalyzer_params
from L1Trigger.TrackerTFP.Producer_cfi import TrackerTFPProducer_params

TrackerTFPAnalyzerGP = cms.EDAnalyzer( 'trackerTFP::AnalyzerGP', TrackerTFPAnalyzer_params, TrackerTFPProducer_params )
TrackerTFPAnalyzerHT = cms.EDAnalyzer( 'trackerTFP::AnalyzerHT', TrackerTFPAnalyzer_params, TrackerTFPProducer_params )
TrackerTFPAnalyzerMHT = cms.EDAnalyzer( 'trackerTFP::AnalyzerMHT', TrackerTFPAnalyzer_params, TrackerTFPProducer_params )
TrackerTFPAnalyzerLR = cms.EDAnalyzer( 'trackerTFP::AnalyzerLR', TrackerTFPAnalyzer_params, TrackerTFPProducer_params )