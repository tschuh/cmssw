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
#include "L1Trigger/TrackerTFP/interface/MiniHoughTransform.h"

#include <string>
#include <numeric>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerMHT
   *  \brief  L1TrackTrigger Mini Hough Transform emulator
   *  \author Thomas Schuh
   *  \date   2020, May
   */
  class ProducerMHT : public stream::EDProducer<> {
  public:
    explicit ProducerMHT(const ParameterSet&);
    ~ProducerMHT() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // ED input token of gp stubs
    EDGetTokenT<TTDTC::Streams> edGetToken_;
    // ED output token for accepted stubs
    EDPutTokenT<TTDTC::Streams> edPutTokenAccepted_;
    // ED output token for lost stubs
    EDPutTokenT<TTDTC::Streams> edPutTokenLost_;
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

  ProducerMHT::ProducerMHT(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& label = iConfig.getParameter<string>("LabelHT");
    const string& branchAccepted = iConfig.getParameter<string>("BranchAccepted");
    const string& branchLost = iConfig.getParameter<string>("BranchLost");
    // book in- and output ED products
    edGetToken_ = consumes<TTDTC::Streams>(InputTag(label, branchAccepted));
    edPutTokenAccepted_ = produces<TTDTC::Streams>(branchAccepted);
    edPutTokenLost_ = produces<TTDTC::Streams>(branchLost);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
  }

  void ProducerMHT::beginRun(const Run& iRun, const EventSetup& iSetup) {
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

  void ProducerMHT::produce(Event& iEvent, const EventSetup& iSetup) {
    // empty MHT products
    TTDTC::Streams accepted(dataFormats_->numStreams(Process::mht));
    TTDTC::Streams lost(dataFormats_->numStreams(Process::mht));
    // read in HT Product and produce MHT product
    if (setup_->configurationSupported()) {
      Handle<TTDTC::Streams> handle;
      iEvent.getByToken<TTDTC::Streams>(edGetToken_, handle);
      const TTDTC::Streams& streams = *handle.product();
      for (int region = 0; region < setup_->numRegions(); region++) {
        // object to find in a region finer rough candidates in r-phi
        MiniHoughTransform mht(iConfig_, setup_, dataFormats_, region);
        // read in and organize input product
        mht.consume(streams);
        // fill output products
        mht.produce(accepted, lost);
      }
    }
    // store products
    iEvent.emplace(edPutTokenAccepted_, move(accepted));
    iEvent.emplace(edPutTokenLost_, move(lost));
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerMHT);