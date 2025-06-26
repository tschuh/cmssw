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
#include "L1Trigger/L1TTrackMatch/interface/TrackFormatter.h"

#include <string>
#include <utility>

namespace l1GTT {

  /*! \class  l1GTT::ProducerTF
   *  \brief  Emulates GTT Track Formatter. This step transforms 18 = 2 (+- eta) x 9 (phi nonant) channels
              transporting 2 tracks over 3 clock ticks into 12 = 2 (+- eta) x 3 (3 combinened phi nonants) channel
              transporting one track per clock tick
   *  \author Thomas Schuh
   *  \date   2025, June
   */
  class ProducerTF : public edm::stream::EDProducer<> {
  public:
    explicit ProducerTF(const edm::ParameterSet&);
    ~ProducerTF() override = default;

  private:
    void produce(edm::Event&, const edm::EventSetup&) override;
    // ED input token of tfp tracks
    edm::EDGetTokenT<tt::StreamsTrack> edGetToken_;
    // ED output token for tracks
    edm::EDPutTokenT<tt::StreamsTT> edPutToken_;
    // Setup token
    edm::ESGetToken<Setup, tt::SetupRcd> esGetToken_;
  };

  ProducerTF::ProducerTF(const edm::ParameterSet& iConfig) {
    const std::string& label = iConfig.getParameter<std::string>("InputLabelTF");
    const std::string& branch = iConfig.getParameter<std::string>("Branch");
    // book in- and output ED products
    edGetToken_ = consumes<tt::StreamsTrack>(edm::InputTag(label, branch));
    edPutToken_ = produces<tt::StreamsTT>(branch);
    // book ES products
    esGetToken_ = esConsumes();
  }

  void ProducerTF::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    // helper class to store configurations
    const Setup* setup = &iSetup.getData(esGetToken_);
    // emulator
    TrackFormatter tf(setup);
    // empty TF product
    tt::StreamsTT output(setup->tfNumChannel());
    // read in TFP Product
    const tt::StreamsTrack& input = iEvent.get(edGetToken_);
    // produce output
    tf.produce(input, output);
    // store output
    iEvent.emplace(edPutToken_, std::move(output));
  }

}  // namespace l1GTT

DEFINE_FWK_MODULE(l1GTT::ProducerTF);
