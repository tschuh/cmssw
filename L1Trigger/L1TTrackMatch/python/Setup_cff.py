# ESProducer processing and providing run-time constants used by Global Track Trigger emulators
import FWCore.ParameterSet.Config as cms

from L1Trigger.TrackTrigger.Setup_cff import TrackTriggerSetup
from L1Trigger.L1TrackMatch.Setup_cfi import GlobalTrackTrigger_params

GlobalTrackTriggerSetup = cms.ESProducer("l1GTT::ProducerSetup", GlobalTrackTrigger_params)
