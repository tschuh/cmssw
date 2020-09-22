import FWCore.ParameterSet.Config as cms

from L1Trigger.TrackerDTC.ProducerES_cff import TrackTriggerSetup
from L1Trigger.TrackerTFP.Producer_cfi import TrackerTFPProducer_params
from L1Trigger.TrackerTFP.ProducerES_cff import TrackTriggerDataFormats
from L1Trigger.TrackerTFP.ProducerLayerEncoding_cff import TrackTriggerLayerEncoding
from L1Trigger.TrackerTFP.KalmanFilterFormats_cff import TrackTriggerKalmanFilterFormats

TrackerTFPProducerGP = cms.EDProducer( 'trackerTFP::ProducerGP', TrackerTFPProducer_params )
TrackerTFPProducerHT = cms.EDProducer( 'trackerTFP::ProducerHT', TrackerTFPProducer_params )
TrackerTFPProducerMHT = cms.EDProducer( 'trackerTFP::ProducerMHT', TrackerTFPProducer_params )
TrackerTFPProducerSF = cms.EDProducer( 'trackerTFP::ProducerSF', TrackerTFPProducer_params )
TrackerTFPProducerSFout = cms.EDProducer( 'trackerTFP::ProducerSFout', TrackerTFPProducer_params )
TrackerTFPProducerKFin = cms.EDProducer( 'trackerTFP::ProducerKFin', TrackerTFPProducer_params )
TrackerTFPProducerKF = cms.EDProducer( 'trackerTFP::ProducerKF', TrackerTFPProducer_params )