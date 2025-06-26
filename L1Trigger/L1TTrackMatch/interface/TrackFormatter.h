
#ifndef L1Trigger_L1TTrackMatch_TrackFormatter_h
#define L1Trigger_L1TTrackMatch_TrackFormatter_h

#include "L1Trigger/L1TTrackMatch/interface/Setup.h"
#include "DataFormats/L1TrackTrigger/interface/TTTypes.h"

#include <vector>
#include <deque>
#include <bitset>
#include <utility>

namespace l1GTT {

  // class to emulate GTT input formatting
  class TrackFormatter {
  public:
    TrackFormatter(const Setup*);
    ~TrackFormatter() = default;

    // fill output data
    void produce(const tt::StreamsTrack&, tt::StreamsTT&) const;

  private:
    // pre-process node inputs
    void produce(const tt::StreamTrack&, std::deque<TTTrackRef>&) const;
    // produce node outputs
    void produce(std::vector<std::deque<TTTrackRef>>&, std::vector<std::deque<TTTrackRef>>&) const;
    // provides run-time constants
    const Setup* setup_;
  };

}  // namespace l1GTT

#endif
