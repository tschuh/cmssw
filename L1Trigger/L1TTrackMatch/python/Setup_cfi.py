# configuration for GlobalTrackTriggerSetup
import FWCore.ParameterSet.Config as cms

GlobalTrackTrigger_params = cms.PSet (

  EnableTruncation = cms.bool  ( True ), # enable emulation of truncation

  # Parmeter specifying TrackFormatter
  TrackFormatter = cms.PSet (
    PartialWidth = cms.int32( 32 ), # number of bits used to describe one part of a track (96 bit)
    PartialFast  = cms.int32(  3 ), # number of track parts forming one track
    PartialSlow  = cms.int32(  2 )  # number of track parts send per clock tick (TFP sends 2/3 tracks per clock and link)
  ),

  # Parmeter specifying Track TrackRouter
  TrackRouter = cms.PSet (
    NumBins     = cms.int32 ( 256 ),       # number of bins in z0
    NumChannel  = cms.int32 (   8 ),       # number of output channel
    DepthMemory = cms.int32 (  32 ),       # internal memory depth
    C0          = cms.double(   0.09867 ), # z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    C1          = cms.double(   0.0007  ), # z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    C2          = cms.double(   0.0587  )  # z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
  )

)
