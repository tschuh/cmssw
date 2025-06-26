#include "L1Trigger/L1TTrackMatch/interface/TrackFormatter.h"

#include <vector>
#include <deque>
#include <iterator>
#include <algorithm>

namespace l1GTT {

  TrackFormatter::TrackFormatter(const Setup* setup) : setup_(setup) {}

  // fill output products
  void TrackFormatter::produce(const tt::StreamsTrack& input, tt::StreamsTT& output) const {
    int channelTF(0);
    for (int channelTFP = 0; channelTFP < setup_->tfpNumChannel(); channelTFP++) {
      for (int mergedRegion = 0; mergedRegion < setup_->tfNumMergedRegions(); mergedRegion++) {
        // this node has partialFast inputs and partialSlow outputs
        const int offset = mergedRegion * setup_->tfPartialFast() * setup_->tfPartialSlow() + channelTFP;
        std::vector<std::deque<TTTrackRef>> nodeIn(setup_->tfPartialFast());
        // pre-process node inputs
        for (int fast = 0; fast < setup_->tfPartialFast(); fast++)
          produce(input[offset + fast], nodeIn[fast]);
        // produce node outputs
        std::vector<std::deque<TTTrackRef>> nodeOut(setup_->tfPartialSlow());
        produce(nodeIn, nodeOut);
        // fill output
        for (const std::deque<TTTrackRef>& out : nodeOut)
          output[channelTF++] = tt::StreamTT(out.begin(), out.end());
      }
    }
  }

  // pre-process node inputs
  void TrackFormatter::produce(const tt::StreamTrack& input, std::deque<TTTrackRef>& output) const {
    TTTrackRef ttTrackRef;
    for (const tt::FrameTrack& frameTrack : input) {
      if (ttTrackRef.isNull()) {
        ttTrackRef = frameTrack.first;
        output.emplace_back(TTTrackRef());
        continue;
      }
      output.push_back(ttTrackRef);
      ttTrackRef = ttTrackRef == frameTrack.first ? TTTrackRef() : frameTrack.first;
    }
    output.pop_front();
  }

  // produce node outputs
  void TrackFormatter::produce(std::vector<std::deque<TTTrackRef>>& input,
                               std::vector<std::deque<TTTrackRef>>& output) const {
    auto isNull = [](const TTTrackRef& ttTrackRef) { return ttTrackRef.isNull(); };
    output[0] = input[0];
    output[1] = input[2];
    for (int index = 0; index < static_cast<int>(input[1].size()); index++) {
      const TTTrackRef& ttTrackRef = input[1][index];
      if (ttTrackRef.isNull())
        continue;
      std::deque<TTTrackRef>& out = index % 2 == 0 ? input[0] : input[1];
      if (index >= static_cast<int>(out.size())) {
        out.push_back(ttTrackRef);
        continue;
      }
      const auto it = find_if(std::next(out.begin(), index), out.end(), isNull);
      if (it != out.end())
        it->operator=(ttTrackRef);
      else
        out.push_back(ttTrackRef);
    }
  }

}  // namespace l1GTT
