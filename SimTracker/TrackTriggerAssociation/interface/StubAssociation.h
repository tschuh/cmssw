#ifndef SimTracker_TrackTriggerAssociation_StubAssociation_h
#define SimTracker_TrackTriggerAssociation_StubAssociation_h

#include "SimTracker/TrackTriggerAssociation/interface/TTTypes.h"
#include "L1Trigger/TrackerDTC/interface/Setup.h"

#include <vector>
#include <map>

namespace tt {

  /*! \class  tt::StubAssociation
   *  \brief  Class to store the MC truth of L1 Track Trigger stubs
   *  \author Thomas Schuh
   *  \date   2020, Apr
   */
  class StubAssociation {
  public:
    StubAssociation() { setup_ = nullptr; }
    StubAssociation(const trackerDTC::Setup* setup) : setup_(setup) {}
    ~StubAssociation() {}

    void insert(const TPPtr& tpPtr, const std::vector<TTStubRef>& ttSTubRefs);
    const std::map<TTStubRef, std::vector<TPPtr>>& getTTStubToTrackingParticlesMap() const { return mapTTStubRefsTPPtrs_; }
    const std::map<TPPtr, std::vector<TTStubRef>>& getTrackingParticleToTTStubsMap() const { return mapTPPtrsTTStubRefs_; }
    const std::vector<TPPtr>& findTrackingParticlePtrs(const TTStubRef& ttStubRef) const;
    const std::vector<TTStubRef>& findTTStubRefs(const TPPtr& tpPtr) const;
    int numStubs() const { return mapTTStubRefsTPPtrs_.size(); };
    int numTPs() const { return mapTPPtrsTTStubRefs_.size(); };
    //
    std::vector<TPPtr> associate(const std::vector<TTStubRef>& ttStubRefs) const;
    //
    std::vector<TPPtr> associate(const std::vector<TTStubRef>& ttStubRefs, double minPt, int minLayers) const;

  private:
    const trackerDTC::Setup* setup_;
    std::map<TTStubRef, std::vector<TPPtr>> mapTTStubRefsTPPtrs_;
    std::map<TPPtr, std::vector<TTStubRef>> mapTPPtrsTTStubRefs_;
    const std::vector<TPPtr> nullTPPtrs_;
    const std::vector<TTStubRef> nullTTStubRefs_;
  };

} // namespace tt

#endif