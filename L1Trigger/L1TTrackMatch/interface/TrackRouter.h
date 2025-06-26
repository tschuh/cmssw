
#ifndef L1Trigger_L1TTrackMatch_TrackRouter_h
#define L1Trigger_L1TTrackMatch_TrackRouter_h

#include "L1Trigger/L1TTrackMatch/interface/Setup.h"
#include "DataFormats/L1TrackTrigger/interface/TTTypes.h"

#include <deque>

namespace l1GTT {

  // class to emulate GTT input routing
  class TrackRouter {
  public:
    TrackRouter(const Setup*);
    ~TrackRouter() = default;

    // fill output data
    void produce(const tt::StreamsTT&, tt::StreamsTT&);

  private:
    // remove and return first element of deque, returns nullptr if empty
    const TTTrackRef* pop_front(std::deque<const TTTrackRef*>&) const;
    // provides run-time constants
    const Setup* setup_;
  };

}  // namespace l1GTT

#endif
