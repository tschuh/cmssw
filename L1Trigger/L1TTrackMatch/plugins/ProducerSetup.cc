#include "FWCore/Framework/interface/ESProducer.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/ESInputTag.h"

#include "L1Trigger/TrackTrigger/interface/Setup.h"
#include "L1Trigger/L1TTrackMatch/interface/Setup.h"

#include <memory>

namespace l1GTT {

  /*! \class  l1GTT::ProducerSetup
   *  \brief  Class providing relevant track trigger and and GTT configuration parameter
   *  \author Thomas Schuh
   *  \date   2025, June
   */
  class ProducerSetup : public edm::ESProducer {
  public:
    ProducerSetup(const edm::ParameterSet& iConfig);
    ~ProducerSetup() override = default;
    std::unique_ptr<Setup> produce(const tt::SetupRcd& rcd);

  private:
    edm::ESGetToken<tt::Setup, tt::SetupRcd> esGetToken_;
    Setup::Config config_;
  };

  ProducerSetup::ProducerSetup(const edm::ParameterSet& iConfig) {
    auto cc = setWhatProduced(this);
    esGetToken_ = cc.consumes();
    config_.enableTruncation_ = iConfig.getParameter<bool>("EnableTruncation");
    const edm::ParameterSet& psTF = iConfig.getParameter<edm::ParameterSet>("TrackFormatter");
    config_.tfPartialWidth_ = psTF.getParameter<int>("PartialWidth");
    config_.tfPartialFast_ = psTF.getParameter<int>("PartialFast");
    config_.tfPartialSlow_ = psTF.getParameter<int>("PartialSlow");
    const edm::ParameterSet& psTR = iConfig.getParameter<edm::ParameterSet>("TrackRouter");
    config_.trNumBins_ = psTR.getParameter<int>("NumBins");
    config_.trNumChannel_ = psTR.getParameter<int>("NumChannel");
    config_.trDepthMemory_ = psTR.getParameter<int>("DepthMemory");
    config_.trC0_ = psTR.getParameter<double>("C0");
    config_.trC1_ = psTR.getParameter<double>("C1");
    config_.trC2_ = psTR.getParameter<double>("C2");
  }

  std::unique_ptr<Setup> ProducerSetup::produce(const tt::SetupRcd& rcd) {
    const tt::Setup* setup = &rcd.get(esGetToken_);
    return std::make_unique<Setup>(setup, config_);
  }

}  // namespace l1GTT

DEFINE_FWK_EVENTSETUP_MODULE(l1GTT::ProducerSetup);
