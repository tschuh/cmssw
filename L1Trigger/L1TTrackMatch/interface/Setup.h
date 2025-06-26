#ifndef L1Trigger_L1TTrackMatch_Setup_h
#define L1Trigger_L1TTrackMatch_Setup_h

#include "FWCore/Framework/interface/data_default_record_trait.h"
#include "L1Trigger/TrackTrigger/interface/Setup.h"

namespace l1GTT {

  /*! \class  l1GTT::Setup
   *  \brief  Class providing relevant track trigger and and GTT configuration parameter
   *  \author Thomas Schuh
   *  \date   20205, June
   */
  class Setup {
  public:
    struct Config {
      bool enableTruncation_;
      int tfPartialWidth_;
      int tfPartialFast_;
      int tfPartialSlow_;
      int trNumBins_;
      int trNumChannel_;
      int trDepthMemory_;
      double trC0_;
      double trC1_;
      double trC2_;
    };
    Setup() {}
    Setup(const tt::Setup* setup, const Config& config);
    ~Setup() = default;

    // enable emulation of truncation
    bool enableTruncation() const { return enableTruncation_; }
    // width of the 'A' port of an DSP slice
    int widthDSPa() const { return setup_->widthDSPa(); }
    // width of the 'A' port of an DSP slice using biased twos complement
    int widthDSPab() const { return setup_->widthDSPab(); }
    // width of the 'A' port of an DSP slice using biased binary
    int widthDSPau() const { return setup_->widthDSPau(); }
    // width of the 'B' port of an DSP slice
    int widthDSPb() const { return setup_->widthDSPb(); }
    // width of the 'B' port of an DSP slice using biased twos complement
    int widthDSPbb() const { return setup_->widthDSPbb(); }
    // width of the 'B' port of an DSP slice using biased binary
    int widthDSPbu() const { return setup_->widthDSPbu(); }
    // width of the 'C' port of an DSP slice
    int widthDSPc() const { return setup_->widthDSPc(); }
    // width of the 'C' port of an DSP slice using biased twos complement
    int widthDSPcb() const { return setup_->widthDSPcb(); }
    // width of the 'C' port of an DSP slice using biased binary
    int widthDSPcu() const { return setup_->widthDSPcu(); }
    // smallest address width of an BRAM36 configured as broadest simple dual port memory
    int widthAddrBRAM36() const { return setup_->widthAddrBRAM36(); }
    // smallest address width of an BRAM18 configured as broadest simple dual port memory
    int widthAddrBRAM18() const { return setup_->widthAddrBRAM18(); }
    // needed gap between events of emp-infrastructure firmware
    int numFramesInfra() const { return setup_->numFramesInfra(); }
    // number of frames betwen 2 resets of 18 BX packets
    int numFrames() const { return 0; }  //setup_->numFramesHigh(); }
    // number of valid frames per 18 BX packet
    int numFramesIO() const { return 0; }  //setup_->numFramesIOHigh(); }
    // number of phi slices the outer tracker readout is organized in
    int tfpNumRegions() const { return setup_->numRegions(); }
    // number of track finding processor output links
    int tfpNumChannel() const { return setup_->tfpNumChannel(); }
    // number of bits used to describe one part of a track (96 bit)
    int tfPartialWidth() const { return tfPartialWidth_; }
    // number of track parts forming one track
    int tfPartialFast() const { return tfPartialFast_; }
    // number of track parts send per clock tick (TFP sends 2/3 tracks per clock and link)
    int tfPartialSlow() const { return tfPartialSlow_; }
    // number of Track Formatter output channel
    int tfNumChannel() const { return tfNumChannel_; }
    // number of merged regions
    int tfNumMergedRegions() const { return tfNumMergedRegions_; }
    // number of bins in z0
    int trNumBins() const { return trNumBins_; }
    // number of output channel
    int trNumChannel() const { return trNumChannel_; }
    // internal memory depth
    int trDepthMemory() const { return trDepthMemory_; }
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC0() const { return trC0_; }
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC1() const { return trC1_; }
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC2() const { return trC2_; }
    /// number of merged bins
    int trNumMerged() const { return trNumMerged_; }

  private:
    // provides track trigger run-time constants
    const tt::Setup* setup_;
    // enable emulation of truncation
    bool enableTruncation_;
    // number of bits used to describe one part of a track (96 bit)
    int tfPartialWidth_;
    // number of track parts forming one track
    int tfPartialFast_;
    // number of track parts send per clock tick (TFP sends 2/3 tracks per clock and link)
    int tfPartialSlow_;
    // number of Track Formatter output channel
    int tfNumChannel_;
    // number of merged regions
    int tfNumMergedRegions_;
    // number of bins in z0
    int trNumBins_;
    // number of output channel
    int trNumChannel_;
    // internal memory depth
    int trDepthMemory_;
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC0_;
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC1_;
    // z resolution modeling dZ = C0 + C1 * eta + C2 * eta ** 2
    double trC2_;
    /// number of merged bins
    int trNumMerged_;
  };

}  // namespace l1GTT

EVENTSETUP_DATA_DEFAULT_RECORD(l1GTT::Setup, tt::SetupRcd);

#endif
