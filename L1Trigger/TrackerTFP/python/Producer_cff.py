import FWCore.ParameterSet.Config as cms

from L1Trigger.TrackerDTC.ProducerES_cff import TrackTriggerSetup
from L1Trigger.TrackerTFP.Producer_cfi import TrackerTFPProducer_params

TrackerTFPProducerGP = cms.EDProducer('trackerTFP::ProducerGP', TrackerTFPProducer_params)