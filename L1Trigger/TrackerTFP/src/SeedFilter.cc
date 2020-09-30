#include "L1Trigger/TrackerTFP/interface/SeedFilter.h"

#include <numeric>
#include <algorithm>
#include <iterator>
#include <deque>
#include <vector>
#include <set>
#include <utility>
#include <cmath>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  SeedFilter::SeedFilter(const ParameterSet& iConfig, const Setup* setup, const DataFormats* dataFormats, int region) :
    enableTruncation_(iConfig.getParameter<bool>("EnableTruncation")),
    setup_(setup),
    dataFormats_(dataFormats),
    region_(region),
    cot_(dataFormats_->format(Variable::cot, Process::sf)),
    z0_(dataFormats_->format(Variable::z0, Process::sf)),
    z_(dataFormats_->format(Variable::z, Process::sf)),
    input_(dataFormats_->numChannel(Process::mht)) {}

  void SeedFilter::consume(const TTDTC::Streams& streams) {
    auto valid = [](int& sum, const TTDTC::Frame& frame){ return sum += (frame.first.isNonnull() ? 1 : 0); };
    int nStubs(0);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      const TTDTC::Stream& stream = streams[region_ * dataFormats_->numChannel(Process::mht) + channel];
      nStubs += accumulate(stream.begin(), stream.end(), 0, valid);
    }
    stubsMHT_.reserve(nStubs);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      const TTDTC::Stream& stream = streams[region_ * dataFormats_->numChannel(Process::mht) + channel];
      vector<StubMHT*>& stubs = input_[channel];
      stubs.reserve(stream.size());
      for (const TTDTC::Frame& frame : stream) {
        StubMHT* stub = nullptr;
        if (frame.first.isNonnull()) {
          stubsMHT_.emplace_back(frame, dataFormats_);
          stub = &stubsMHT_.back();
        }
        stubs.push_back(stub);
      }
    }
  }

  void SeedFilter::produce(TTDTC::Streams& accepted, TTDTC::Streams& lost) {
    auto smallerChi = [](const vector<StubSF*>& lhs, const vector<StubSF*>& rhs) {
      auto chi = [](const vector<StubSF*>& stubs) {
        double c(0.);
        for (StubSF* stub : stubs)
          c += abs(stub->z());
        return c;
      };
      return chi(lhs) < chi(rhs);
    };
    auto moreLayer = [](const vector<StubSF*>& lhs, const vector<StubSF*>& rhs) {
      auto nLayer = [](const vector<StubSF*>& stubs) {
        set<int> layer;
        for (StubSF* stub : stubs)
          layer.insert(stub->layer());
        return layer.size();
      };
      return nLayer(lhs) > nLayer(rhs);
    };
    const int numLayers = 16;
    static const vector<pair<int, int>> seedingLayers = {{1, 2}, {1, 3}, {2, 3}, {1, 11}, {2, 11}, {1, 12}, {2, 12}, {11, 12}};
    //const double dT = setup_->chosenRofPhi() - setup_->chosenRofZ();
    const double dT = setup_->chosenRofPhi();
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      TTDTC::Stream& acceptedStream = accepted[region_ * dataFormats_->numChannel(Process::mht) + channel];
      TTDTC::Stream& lostStream = lost[region_ * dataFormats_->numChannel(Process::mht) + channel];
      deque<StubSF*> output;
      vector<StubMHT*>& stubs = input_[channel];
      stubs.erase(remove(stubs.begin(), stubs.end(), nullptr), stubs.end());
      set<int> ids;
      for (StubMHT* stub : stubs)
        if (stub)
          ids.insert(stub->trackId());
      vector<vector<StubMHT*>> tracks(ids.size());
      int i(0);
      for (auto it = stubs.begin(); it != stubs.end();) {
        const auto start = it;
        const int id = (*it)->trackId();
        auto different = [id](StubMHT* stub){ return id != stub->trackId(); };
        it = find_if(it, stubs.end(), different);
        vector<StubMHT*>& track = tracks[i++];
        track.reserve(distance(start, it));
        copy(start, it, back_inserter(track));
      }
      for (const vector<StubMHT*>& track : tracks) {
        /*for (StubMHT* stub : track)
          cout << stub->r() + setup_->chosenRofPhi() << " " << stub->z() << " " << setup_->dZ(stub->ttStubRef()) << endl;*/
        const int eta = track.front()->sectorEta();
        const double dZT = (sinh(setup_->boundarieEta(eta + 1)) - sinh(setup_->boundarieEta(eta))) * setup_->chosenRofZ();
        /*cout << dCot << endl;
        cout << sinh(setup_->boundarieEta(eta + 1)) << " " << sinh(setup_->boundarieEta(eta)) << endl;*/
        vector<vector<StubMHT*>> layerStubs(numLayers);
        for (vector<StubMHT*>& layer : layerStubs)
          layer.reserve(track.size());
        for (StubMHT* stub : track)
          layerStubs[layerId(stub)].push_back(stub);
        deque<pair<StubMHT*, StubMHT*>> seeds;
        for (const pair<int, int>& seedingLayer : seedingLayers)
          for (StubMHT* inner : layerStubs[seedingLayer.first])
            for (StubMHT* outer : layerStubs[seedingLayer.second])
              if (outer->r() > inner->r())
                seeds.emplace_back(inner, outer);
        vector<vector<StubSF*>> seedTracks;
        seedTracks.reserve(seeds.size());
        for (const pair<StubMHT*, StubMHT*>& seed : seeds) {
          const double r1 = seed.first->r() + dT;
          const double r2 = seed.second->r() + dT;
          const double z1 = seed.first->z();
          const double z2 = seed.second->z();
          const double r = (r2 + r1) / 2.;
          const double z = (z2 + z1) / 2.;
          const double dr = (r2 - r1);
          const double dz = (z2 - z1);
          const double cot = cot_.digi(dz / dr);
          const double z0 = z0_.digi(z - cot * r);
          const double zT = z0 + cot * setup_->chosenRofZ();
          //cout << ", " << cot << " * x + " << z0;
          if (abs(z0) > setup_->beamWindowZ())
            continue;
          //if (abs(cot) > dCot / 2.)
          if (abs(zT) > dZT / 2.)
            continue;
          vector<StubSF*> stubsSF;
          stubsSF.reserve(track.size());
          for (StubMHT* stub : track) {
            const double sr = stub->r() + dT;
            const double sz = stub->z();
            const double chi = sz - (z0 + sr * cot);
            const double dZ = z0_.base() + cot_.base() * sr + setup_->dZ(stub->ttStubRef());
            //cout << chi << " " << dZ << endl;
            if (abs(chi) < dZ / 2.) {
              stubsSF_.emplace_back(*stub, z0_.integer(z0), cot_.integer(cot));
              stubsSF.push_back(&stubsSF_.back());
            }
          }
          set<int> ids;
          for (StubSF* stub : stubsSF)
            ids.insert(stub->layer());
          if ((int)ids.size() >= setup_->sfMinLayers())
            seedTracks.push_back(stubsSF);
        }
        /*cout << endl;
        cout << cot_.base() << " " << z0_.base() << endl;
        throw cms::Exception("...");*/
        if (seedTracks.empty())
          continue;
        stable_sort(seedTracks.begin(), seedTracks.end(), smallerChi);
        stable_sort(seedTracks.begin(), seedTracks.end(), moreLayer);
        const vector<StubSF*>& stubs = seedTracks.front();
        copy(stubs.begin(), stubs.end(), back_inserter(output));
      }
      if (enableTruncation_ && ((int)output.size() > setup_->numFrames())) {
        const auto limit = next(output.begin(), setup_->numFrames());
        lostStream.reserve(distance(limit, output.end()));
        transform(limit, output.end(), back_inserter(lostStream), [](StubSF* stub){ return stub->frame(); });
        output.erase(limit, output.end());
      }
      acceptedStream.reserve(output.size());
      transform(output.begin(), output.end(), back_inserter(acceptedStream), [](StubSF* stub){ return stub->frame(); });
    }
  }

  //
  int SeedFilter::layerId(StubMHT* stub) const {
    int layer = stub->layer();
    bool barrel = setup_->barrel(stub->ttStubRef());
    int lay = -1;
    if (layer == 0)
      lay = 1;
    else if (layer == 1)
      lay = 2;
    else if (layer == 2)
      lay = barrel ? 6 : 11;
    else if (layer == 3)
      lay = barrel ? 5 : 12;
    else if (layer == 4)
      lay = barrel ? 4 : 13;
    else if (layer == 5)
      lay = 14;
    else if (layer == 6)
      lay = barrel ? 3 : 15;
    return lay;
  }

  // remove and return first element of deque, returns nullptr if empty
  template<class T>
  T* SeedFilter::pop_front(deque<T*>& ts) const {
    T* t = nullptr;
    if (!ts.empty()) {
      t = ts.front();
      ts.pop_front();
    }
    return t;
  }

} // namespace trackerTFP