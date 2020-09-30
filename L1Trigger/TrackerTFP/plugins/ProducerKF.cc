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
#include "L1Trigger/TrackerTFP/interface/KalmanFilterFormats.h"
#include "L1Trigger/TrackerTFP/interface/LayerEncoding.h"
#include "L1Trigger/TrackerTFP/interface/KalmanFilter.h"

#include <string>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerKF
   *  \brief  L1TrackTrigger Kamlan Filter emulator
   *  \author Thomas Schuh
   *  \date   2020, July
   */
  class ProducerKF : public stream::EDProducer<> {
  public:
    explicit ProducerKF(const ParameterSet&);
    ~ProducerKF() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // ED input token of sf stubs and tracks
    EDGetTokenT<TTDTC::Streams> edGetTokenStubs_;
    EDGetTokenT<StreamsTrack> edGetTokenTracks_;
    // ED output token for accepted tracks
    EDPutTokenT<StreamsTrack> edPutTokenAccepted_;
    // ED output token for lost tracks
    EDPutTokenT<StreamsTrack> edPutTokenLost_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // DataFormats token
    ESGetToken<DataFormats, DataFormatsRcd> esGetTokenDataFormats_;
    // KalmanFilterFormats token
    ESGetToken<KalmanFilterFormats, KalmanFilterFormatsRcd> esGetTokenKalmanFilterFormats_;
    // configuration
    ParameterSet iConfig_;
    // helper class to store configurations
    const Setup* setup_;
    // helper class to extract structured data from TTDTC::Frames
    const DataFormats* dataFormats_;
    // helper class to
    const KalmanFilterFormats* kalmanFilterFormats_;
  };

  ProducerKF::ProducerKF(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& label = iConfig.getParameter<string>("LabelKFin");
    const string& branchAccepted = iConfig.getParameter<string>("BranchAccepted");
    const string& branchLost = iConfig.getParameter<string>("BranchLost");
    // book in- and output ED products
    edGetTokenStubs_ = consumes<TTDTC::Streams>(InputTag(label, branchAccepted));
    edGetTokenTracks_ = consumes<StreamsTrack>(InputTag(label, branchAccepted));
    edPutTokenAccepted_ = produces<StreamsTrack>(branchAccepted);
    edPutTokenLost_ = produces<StreamsTrack>(branchLost);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    esGetTokenKalmanFilterFormats_ = esConsumes<KalmanFilterFormats, KalmanFilterFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
    kalmanFilterFormats_ = nullptr;
  }

  void ProducerKF::beginRun(const Run& iRun, const EventSetup& iSetup) {
    // helper class to store configurations
    setup_ = &iSetup.getData(esGetTokenSetup_);
    if (!setup_->configurationSupported())
      return;
    // check process history if desired
    if (iConfig_.getParameter<bool>("CheckHistory"))
      setup_->checkHistory(iRun.processHistory());
    // helper class to extract structured data from TTDTC::Frames
    dataFormats_ = &iSetup.getData(esGetTokenDataFormats_);
    // helper class to
    kalmanFilterFormats_ = &iSetup.getData(esGetTokenKalmanFilterFormats_);
  }

  void ProducerKF::produce(Event& iEvent, const EventSetup& iSetup) {
    // empty KF products
    StreamsTrack accepted(setup_->numRegions());
    StreamsTrack lost(setup_->numRegions());
    // read in SF Product and produce KF product
    if (setup_->configurationSupported()) {
      Handle<TTDTC::Streams> handleStubs;
      iEvent.getByToken<TTDTC::Streams>(edGetTokenStubs_, handleStubs);
      const TTDTC::Streams& stubs = *handleStubs.product();
      Handle<StreamsTrack> handleTracks;
      iEvent.getByToken<StreamsTrack>(edGetTokenTracks_, handleTracks);
      const StreamsTrack& tracks = *handleTracks.product();
      for (int region = 0; region < setup_->numRegions(); region++) {
        // object to find in a region finer rough candidates in r-z
        KalmanFilter kf(iConfig_, setup_, dataFormats_, kalmanFilterFormats_, region);
        // read in and organize input stubs
        kf.consume(stubs);
        // read in and organize input tracks
        kf.consume(tracks);
        // fill output products
        kf.produce(accepted[region], lost[region]);
      }
    }
    // store products
    iEvent.emplace(edPutTokenAccepted_, move(accepted));
    iEvent.emplace(edPutTokenLost_, move(lost));
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerKF);