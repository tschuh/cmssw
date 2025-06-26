#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/L1TrackTrigger/interface/TTTypes.h"
#include "L1Trigger/TrackTrigger/interface/Setup.h"
#include "L1Trigger/L1TTrackMatch/interface/Setup.h"
#include "L1Trigger/L1TTrackMatch/interface/TrackRouter.h"

#include <string>
#include <utility>

namespace l1GTT {

  /*! \class  l1GTT::ProducerTR
   *  \brief  Emulates GTT Track Router. This step assignes tracks to multiple fine z0 bins (~0.16 cm) accoring to
              expected z0 track resolution. It routes the 12 = 2 (+- eta) x 3 (3 combinened phi nonants) channel into
              into 8 channel containing 32 equidistant z0 bins
   *  \author Thomas Schuh
   *  \date   2025, June
   */
  class ProducerTR : public edm::stream::EDProducer<> {
  public:
    explicit ProducerTR(const edm::ParameterSet&);
    ~ProducerTR() override = default;

  private:
    void produce(edm::Event&, const edm::EventSetup&) override;
    // ED input token of tfp tracks
    edm::EDGetTokenT<tt::StreamsTT> edGetToken_;
    // ED output token for tracks
    edm::EDPutTokenT<tt::StreamsTT> edPutToken_;
    // Setup token
    edm::ESGetToken<Setup, tt::SetupRcd> esGetToken_;
  };

  ProducerTR::ProducerTR(const edm::ParameterSet& iConfig) {
    const std::string& label = iConfig.getParameter<std::string>("InputLabelTR");
    const std::string& branch = iConfig.getParameter<std::string>("Branch");
    // book in- and output ED products
    edGetToken_ = consumes<tt::StreamsTT>(edm::InputTag(label, branch));
    edPutToken_ = produces<tt::StreamsTT>(branch);
    // book ES products
    esGetToken_ = esConsumes();
  }

  void ProducerTR::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    // helper class to store configurations
    const Setup* setup = &iSetup.getData(esGetToken_);
    // emulator
    TrackRouter tr(setup);
    // empty TR product
    tt::StreamsTT output;
    // read in TF Product
    const tt::StreamsTT& input = iEvent.get(edGetToken_);
    // produce output
    tr.produce(input, output);
    // store output
    iEvent.emplace(edPutToken_, std::move(output));
  }

}  // namespace l1GTT

DEFINE_FWK_MODULE(l1GTT::ProducerTR);
