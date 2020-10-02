import FWCore.ParameterSet.Config as cms

TrackTriggerKalmanFilterFormats_params = cms.PSet (

    BaseShiftr0     = cms.int32(  -3 ),
    BaseShiftr02    = cms.int32(  -5 ),
    BaseShiftv0     = cms.int32(  -2 ),
    BaseShiftS00    = cms.int32(  -1 ),
    BaseShiftS01    = cms.int32(  -7 ),
    BaseShiftK00    = cms.int32(  -9 ),
    BaseShiftK10    = cms.int32( -15 ),
    BaseShiftR00    = cms.int32(  -2 ),
    BaseShiftInvR00 = cms.int32( -19 ),
    BaseShiftChi20  = cms.int32(  -5 ),
    BaseShiftC00    = cms.int32(   5 ),
    BaseShiftC01    = cms.int32(  -3 ),
    BaseShiftC11    = cms.int32(  -7 ),

    BaseShiftr1     = cms.int32(   2 ),
    BaseShiftr12    = cms.int32(   5 ),
    BaseShiftv1     = cms.int32(   3 ),
    BaseShiftS12    = cms.int32(  -3 ),
    BaseShiftS13    = cms.int32(  -3 ),
    BaseShiftK21    = cms.int32( -13 ),
    BaseShiftK31    = cms.int32( -14 ),
    BaseShiftR11    = cms.int32(   3 ),
    BaseShiftInvR11 = cms.int32( -21 ),
    BaseShiftChi21  = cms.int32(  -5 ),
    BaseShiftC22    = cms.int32(  -3 ),
    BaseShiftC23    = cms.int32(  -5 ),
    BaseShiftC33    = cms.int32(  -3 ),

    BaseShiftChi2   = cms.int32(  -5 )

)