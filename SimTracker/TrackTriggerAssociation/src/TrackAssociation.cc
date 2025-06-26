#include "SimTracker/TrackTriggerAssociation/interface/TrackAssociation.h"
#include "DataFormats/Common/interface/RefToPtr.h"

#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <iterator>

namespace tt {

  TrackAssociation::TrackAssociation(const Setup* setup,
                                     const Config* config,
                                     const StubAssociation* stubAssociation,
                                     const std::vector<TTTrackRef>& ttTrackRefs)
      : setup_(setup), config_(config) {
    // collect reconstuctable vertices
    std::map<TVPtr, std::vector<TPPtr>> mapTVPtrTPPtrs;
    for (const auto& p : stubAssociation->getTrackingParticleToTTStubsMap()) {
      const TVPtr tvPtr = edm::refToPtr(p.first->parentVertex());
      if (!mapTVPtrTPPtrs.contains(tvPtr))
        mapTVPtrTPPtrs[tvPtr].reserve(tvPtr->nDaughterTracks());
      mapTVPtrTPPtrs[tvPtr].push_back(p.first);
    }
    // remove vertices with not enough tracks to be considered reconstructable
    auto nonReconstructable = [this](const auto& p) {
      const bool minTPs = static_cast<int>(p.second.size()) < config_->minTPs_;
      const bool maxR = std::sqrt(p.first->position().perp2()) > config_->maxR_;
      const bool maxZ = std::abs(p.first->position().z()) > config_->maxZ_;
      return minTPs || maxR || maxZ;
    };
    std::erase_if(mapTVPtrTPPtrs, nonReconstructable);
    // associate tracks with vertices
    std::map<TVPtr, int> sizes;
    for (const TTTrackRef& ttTrackRef : ttTrackRefs) {
      const std::vector<TPPtr> tpPtrs = stubAssociation->associateFinal(ttTrackRef->getStubRefs());
      mapTTTrackRefsTVPtrs_[ttTrackRef].reserve(tpPtrs.size());
      for (const TPPtr& tpPtr : tpPtrs) {
        const TVPtr tvPtr = edm::refToPtr(tpPtr->parentVertex());
        if (mapTVPtrTPPtrs.contains(tvPtr))
          continue;
        sizes[tvPtr]++;
        mapTTTrackRefsTVPtrs_[ttTrackRef].push_back(tvPtr);
      }
    }
    // prep vertex to tracks association
    for (const auto& p : sizes)
      mapTVPtrTTTrackRefs_[p.first].reserve(p.second);
    // fill vertex to tracks association
    for (const auto& p : mapTTTrackRefsTVPtrs_)
      for (const TVPtr& tvPtr : p.second)
        mapTVPtrTTTrackRefs_[tvPtr].push_back(p.first);
    // sort vertex to tracks association for later benefit
    for (auto& p : mapTVPtrTTTrackRefs_)
      std::sort(p.second.begin(), p.second.end());
  }

  // returns collection of TVPtrs associated to given TTTrackRef
  const std::vector<TVPtr>& TrackAssociation::findTrackingVertexPtrs(const TTTrackRef& ttTrackRef) const {
    const auto it = mapTTTrackRefsTVPtrs_.find(ttTrackRef);
    return it != mapTTTrackRefsTVPtrs_.end() ? it->second : emptyTVPtrs_;
  }

  // returns collection of TTtrackRefs associated to given TVPtr
  const std::vector<TTTrackRef>& TrackAssociation::findTTTrackRefs(const TVPtr& tvPtr) const {
    const auto it = mapTVPtrTTTrackRefs_.find(tvPtr);
    return it != mapTVPtrTTTrackRefs_.end() ? it->second : emptyTTTrackRefs_;
  }

  // Get all TVs that are matched to these tracks
  std::vector<TVPtr> TrackAssociation::associate(std::vector<TTTrackRef>& ttTrackRefs) const {
    std::sort(ttTrackRefs.begin(), ttTrackRefs.end());
    std::set<TVPtr> candidates;
    for (const TTTrackRef& ttTrackRef : ttTrackRefs) {
      const std::vector<TVPtr>& tvPtr = this->findTrackingVertexPtrs(ttTrackRef);
      candidates.insert(tvPtr.begin(), tvPtr.end());
    }
    std::vector<TVPtr> tvPtrs;
    tvPtrs.reserve(candidates.size());
    for (const TVPtr& tvPtr : candidates) {
      const std::vector<TTTrackRef>& cands = this->findTTTrackRefs(tvPtr);
      std::vector<TTTrackRef> common;
      common.reserve(std::min(cands.size(), ttTrackRefs.size()));
      std::set_intersection(
          ttTrackRefs.begin(), ttTrackRefs.end(), cands.begin(), cands.end(), std::back_inserter(common));
      const bool minFrac = common.size() / static_cast<double>(cands.size()) > config_->minFrac_;
      const bool maxFrac = common.size() / static_cast<double>(ttTrackRefs.size()) < config_->maxFrac_;
      if (minFrac && maxFrac)
        tvPtrs.push_back(tvPtr);
    }
    return tvPtrs;
  }

}  // namespace tt
