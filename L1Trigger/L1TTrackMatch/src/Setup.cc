#include "L1Trigger/L1TTrackMatch/interface/Setup.h"

namespace l1GTT {

  Setup::Setup(const tt::Setup* setup, const Config& config)
      : setup_(setup),
        tfPartialWidth_(config.tfPartialWidth_),
        tfPartialFast_(config.tfPartialFast_),
        tfPartialSlow_(config.tfPartialSlow_),
        trNumBins_(config.trNumBins_),
        trNumChannel_(config.trNumChannel_),
        trDepthMemory_(config.trDepthMemory_) {
    // derived constants
    tfNumChannel_ = this->tfpNumRegions() * this->tfpNumChannel() * tfPartialSlow_ / tfPartialFast_;
    tfNumMergedRegions_ = this->tfpNumRegions() / tfPartialFast_;
    trNumMerged_ = trNumBins_ / trNumChannel_;
  }

}  // namespace l1GTT
