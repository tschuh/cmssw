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

#include <string>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerKFTTTacks
   *  \brief  Converts KF output into TTTracks
   *  \author Thomas Schuh
   *  \date   2020, Oct
   */
  class ProducerKFTTTacks : public stream::EDProducer<> {
  public:
    explicit ProducerKFTTTacks(const ParameterSet&);
    ~ProducerKFTTTacks() override {}

  private:
    void beginRun(const Run&, const EventSetup&) override;
    void produce(Event&, const EventSetup&) override;
    void endJob() {}

    // ED input token of kf tracks
    EDGetTokenT<StreamsTrack> edGetToken_;
    // ED output token for TTTracks
    EDPutTokenT<TTTracks> edPutToken_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // DataFormats token
    ESGetToken<DataFormats, DataFormatsRcd> esGetTokenDataFormats_;
    // configuration
    ParameterSet iConfig_;
    // helper class to store configurations
    const Setup* setup_;
    // helper class to extract structured data from TTDTC::Frames
    const DataFormats* dataFormats_;
  };

  ProducerKFTTTacks::ProducerKFTTTacks(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& label = iConfig.getParameter<string>("LabelKF");
    const string& branch = iConfig.getParameter<string>("BranchAccepted");
    // book in- and output ED products
    edGetToken_ = consumes<StreamsTrack>(InputTag(label, branch));
    edPutToken_ = produces<TTTracks>(branch);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
  }

  void ProducerKFTTTacks::beginRun(const Run& iRun, const EventSetup& iSetup) {
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

  void ProducerKFTTTacks::produce(Event& iEvent, const EventSetup& iSetup) {
    /*// empty KFTTTrack product
    TTTracks ttTracks;
    // read in KF Product and produce KFTTTrack product
    if (setup_->configurationSupported()) {
      Handle<StreamsTrack> handle;
      iEvent.getByToken<StreamsTrack>(edGetToken_, handle);
      const StreamsTrack streams = handle.product();
      int nTracks(0);
      for (const StreamTrack& stream : streams)
        nTracks += accumulate(stream.begin(), stream.end(), 0, [](int& sum, const FrameTrack& frame){ return sum += frame.first.isNonnull() ? 1 : 0; });
      ttTracks.reserve(nTracks);
      for (const StreamTrack& stream : streams) {
        for (const FrameTrack& frame : stream) {
          if (frame.first.isNull())
            continue;
          TrackKF track(frame, dataFormats_);
          ttTracks.emplace_back(track.ttTrack());
        }
      }
    }
    // store products
    iEvent.emplace(edPutToken_, move(ttTracks));*/
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerKFTTTacks);