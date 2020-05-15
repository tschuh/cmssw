#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"

#include <map>
#include <vector>

using namespace std;

namespace tt {

  StubAssociation::StubAssociation() {}

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

} // namespace tt