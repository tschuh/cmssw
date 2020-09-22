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
#include <vector>
#include <deque>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerSFout
   *  \brief  transforms SF output into TTTracks
   *  \author Thomas Schuh
   *  \date   2020, July
   */
  class ProducerSFout : public stream::EDProducer<> {
  public:
    explicit ProducerSFout(const ParameterSet&);
    ~ProducerSFout() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // ED input token of sf stubs
    EDGetTokenT<TTDTC::Streams> edGetToken_;
    // ED output token of TTTracks
    EDPutTokenT<vector<TTTrack<Ref_Phase2TrackerDigi_>>> edPutToken_;
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

  ProducerSFout::ProducerSFout(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& label = iConfig.getParameter<string>("LabelSF");
    const string& branchAccepted = iConfig.getParameter<string>("BranchAccepted");
    // book in- and output ED products
    edGetToken_ = consumes<TTDTC::Streams>(InputTag(label, branchAccepted));
    edPutToken_ = produces<vector<TTTrack<Ref_Phase2TrackerDigi_>>>(branchAccepted);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
  }

  void ProducerSFout::beginRun(const Run& iRun, const EventSetup& iSetup) {
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

  void ProducerSFout::produce(Event& iEvent, const EventSetup& iSetup) {
    const DataFormat& dfCot = dataFormats_->format(Variable::cot, Process::sf);
    const DataFormat& dfZ0 = dataFormats_->format(Variable::z0, Process::sf);
    const DataFormat& dfPhiT = dataFormats_->format(Variable::phiT, Process::sf);
    const DataFormat& dfQoverPt = dataFormats_->format(Variable::qOverPt, Process::sf);
    // empty SFout product
    deque<TTTrack<Ref_Phase2TrackerDigi_>> ttTracks;
    // read in SF Product and produce SFout product
    if (setup_->configurationSupported()) {
      Handle<TTDTC::Streams> handle;
      iEvent.getByToken<TTDTC::Streams>(edGetToken_, handle);
      const TTDTC::Streams& streams = *handle.product();
      for (int region = 0; region < setup_->numRegions(); region++ ) {
        for (int channel = 0; channel < dataFormats_->numChannel(Process::sf); channel++) {
          const int index = region * dataFormats_->numChannel(Process::sf) + channel;
          // convert stream to stubs
          const TTDTC::Stream& stream = streams[index];
          vector<StubSF> stubs;
          stubs.reserve(stream.size());
          for (const TTDTC::Frame& frame : stream)
            if(frame.first.isNonnull())
              stubs.emplace_back(frame, dataFormats_);
          // form tracks
          int i(0);
          for (auto it = stubs.begin(); it != stubs.end();) {
            const auto start = it;
            const int id = it->trackId();
            auto different = [id](const StubSF& stub){ return id != stub.trackId(); };
            it = find_if(it, stubs.end(), different);
            vector<TTStubRef> ttStubRefs;
            ttStubRefs.reserve(distance(start, it));
            transform(start, it, back_inserter(ttStubRefs), [](const StubSF& stub){ return stub.ttStubRef(); });
            const double z0 = dfZ0.floating(start->z0());
            const double cot = dfCot.floating(start->cot());
            const double phiT = dfPhiT.floating(start->phiT());
            const double qOverPt = dfQoverPt.floating(start->qOverPt());
            const int trackId = channel * setup_->sfMaxTracks() + i;
            ttTracks.emplace_back(qOverPt, phiT, cot, z0, 0., 0., 0., 0., 0., trackId, 0, 0.);
            ttTracks.back().setStubRefs(ttStubRefs);
            ttTracks.back().setPhiSector(start->sectorPhi() + region * setup_->numSectorsPhi());
            ttTracks.back().setEtaSector(start->sectorEta());
            ttTracks.back().setTrackSeedType(start->trackId());
            if (i++ == setup_->sfMaxTracks())
              break;
          }
        }
      }
    }
    // store product
    iEvent.emplace(edPutToken_, ttTracks.begin(), ttTracks.end());
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerSFout);