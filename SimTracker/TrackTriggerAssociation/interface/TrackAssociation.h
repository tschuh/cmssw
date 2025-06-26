#ifndef SimTracker_TrackTriggerAssociation_TrackAssociation_h
#define SimTracker_TrackTriggerAssociation_TrackAssociation_h

#include "SimDataFormats/Associations/interface/TTTypes.h"
#include "L1Trigger/TrackTrigger/interface/Setup.h"
#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"

#include <vector>
#include <map>

namespace tt {

  /*! \class  tt::TrackAssociation
   *  \brief  Class to associate reconstrucable TrackingVertices with TTTracks and vice versa.
   *  \author Thomas Schuh
   *  \date   2025, June
   */
  class TrackAssociation {
  public:
    // configuration
    struct Config {
      // minimum number of reconstuctabel TPs associated to a Vertex to consider it reconstuctabel
      int minTPs_;
      // max distance to the beamline to consider a vertex as reconstuctabel in cm
      double maxR_;
      // max z psotion to consider a vertex as reconstuctabel in cm
      double maxZ_;
      // minimum fraction of shared tracks to consider a candidate matched to a vertex
      double minFrac_;
      // maximum fraction of not shared tracks to still consider a candidate matched to a vertex
      double maxFrac_;
    };
    TrackAssociation() {}
    TrackAssociation(const Setup*, const Config*, const StubAssociation*, const std::vector<TTTrackRef>&);
    ~TrackAssociation() = default;
    // returns map containing TTTrackRef and their associated collection of TVPtrs
    const std::map<TTTrackRef, std::vector<TVPtr>>& getTTTrackToTrackingVerticesMap() const {
      return mapTTTrackRefsTVPtrs_;
    }
    // returns map containing TVPtr and their associated collection of TTTrackRefs
    const std::map<TVPtr, std::vector<TTTrackRef>>& getTrackingVertexToTTTracksMap() const {
      return mapTVPtrTTTrackRefs_;
    }
    // returns collection of TVPtrs associated to given TTTrackRef
    const std::vector<TVPtr>& findTrackingVertexPtrs(const TTTrackRef& ttTrackRef) const;
    // returns collection of TTTrackRefs associated to given TVPtr
    const std::vector<TTTrackRef>& findTTTrackRefs(const TVPtr& tvPtr) const;
    // total number of tracks associated with TVs
    int numTracks() const { return mapTTTrackRefsTVPtrs_.size(); };
    // total number of TVs associated with tracks
    int numTVs() const { return mapTVPtrTTTrackRefs_.size(); };
    // Get all TVs that are matched to these tracks
    std::vector<TVPtr> associate(std::vector<TTTrackRef>& ttTrackRefs) const;

  private:
    // stores, calculates and provides run-time constants
    const Setup* setup_ = nullptr;
    // configuration
    const Config* config_;
    // map containing TTTrackRef and their associated collection of TVPtrs
    std::map<TTTrackRef, std::vector<TVPtr>> mapTTTrackRefsTVPtrs_;
    // map containing TVPtr and their associated collection of TTTrackRefs
    std::map<TVPtr, std::vector<TTTrackRef>> mapTVPtrTTTrackRefs_;
    // empty container of TVPtr
    const std::vector<TVPtr> emptyTVPtrs_;
    // empty container of TTTrackRef
    const std::vector<TTTrackRef> emptyTTTrackRefs_;
  };

}  // namespace tt

#endif
