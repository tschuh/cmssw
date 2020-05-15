#include "L1Trigger/TrackerTFP/interface/GeometricProcessor.h"

#include <numeric>
#include <algorithm>
#include <iterator>
#include <deque>
#include <vector>

using namespace std;

namespace trackerTFP {

  GeometricProcessor::GeometricProcessor(const edm::ParameterSet& iConfig, const trackerDTC::Setup& setup, int region, int nStubs) :
    enableTruncation_(iConfig.getParameter<bool>("EnableTruncation")),
    setup_(&setup),
    region_(region),
    input_(setup.numDTCs()),
    lost_(setup.numSectors())
  {
    stubs_.reserve(nStubs);
  }

  void GeometricProcessor::consume(const TTDTC::Stream& stream, int channel) {
    // organize stubs
    vector<Stub*>& input = input_[channel];
    input.reserve(stream.size());
    for (const TTDTC::Frame& frame : stream) {
      Stub* stub = nullptr;
      if (frame.first.isNonnull()) {
        stubs_.emplace_back(frame, setup_);
        stub = &stubs_.back();
      }
      input.push_back(stub);
    }
    // truncate if desired
    if (!enableTruncation_ || (int)input.size() <= setup_->numFramesIO())
      return;
    const auto limit = next(input.begin(), setup_->numFramesIO());
    input.erase(remove(limit, input.end(), nullptr), input.end());
    for (auto it = limit; it != input.end(); it++)
      for (int sector = 0; sector < setup_->numSectors(); sector++)
        if ((*it)->inSector_[sector])
          lost_[sector].push_back(*it);
    input.erase(limit, input.end());
    for (auto it = input.end(); it != input.begin();)
      it = (*--it) ? input.begin() : input.erase(it);
  }

  void GeometricProcessor::produce(TTDTC::Streams& streamAccepted, TTDTC::Streams& streamLost) {
    for (int sector = 0; sector < setup_->numSectors(); sector++) {
      auto sectorMask = [sector](Stub* stub){ return stub && stub->inSector_[sector] ? stub : nullptr; };
      deque<Stub*> accepted;
      deque<Stub*> lost(lost_[sector]);
      // prepare input stubs
      vector<deque<Stub*>> stacks(setup_->numDTCs());
      vector<deque<Stub*>> inputs(setup_->numDTCs());
      for (int dtc = 0; dtc < setup_->numDTCs(); dtc++) {
        const vector<Stub*>& stubs = input_[dtc];
        transform(stubs.begin(), stubs.end(), back_inserter(inputs[dtc]), sectorMask);
      }
      // clock accurate firmware emulation, each while trip describes one clock tick, one stub in and one stub out per tick
      while(!all_of(inputs.begin(), inputs.end(), [](const deque<Stub*>& stubs){ return stubs.empty(); }) or
            !all_of(stacks.begin(), stacks.end(), [](const deque<Stub*>& stubs){ return stubs.empty(); })) {
        // fill input fifo
        for (int dtc = 0; dtc < setup_->numDTCs(); dtc++) {
          deque<Stub*>& stack = stacks[dtc];
          Stub* stub = pop_front(inputs[dtc]);
          if (stub) {
            if (enableTruncation_ && (int)stack.size() == setup_->gpDepthMemory() - 1)
              lost.push_back(pop_front(stack));
            stack.push_back(stub);
          }
        }
        // merge input fifos to one stream
        bool nothingToRoute(true);
        for (int dtc = setup_->numDTCs() - 1; dtc >= 0; dtc--) {
          Stub* stub = pop_front(stacks[dtc]);
          if (stub) {
            nothingToRoute = false;
            accepted.push_back(stub);
            break;
          }
        }
        if (nothingToRoute)
          accepted.push_back(nullptr);
      }
      // truncate if desired
      if (enableTruncation_ && (int)accepted.size() > setup_->numFrames()) {
        const auto limit = next(accepted.begin(), setup_->numFrames());
        copy_if(limit, accepted.end(), back_inserter(lost), [](const Stub* stub){ return stub; });
        accepted.erase(limit, accepted.end());
      }
      // remove all gaps between end and last stub
      for(auto it = accepted.end(); it != accepted.begin();)
        it = (*--it) ? accepted.begin() : accepted.erase(it);
      // fill products
      auto put = [this, sector](const deque<Stub*> stubs, TTDTC::Stream& stream) {
        auto toFrame = [this, sector](Stub* stub){ return stub ? stub->toFrame(sector, setup_) : TTDTC::Frame(); };
        stream.reserve(stubs.size());
        transform(stubs.begin(), stubs.end(), back_inserter(stream), toFrame);
      };
      const int index = region_ * setup_->numSectors() + sector;
      put(accepted, streamAccepted[index]);
      put(lost, streamLost[index]);
    }
  }

  GeometricProcessor::Stub::Stub(const TTDTC::Frame& frame, const trackerDTC::Setup* setup) :
    ttStubRef_(frame.first),
    ttBV_(frame.second),
    inSector_(0, setup->numSectors())
  {
    TTBV ttBV(ttBV_);
    ttBV >>= setup->widthLayer();
    const TTBV phis(ttBV.val(setup->numSectorsPhi(), setup->numSectorsPhi()));
    ttBV >>= setup->numSectorsPhi();
    const int etaMax = ttBV.val(setup->widthSectorEta());
    ttBV >>= setup->widthSectorEta();
    const int etaMin = ttBV.val(setup->widthSectorEta());
    for (int eta = etaMin; eta <= etaMax; eta++)
      for (int phi = 0; phi < setup->numSectorsPhi(); phi++)
        inSector_[eta * setup->numSectorsPhi() + phi] = phis[phi];
  }

  TTDTC::Frame GeometricProcessor::Stub::toFrame(int sector, const trackerDTC::Setup* setup) {
    const int sectorPhi = sector % setup->numSectorsPhi();
    const int sectorEta = sector / setup->numSectorsPhi();
    const TTBV hwGap(0, setup->gpNumUnusedBits());
    const TTBV hwValid(1, 1);
    // parse DTC stub
    TTBV ttBV(ttBV_);
    const TTBV hwLayer(ttBV, setup->widthLayer());
    ttBV >>= 2 * setup->widthSectorEta() + setup->numSectorsPhi() + setup->widthLayer();
    const TTBV hwQoverPtMinMax(ttBV, 2 * setup->htWidthQoverPt());
    ttBV >>= 2 * setup->htWidthQoverPt();
    double z = ttBV.val(setup->baseZ(), setup->widthZ(), 0, true);
    ttBV >>= setup->widthZ();
    double phi = ttBV.val(setup->basePhi(), setup->widthPhiDTC(), 0, true);
    ttBV >>= setup->widthPhiDTC();
    double r = ttBV.val(setup->baseR(), setup->widthR(), 0, true);
    const TTBV hwR(ttBV, setup->widthR());
    // transform phi and z
    r += setup->chosenRofPhi();
    phi += (sectorPhi - .5) * setup->baseSector();
    z -= r * setup->sectorCot(sectorEta);
    const TTBV hwPhi(phi, setup->basePhi(), setup->widthPhi(), true);
    const TTBV hwZ(z, setup->baseZ(), setup->widthChiZ(), true);
    // assemble final bitset
    const TTDTC::BV bv(hwGap.str() + hwValid.str() + hwR.str() + hwPhi.str() + hwZ.str() + hwQoverPtMinMax.str() + hwLayer.str());
    return make_pair(ttStubRef_, bv);
  }

  GeometricProcessor::Stub* GeometricProcessor::pop_front(deque<GeometricProcessor::Stub*>& stubs) {
    Stub* stub = nullptr;
    if (!stubs.empty()) {
      stub = stubs.front();
      stubs.pop_front();
    }
    return stub;
  }

}