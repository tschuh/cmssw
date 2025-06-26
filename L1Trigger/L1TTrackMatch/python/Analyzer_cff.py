# EDAnalyzer for Track Trigger emulation steps
import FWCore.ParameterSet.Config as cms

from L1Trigger.L1TTrackMatch.Analyzer_cfi import L1GTTAnalyzer_params
from L1Trigger.L1TTrackMatch.Producer_cfi import L1GTTProducer_params

AnalyzerTF = cms.EDAnalyzer( 'l1GTT::AnalyzerTF' , L1GTTAnalyzer_params, L1GTTProducer_params )
AnalyzerTR = cms.EDAnalyzer( 'l1GTT::AnalyzerTR' , L1GTTAnalyzer_params, L1GTTProducer_params )
