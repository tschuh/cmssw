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
#include "L1Trigger/TrackerTFP/interface/LayerEncoding.h"

#include <string>
#include <vector>
#include <deque>
#include <iterator>
#include <cmath>
#include <numeric>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::ProducerKFin
   *  \brief  transforms TTTracks into KF input
   *  \author Thomas Schuh
   *  \date   2020, July
   */
  class ProducerKFin : public stream::EDProducer<> {
  public:
    explicit ProducerKFin(const ParameterSet&);
    ~ProducerKFin() override {}

  private:
    virtual void beginRun(const Run&, const EventSetup&) override;
    virtual void produce(Event&, const EventSetup&) override;
    virtual void endJob() {}

    // ED input token of TTTracks
    EDGetTokenT<vector<TTTrack<Ref_Phase2TrackerDigi_>>> edGetTokenTTTracks_;
    // ED input token of Stubs
    EDGetTokenT<TTDTC::Streams> edGetTokenStubs_;
    // ED output token for stubs
    EDPutTokenT<TTDTC::Streams> edPutTokenAcceptedStubs_;
    EDPutTokenT<TTDTC::Streams> edPutTokenLostStubs_;
    // ED output token for tracks
    EDPutTokenT<StreamsTrack> edPutTokenAcceptedTracks_;
    EDPutTokenT<StreamsTrack> edPutTokenLostTracks_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // DataFormats token
    ESGetToken<DataFormats, DataFormatsRcd> esGetTokenDataFormats_;
    // LayerEncoding token
    ESGetToken<LayerEncoding, LayerEncodingRcd> esGetTokenLayerEncoding_;
    // configuration
    ParameterSet iConfig_;
    // helper class to store configurations
    const Setup* setup_;
    // helper class to extract structured data from TTDTC::Frames
    const DataFormats* dataFormats_;
    // helper class to encode layer
    const LayerEncoding* layerEncoding_;
    //
    bool enableTruncation_;
  };

  ProducerKFin::ProducerKFin(const ParameterSet& iConfig) :
    iConfig_(iConfig)
  {
    const string& labelTTTracks = iConfig.getParameter<string>("LabelSFout");
    const string& labelStubs = iConfig.getParameter<string>("LabelSF");
    const string& branchAccepted = iConfig.getParameter<string>("BranchAccepted");
    const string& branchLost = iConfig.getParameter<string>("BranchLost");
    // book in- and output ED products
    edGetTokenTTTracks_ = consumes<vector<TTTrack<Ref_Phase2TrackerDigi_>>>(InputTag(labelTTTracks, branchAccepted));
    edGetTokenStubs_ = consumes<TTDTC::Streams>(InputTag(labelStubs, branchAccepted));
    edPutTokenAcceptedStubs_ = produces<TTDTC::Streams>(branchAccepted);
    edPutTokenAcceptedTracks_ = produces<StreamsTrack>(branchAccepted);
    edPutTokenLostStubs_ = produces<TTDTC::Streams>(branchLost);
    edPutTokenLostTracks_ = produces<StreamsTrack>(branchLost);
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    esGetTokenLayerEncoding_ = esConsumes<LayerEncoding, LayerEncodingRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
    layerEncoding_ = nullptr;
    //
    enableTruncation_ = iConfig.getParameter<bool>("EnableTruncation");
  }

  void ProducerKFin::beginRun(const Run& iRun, const EventSetup& iSetup) {
    // helper class to store configurations
    setup_ = &iSetup.getData(esGetTokenSetup_);
    if (!setup_->configurationSupported())
      return;
    // check process history if desired
    if (iConfig_.getParameter<bool>("CheckHistory"))
      setup_->checkHistory(iRun.processHistory());
    // helper class to extract structured data from TTDTC::Frames
    dataFormats_ = &iSetup.getData(esGetTokenDataFormats_);
    // helper class to encode layer
    layerEncoding_ = &iSetup.getData(esGetTokenLayerEncoding_);
  }

  void ProducerKFin::produce(Event& iEvent, const EventSetup& iSetup) {
    const DataFormat& dfCot = dataFormats_->format(Variable::cot, Process::sf);
    const DataFormat& dfZ0 = dataFormats_->format(Variable::z0, Process::sf);
    // empty KFin products
    TTDTC::Streams streamAcceptedStubs(dataFormats_->numStreams(Process::kf) * setup_->numLayers());
    StreamsTrack streamAcceptedTracks(dataFormats_->numStreams(Process::kf));
    TTDTC::Streams streamLostStubs(dataFormats_->numStreams(Process::kf) * setup_->numLayers());
    StreamsTrack streamLostTracks(dataFormats_->numStreams(Process::kf));
    // read in SFout Product and produce KFin product
    if (setup_->configurationSupported()) {
      Handle<TTDTC::Streams> handleStubs;
      iEvent.getByToken<TTDTC::Streams>(edGetTokenStubs_, handleStubs);
      const TTDTC::Streams& streams = *handleStubs.product();
      Handle<vector<TTTrack<Ref_Phase2TrackerDigi_>>> handleTTTracks;
      iEvent.getByToken<vector<TTTrack<Ref_Phase2TrackerDigi_>>>(edGetTokenTTTracks_, handleTTTracks);
      const vector<TTTrack<Ref_Phase2TrackerDigi_>>& ttTracks = *handleTTTracks.product();
      for (int region = 0; region < setup_->numRegions(); region++) {
        int nStubsSF(0);
        for (int channel = 0; channel < dataFormats_->numChannel(Process::sf); channel++) {
          const int index = region * dataFormats_->numChannel(Process::sf) + channel;
          const TTDTC::Stream& stream = streams[index];
          nStubsSF += accumulate(stream.begin(), stream.end(), 0, [](int& sum, const TTDTC::Frame& frame){ return sum += frame.first.isNonnull() ? 1 : 0; });
        }
        vector<StubSF> stubsSF;
        stubsSF.reserve(nStubsSF);
        for (int channel = 0; channel < dataFormats_->numChannel(Process::sf); channel++) {
          const int index = region * dataFormats_->numChannel(Process::sf) + channel;
          for (const TTDTC::Frame& frame : streams[index])
            if (frame.first.isNonnull())
              stubsSF.emplace_back(frame, dataFormats_);
        }
        vector<deque<TTDTC::Frame>> dequesStubs(dataFormats_->numChannel(Process::kf) * setup_->numLayers());
        vector<deque<FrameTrack>> dequesTracks(dataFormats_->numChannel(Process::kf));
        int i(0);
        for (const TTTrack<Ref_Phase2TrackerDigi_>& ttTrack : ttTracks) {
          if ((int)ttTrack.phiSector() / setup_->numSectorsPhi() != region) {
            i++;
            continue;
          }
          const int sectorPhi = ttTrack.phiSector() % setup_->numSectorsPhi();
          deque<FrameTrack>& tracks = dequesTracks[sectorPhi];
          const int binEta = ttTrack.etaSector();
          const int binZ0 = dfZ0.toUnsigned(dfZ0.integer(ttTrack.z0()));
          const int binCot = dfCot.toUnsigned(dfCot.integer(ttTrack.tanL()));
          StubSF* stubSF = nullptr;
          TTBV hitPattern(0, setup_->numLayers());
          vector<int> layerCounts(setup_->numLayers(), 0);
          for (const TTStubRef& ttStubRef : ttTrack.getStubRefs()) {
            const int layerId = setup_->layerId(ttStubRef);
            const int layerIdKF = layerEncoding_->layerIdKF(binEta, binZ0, binCot, layerId);
            hitPattern.set(layerIdKF);
            layerCounts[layerIdKF]++;
            deque<TTDTC::Frame>& stubs = dequesStubs[sectorPhi * setup_->numLayers() + layerIdKF];
            auto identical = [ttStubRef, ttTrack](const StubSF& stub){
              return (int)ttTrack.trackSeedType() == stub.trackId() && ttStubRef == stub.ttStubRef();
            };
            stubSF = &*find_if(stubsSF.begin(), stubsSF.end(), identical);
            const StubKFin stubKFin(*stubSF, layerIdKF, ttTrack.hitPattern());
            stubs.emplace_back(stubKFin.frame());
          }
          const TTBV& layerMap = setup_->layerMap(layerCounts);
          TrackKFin track(*stubSF, TTTrackRef(handleTTTracks, i++), hitPattern, layerMap);
          tracks.emplace_back(track.frame());
        }
        // transform deques to vectors
        for (int channel = 0; channel < dataFormats_->numChannel(Process::kf); channel++) {
          const int index = region * dataFormats_->numChannel(Process::kf) + channel;
          deque<FrameTrack>& tracks = dequesTracks[channel];
          const auto limitTracks = next(tracks.begin(), min((int)tracks.size(), enableTruncation_ ? setup_->numFrames() : 0));
          streamAcceptedTracks[index] = StreamTrack(tracks.begin(), limitTracks);
          streamLostTracks[index] = StreamTrack(limitTracks, tracks.end());
          for (int l = 0; l < setup_->numLayers(); l++) {
            deque<TTDTC::Frame>& stubs = dequesStubs[channel * setup_->numLayers() + l];
            const auto limitStubs = next(stubs.begin(), min((int)stubs.size(), enableTruncation_ ? setup_->numFrames() : 0));
            streamAcceptedStubs[index * setup_->numLayers() + l] = TTDTC::Stream(stubs.begin(), limitStubs);
            streamLostStubs[index * setup_->numLayers() + l] = TTDTC::Stream(limitStubs, stubs.end());
          }
        }
      }
    }
    // store products
    iEvent.emplace(edPutTokenAcceptedStubs_, move(streamAcceptedStubs));
    iEvent.emplace(edPutTokenAcceptedTracks_, move(streamAcceptedTracks));
    iEvent.emplace(edPutTokenLostStubs_, move(streamLostStubs));
    iEvent.emplace(edPutTokenLostTracks_, move(streamLostTracks));
  }

} // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::ProducerKFin);