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

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"
#include "L1Trigger/TrackerTFP/interface/LinearRegression.h"

#include <string>
#include <numeric>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerLR
   *  \brief  L1TrackTrigger Linear Regression emulator
   *  \author Thomas Schuh
   *  \date   2020, June
   */
  class ProducerLR : public stream::EDProducer<> {
  public:
    explicit ProducerLR(const ParameterSet&);
    ~ProducerLR() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // ED input token of gp stubs
    EDGetTokenT<TTDTC::Streams> edGetToken_;
    // ED output token for stub information
    EDPutTokenT<TTDTC::Streams> edPutTokenStubs_;
    // ED output token for track information
    EDPutTokenT<TTDTC::Streams> edPutTokenTracks_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // DataFormats token
    ESGetToken<DataFormats, DataFormatsRcd> esGetTokenDataFormats_;
    // configuration
    ParameterSet iConfig_;
    const Setup* setup_;
    // helper class to extract structured data from TTDTC::Frames
    const DataFormats* dataFormats_;
  };

  ProducerLR::ProducerLR(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& label = iConfig.getParameter<string>("LabelMHT");
    const string& branchStubs = iConfig.getParameter<string>("BranchAccepted");
    const string& branchTracks = iConfig.getParameter<string>("BranchTracks");
    // book in- and output ED products
    edGetToken_ = consumes<TTDTC::Streams>(InputTag(label, branchStubs));
    edPutTokenStubs_ = produces<TTDTC::Streams>(branchStubs);
    edPutTokenTracks_ = produces<TTDTC::Streams>(branchTracks);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
  }

  void ProducerLR::beginRun(const Run& iRun, const EventSetup& iSetup) {
    // helper class to store configurations
    setup_ = &iSetup.getData(esGetTokenSetup_);
    if (!setup_->configurationSupported())
      return;
    // check process history if desired
    if (iConfig_.getParameter<bool>("CheckHistory"))
      setup_->checkHistory(iRun.processHistory());
    // helper class to extract structured data from TTDTC::Frames
    dataFormats_ = &iSetup.getData(esGetTokenDataFormats_);
  }

  void ProducerLR::produce(Event& iEvent, const EventSetup& iSetup) {
    // empty lr products
    TTDTC::Streams stubs(dataFormats_->numStreams(Process::mht));
    TTDTC::Streams tracks(dataFormats_->numStreams(Process::mht));
    // read in MHT Product and produce LR product
    if (setup_->configurationSupported()) {
      Handle<TTDTC::Streams> handle;
      iEvent.getByToken<TTDTC::Streams>(edGetToken_, handle);
      const TTDTC::Streams& streams = *handle.product();
      for (int region = 0; region < setup_->numRegions(); region++) {
        // object to fit in a region tracks
        LinearRegression lr(iConfig_, setup_, dataFormats_, region);
        // read in and organize input product
        lr.consume(streams);
        // fill output products
        lr.produce(stubs, tracks);
      }
    }
    // store products
    iEvent.emplace(edPutTokenStubs_, move(stubs));
    iEvent.emplace(edPutTokenTracks_, move(tracks));
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerLR);