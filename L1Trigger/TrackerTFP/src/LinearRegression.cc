#include "L1Trigger/TrackerTFP/interface/LinearRegression.h"

using namespace std;
using namespace edm;
using namespace trackerDTC;

#include <numeric>
#include <vector>
#include <iterator>
#include <algorithm>

namespace trackerTFP {

  LinearRegression::LinearRegression(const ParameterSet& iConfig, const Setup* setup, const DataFormats* dataFormats, int region) :
    setup_(setup),
    dataFormats_(dataFormats),
    region_(region),
    input_(dataFormats_->numChannel(Process::mht)),
    stubMap_(setup_->numLayers()),
    layerPopulation_(setup_->numLayers()),
    residuals_(setup_->numLayers()) {}

  void LinearRegression::consume(const TTDTC::Streams& streams) {
    auto valid = [](int& sum, const TTDTC::Frame& frame){ return sum += (frame.first.isNonnull() ? 1 : 0); };
    int nStubsMHT(0);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      const TTDTC::Stream& stream = streams[region_ * dataFormats_->numChannel(Process::mht) + channel];
      nStubsMHT += accumulate(stream.begin(), stream.end(), 0, valid);
    }
    stubsMHT_.reserve(nStubsMHT);
    stubsLR_.reserve(nStubsMHT);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      const TTDTC::Stream& stream = streams[region_ * dataFormats_->numChannel(Process::mht) + channel];
      vector<StubMHT*>& input = input_[channel];
      input.reserve(stream.size());
      for (const TTDTC::Frame& frame : stream) {
        StubMHT* stub = nullptr;
        if (frame.first.isNonnull()) {
          stubsMHT_.emplace_back(frame, dataFormats_);
          stub = & stubsMHT_.back();
        }
        input.push_back(stub);
      }
    }
  }

  void LinearRegression::produce(TTDTC::Streams& stubs, TTDTC::Streams& tracks) {
    for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
      const vector<StubMHT*>& input = input_[channel];
      TTDTC::Stream& streamStubs = stubs[region_ * dataFormats_->numChannel(Process::mht) + channel];
      TTDTC::Stream& streamTracks = tracks[region_ * dataFormats_->numChannel(Process::mht) + channel];
      streamStubs.reserve(input.size());
      streamTracks.reserve(input.size());
      for (auto it = input.begin(); it != input.end();) {
        if (!*it) {
          streamStubs.emplace_back(TTDTC::Frame());
          streamTracks.emplace_back(TTDTC::Frame());
          it++;
          continue;
        }
        const auto start = it;
        const int id = (*it)->trackId();
        auto different = [id](StubMHT* stub){ return !stub || id != stub->trackId(); };
        it = find_if(it, input.end(), different);
        vector<StubMHT*> track;
        StubLR* stub = nullptr;
        track.reserve(distance(start, it));
        copy(start, it, back_inserter(track));
        //fit(track, stub);
        for (StubMHT* stub : track)
          streamStubs.emplace_back(stub ? stub->frame() : TTDTC::Frame());
        streamTracks.insert(streamTracks.end(), track.size(), stub ? stub->frame() : TTDTC::Frame());
      }
    }
  }

  // fit track
  void LinearRegression::fit(vector<StubMHT*>& stubs, StubLR* track) {
    initFit(stubs);
    if (valid_) {
      while (nIterations_++ < setup_->lrNumIterations()) {
        calcHelix();
        calcResidual();
        const bool nothingToKill = killLargestResidual();
        if (nothingToKill)
          break;
      }
    }
    if (valid_) {
      stubsLR_.emplace_back(*stubs.front(), phiT_, qOverPt_, zT_, cot_);
      track = &stubsLR_.back();
      for (auto& stub : stubs)
        if (find(stubs_.begin(), stubs_.end(), stub) == stubs_.end())
          stub = nullptr;
    } else {
      for (auto& stub : stubs)
        stub = nullptr;
    }
  }

  //
  void LinearRegression::initFit(const vector<StubMHT*>& stubs) {
    for (auto& residuals : residuals_)
      residuals.clear();
    for (auto& stubMap : stubMap_)
      stubMap.clear();
    layerPopulation_ = vector<int>(setup_->numLayers(), 0);
    nIterations_ = 0;
    largestResid_ = Residual(-1.);
    phiT_ = 0.;
    qOverPt_ = 0.;
    zT_ = 0.;
    cot_ = 0.;
    stubs_ = stubs;
    valid_ = checkValidity(stubs_);
    if (!valid_)
      return;
    for (auto const &stub : stubs_) {
      stubMap_[stub->layer()].push_back(stub);
      layerPopulation_[stub->layer()]++;
    }
  }

  //
  bool LinearRegression::checkValidity(const vector<StubMHT*>& stubs) const {
    bool valid = true;
    if (countLayers(stubs) < setup_->lrMinLayers())
      valid = false;
    if (countLayers(stubs, true) < setup_->lrMinLayersPS())
      valid = false;
    return valid;
  }

  //
  int LinearRegression::countLayers(const vector<StubMHT*>& stubs, bool onlySeed) const {
    TTBV foundLayers(0, setup_->numLayers());
    for (auto const& stub : stubs)
      if (!onlySeed || stub->psModule())
        foundLayers.set(stub->layer());
    return foundLayers.count();
  }

  //
  void LinearRegression::calcHelix() {
    Sum phiSums, zSums;
    for (auto const& stubs : stubMap_) {
      if (stubs.empty())
        continue;
      Stub layerMinPos(numeric_limits<double>::infinity(), numeric_limits<double>::infinity(), numeric_limits<double>::infinity(), numeric_limits<double>::infinity());
      Stub layerMaxPos(-numeric_limits<double>::infinity(), -numeric_limits<double>::infinity(), -numeric_limits<double>::infinity(), -numeric_limits<double>::infinity());
      bool ps(false);
      for (auto const& stub : stubs) {
        Stub pos(stub, setup_);
        if (stub->psModule()) {
          ps = true;
          layerMinPos <= pos;
          layerMaxPos >= pos;
        } else {
          layerMinPos.RPhi = min(layerMinPos.RPhi, pos.RPhi);
          layerMinPos.Phi = min(layerMinPos.Phi, pos.Phi);
          layerMaxPos.RPhi = max(layerMaxPos.RPhi, pos.RPhi);
          layerMaxPos.Phi = max(layerMaxPos.Phi, pos.Phi);
        }
      }
      Stub layerPos = layerMinPos + layerMaxPos;
      layerPos /= 2.;
      phiSums += make_pair(layerPos.RPhi, layerPos.Phi);
      if (ps)
        zSums += make_pair(layerPos.RZ, layerPos.Z);
    }
    const pair<float, float>& phiParameter = phiSums.calcLinearParameter();
    const pair<float, float>& zParameter = zSums.calcLinearParameter();
    phiT_ = phiParameter.second;
    qOverPt_ = phiParameter.first;
    zT_ = zParameter.second;
    cot_ = zParameter.first;
  }

  //
  void LinearRegression::calcResidual() {
    for (auto& residuals : residuals_)
      residuals.clear();
    for (int layer = 0; layer < setup_->numLayers(); layer++) {
      auto& stubs = stubMap_[layer];
      for (auto const &stub : stubs) {
        Stub pos(stub, setup_);
        float zResid = pos.Z - (zT_ + cot_ * pos.RZ);
        float phiResid = deltaPhi(pos.Phi - (phiT_ - qOverPt_ * pos.RPhi));
        Residual resid(fabs(phiResid), fabs(zResid), stub);
        resid.phi /= setup_->lrResidPhi();
        if (!stub->barrel())
          resid.z /= fabs(cot_);
        if (stub->psModule())
          resid.z /= setup_->lrResidZPS();
        else
          resid.z /= setup_->lrResidZ2S();
        residuals_[layer].push_back(resid);
      }
    }
  }

  //
  bool LinearRegression::killLargestResidual() {
    findLargestResidual();
    valid_ = largestResid_.combined() < 2.;
    if (countStubs(stubs_) == 4)
      return true;
    const int& layerID = largestResid_.layerId();
    const auto& stub = largestResid_.stub;
    auto& stubs = stubMap_[layerID];
    stubs_.erase(remove(stubs_.begin(), stubs_.end(), stub ), stubs_.end());
    stubs.erase(remove(stubs.begin(), stubs.end(), stub ), stubs.end());
    layerPopulation_[layerID]--;
    return false;
  }

  //
  void LinearRegression::findLargestResidual() {
    largestResid_ = Residual(-1.);
    for (int layer = 0; layer < setup_->numLayers(); layer++) {
      auto& residuals = residuals_[layer];
      if (residuals.empty())
        continue;
      bool single = residuals.size() == 1;
      bool notPurged = countStubs(stubs_) != countLayers(stubs_);
      bool layerCritical = countLayers(stubs_) == setup_->lrMinLayers();
      bool psCritical = countLayers(stubs_, true) == setup_->lrMinLayersPS();
      bool ctrical = countStubs(stubs_) == setup_->lrMinLayers();
      if (single && notPurged && layerCritical)
        continue;
      for (const auto& resid : residuals) {
        if(resid.ps() && psCritical && !ctrical)
          if (countStubs(stubs_, true) == setup_->lrMinLayersPS() || single)
            continue;
        if (resid.combined() > largestResid_.combined())
          largestResid_ = resid;
      }
    }
  }

  //
  int LinearRegression::countStubs(const vector<StubMHT*>& stubs, bool onlyPS) const {
    int nStubs(0);
    if (onlyPS) {
      for (const auto& stub : stubs)
        if (stub->psModule())
          nStubs++;
    } else
      nStubs = stubs_.size();
    return nStubs;
  }

} // namespace trackerTFP