# Produce L1 tracks with TMTT C++ emulation
import FWCore.ParameterSet.Config as cms

from L1Trigger.L1TTrackMatch.Setup_cff import GlobalTrackTriggerSetup
from L1Trigger.L1TTrackMatch.Producer_cfi import L1GTTProducer_params

ProducerTF = cms.EDProducer( 'l1GTT::ProducerTF' , TrackerGTTProducer_params )
ProducerTR = cms.EDProducer( 'l1GTT::ProducerTR' , TrackerGTTProducer_params )
