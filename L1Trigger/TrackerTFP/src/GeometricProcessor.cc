#include "L1Trigger/TrackerTFP/interface/GeometricProcessor.h"

#include <numeric>
#include <algorithm>
#include <iterator>
#include <deque>
#include <vector>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  GeometricProcessor::GeometricProcessor(const ParameterSet& iConfig, const Setup* setup, const DataFormats* dataFormats, int region) :
    enableTruncation_(iConfig.getParameter<bool>("EnableTruncation")),
    setup_(setup),
    dataFormats_(dataFormats),
    region_(region),
    input_(dataFormats_->numChannel(Process::gp), vector<deque<StubPP*>>(dataFormats_->numChannel(Process::pp))) {}

  void GeometricProcessor::consume(const TTDTC& ttDTC) {
    auto validFrame = [](int& sum, const TTDTC::Frame& frame){ return sum += frame.first.isNonnull() ? 1 : 0; };
    int nStubsPP(0);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::pp); channel++) {
      const TTDTC::Stream& stream = ttDTC.stream(region_, channel);
      nStubsPP += accumulate(stream.begin(), stream.end(), 0, validFrame);
    }
    stubsPP_.reserve(nStubsPP);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::pp); channel++) {
      for (const TTDTC::Frame& frame : ttDTC.stream(region_, channel)) {
        StubPP* stub = nullptr;
        if (frame.first.isNonnull()) {
          stubsPP_.emplace_back(frame, dataFormats_);
          stub = &stubsPP_.back();
        }
        for (int sector = 0; sector < dataFormats_->numChannel(Process::gp); sector++)
          input_[sector][channel].push_back(stub && stub->inSector(sector) ? stub : nullptr);
      }
    }
    // remove all gaps between end and last stub
    for(vector<deque<StubPP*>>& input : input_)
      for(deque<StubPP*>& stubs : input)
        for(auto it = stubs.end(); it != stubs.begin();)
          it = (*--it) ? stubs.begin() : stubs.erase(it);
    auto validStub = [](int& sum, StubPP* stub){ return sum += stub ? 1 : 0; };
    int nStubsGP(0);
    for (const vector<deque<StubPP*>>& sector : input_)
      for (const deque<StubPP*>& channel : sector)
        nStubsGP += accumulate(channel.begin(), channel.end(), 0, validStub);
    stubsGP_.reserve(nStubsGP);
  }

  void GeometricProcessor::produce(TTDTC::Streams& accepted, TTDTC::Streams& lost) {
    for (int sector = 0; sector < dataFormats_->numChannel(Process::gp); sector++) {
      vector<deque<StubPP*>>& inputs = input_[sector];
      vector<deque<StubGP*>> stacks(dataFormats_->numChannel(Process::pp));
      const int sectorPhi = sector % setup_->numSectorsPhi();
      const int sectorEta = sector / setup_->numSectorsPhi();
      auto size =  [](int& sum, const deque<StubPP*>& stubs){ return sum += stubs.size(); };
      const int nStubs = accumulate(inputs.begin(), inputs.end(), 0, size);
      vector<StubGP*> acceptedSector;
      vector<StubGP*> lostSector;
      acceptedSector.reserve(nStubs);
      lostSector.reserve(nStubs);
      // clock accurate firmware emulation, each while trip describes one clock tick, one stub in and one stub out per tick
      while(!all_of(inputs.begin(), inputs.end(), [](const deque<StubPP*>& stubs){ return stubs.empty(); }) or
            !all_of(stacks.begin(), stacks.end(), [](const deque<StubGP*>& stubs){ return stubs.empty(); })) {
        // fill input fifo
        for (int channel = 0; channel < dataFormats_->numChannel(Process::pp); channel++) {
          deque<StubGP*>& stack = stacks[channel];
          StubPP* stub = pop_front(inputs[channel]);
          if (stub) {
            stubsGP_.emplace_back(*stub, sectorPhi, sectorEta);
            if (enableTruncation_ && (int)stack.size() == setup_->gpDepthMemory() - 1)
              lostSector.push_back(pop_front(stack));
            stack.push_back(&stubsGP_.back());
          }
        }
        // merge input fifos to one stream
        bool nothingToRoute(true);
        for (int channel = dataFormats_->numChannel(Process::pp) - 1; channel >= 0; channel--) {
          StubGP* stub = pop_front(stacks[channel]);
          if (stub) {
            nothingToRoute = false;
            acceptedSector.push_back(stub);
            break;
          }
        }
        if (nothingToRoute)
          acceptedSector.push_back(nullptr);
      }
      // truncate if desired
      if (enableTruncation_ && (int)acceptedSector.size() > setup_->numFrames()) {
        const auto limit = next(acceptedSector.begin(), setup_->numFrames());
        copy_if(limit, acceptedSector.end(), back_inserter(lostSector), [](const StubGP* stub){ return stub; });
        acceptedSector.erase(limit, acceptedSector.end());
      }
      // remove all gaps between end and last stub
      for(auto it = acceptedSector.end(); it != acceptedSector.begin();)
        it = (*--it) ? acceptedSector.begin() : acceptedSector.erase(it);
      // fill products
      auto put = [](const vector<StubGP*>& stubs, TTDTC::Stream& stream) {
        //auto toFrame = [](StubGP* stub){ return stub ? stub->frame() : TTDTC::Frame(); };
        stream.reserve(stubs.size());
        for (StubGP* stub : stubs)
          if (stub)
            stream.emplace_back(stub->frame());
        //transform(stubs.begin(), stubs.end(), back_inserter(stream), toFrame);
      };
      const int index = region_ * dataFormats_->numChannel(Process::gp) + sector;
      put(acceptedSector, accepted[index]);
      put(lostSector, lost[index]);
    }
  }

  // remove and return first element of deque, returns nullptr if empty
  template<class T>
  T* GeometricProcessor::pop_front(deque<T*>& ts) const {
    T* t = nullptr;
    if (!ts.empty()) {
      t = ts.front();
      ts.pop_front();
    }
    return t;
  }

}