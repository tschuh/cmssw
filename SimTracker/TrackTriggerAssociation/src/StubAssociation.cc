#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"

#include <map>
#include <vector>

using namespace std;
using namespace trackerDTC;

namespace tt {

  void StubAssociation::insert(const TPPtr& tpPtr, const vector<TTStubRef>& ttSTubRefs) {
    mapTPPtrsTTStubRefs_.insert({tpPtr, ttSTubRefs});
    for (const TTStubRef& ttSTubRef : ttSTubRefs)
      mapTTStubRefsTPPtrs_[ttSTubRef].push_back(tpPtr);
  }

  const map<TTStubRef, vector<TPPtr>>& StubAssociation::getTTStubToTrackingParticlesMap() const {
    return mapTTStubRefsTPPtrs_;
  }

  const map<TPPtr, vector<TTStubRef>>& StubAssociation::getTrackingParticleToTTStubsMap() const {
    return mapTPPtrsTTStubRefs_;
  }

  const vector<TPPtr>& StubAssociation::findTrackingParticlePtrs(const TTStubRef& ttStubRef) const {
    const auto it = mapTTStubRefsTPPtrs_.find(ttStubRef);
    return it != mapTTStubRefsTPPtrs_.end() ? it->second : nullTPPtrs_;
  }

  const vector<TTStubRef>& StubAssociation::findTTStubRefs(const TPPtr& tpPtr) const {
    const auto it = mapTPPtrsTTStubRefs_.find(tpPtr);
    return it != mapTPPtrsTTStubRefs_.end() ? it->second : nullTTStubRefs_;
  }

  //
  vector<TPPtr> StubAssociation::associate(const vector<TTStubRef>& ttStubRefs) const {
    map<TPPtr, set<int>> m;
    for (const TTStubRef& ttStubRef : ttStubRefs)
      for (const TPPtr& tpPtr : findTrackingParticlePtrs(ttStubRef))
        m[tpPtr].insert(setup_->layerId(ttStubRef));
    vector<TPPtr> tpPtrs;
    tpPtrs.reserve(m.size());
    for (const auto& p : m)
      if ((int)p.second.size() >= setup_->tpMinLayers())
        tpPtrs.push_back(p.first);
    tpPtrs.shrink_to_fit();
    return tpPtrs;
  }

  //
  vector<TPPtr> StubAssociation::associate(const vector<TTStubRef>& ttStubRefs, double minPt, int minLayers) const {
    map<TPPtr, set<int>> m;
    for (const TTStubRef& ttStubRef : ttStubRefs)
      for (const TPPtr& tpPtr : findTrackingParticlePtrs(ttStubRef))
        if (tpPtr->pt() >= minPt)
          m[tpPtr].insert(setup_->layerId(ttStubRef));
    vector<TPPtr> tpPtrs;
    tpPtrs.reserve(m.size());
    for (const auto& p : m)
      if ((int)p.second.size() >= minLayers)
        tpPtrs.push_back(p.first);
    tpPtrs.shrink_to_fit();
    return tpPtrs;
  }

} // namespace tt