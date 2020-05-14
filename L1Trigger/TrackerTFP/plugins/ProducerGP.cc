#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/Handle.h"

#include "DataFormats/L1TrackTrigger/interface/TTDTC.h"
#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/GeometricProcessor.h"

#include <numeric>
#include <string>

using namespace std;
using namespace edm;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerGP
   *  \brief  L1TrackTrigger Geometric Processor emulator
   *  \author Thomas Schuh
   *  \date   2020, March
   */
  class ProducerGP : public stream::EDProducer<> {
  public:
    explicit ProducerGP(const ParameterSet&);
    ~ProducerGP() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // helper classe to store configurations
    Setup setup_;
    // ED input token of DTC stubs
    EDGetTokenT<TTDTC> edGetToken_;
    // ED output token for accepted stubs
    EDPutTokenT<TTDTC::Streams> edPutTokenAccepted_;
    // ED output token for lost stubs
    EDPutTokenT<TTDTC::Streams> edPutTokenLost_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetToken_;
    // configuration
    ParameterSet iConfig_;
  };

  ProducerGP::ProducerGP(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& labelDTC = iConfig.getParameter<string>("trackerDTCProducer");
    const string& labelGP = iConfig.getParameter<string>("trackerFTPProducerGP");
    const string& branchAccepted = iConfig.getParameter<string>("StubAccepted");
    const string& branchLost = iConfig.getParameter<string>("StubLost");
    // book in- and output ED products
    edGetToken_ = consumes<TTDTC>(InputTag(labelDTC, branchAccepted));
    edPutTokenAccepted_ = produces<TTDTC::Streams>(InputTag(labelGP, branchAccepted));
    edPutTokenLost_ = produces<TTDTC::Streams>(InputTag(labelGP, branchLost));
    // book ES product
    esGetToken_ = esConsumes<trackerDTC::Setup, trackerDTC::SetupRcd, Transition::BeginRun>();
  }

  void ProducerGP::beginRun(const Run& iRun, const EventSetup& iSetup) {
    setup_ = iSetup.getData(esGetToken_);
    if (!setup_.configurationSupported())
      return;
    // check process history if desired
    if (iConfig.getParameter<string>("CheckHistory"))
      setup_.checkHistory(iRun.processHistory());
  }

  void ProducerGP::produce(Event& iEvent, const EventSetup& iSetup) {
    auto isNonnull = [](int& sum, const TTDTC::Frame& frame){ return sum += frame.first.isNonnull() ? 1 : 0; };
    // empty GP products
    TTDTC::Streams accepted(setup_.dtcNumStreams());
    TTDTC::Streams lost(setup_.dtcNumStreams());
    // read in DTC Product and produce TFP product
    Handle<TTDTC> handleTTDTC;
    iEvent.getByToken<TTDTC>(getTokenTTDTC_, handleTTDTC);
    for (int region = 0; region < setup_.numRegions(); region++) {
      int nStubs(0);
      for (int dtc = 0; dtc < setup_.numDTCs(); dtc++) {
        const TTDTC::Stream& stream = handleTTDTC->stream(region, dtc);
        nStubs += accumulate(stream.begin(), stream.end(), 0, isNonnull);
      }
      GeometricProcessor gp(iConfig, setup_, region, nStubs);
      for (int dtc = 0; dtc < settings_.numDTCs(); dtc++)
        gp.consume(handleTTDTC->stream(region,dtc), dtc);
      gp.produce(accepted, lost);
    }
    // store products
    iEvent.emplace(edPutTokenAccepted_, move(accepted));
    iEvent.emplace(edPutTokenLost_, move(lost));
  }

}

DEFINE_FWK_MODULE(trackerTFP::ProducerGP);