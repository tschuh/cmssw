#include "L1Trigger/L1TTrackMatch/interface/TrackRouter.h"

#include <vector>
#include <deque>
#include <algorithm>
#include <iterator>
#include <cmath>

namespace l1GTT {

  TrackRouter::TrackRouter(const Setup* setup) : setup_(setup) {}

  // fill output products
  void TrackRouter::produce(const tt::StreamsTT& input, tt::StreamsTT& output) {
    for (int channelOut = 0; channelOut < setup_->trNumChannel(); channelOut++) {
      // helper
      auto valid = [channelOut, this](const TTTrackRef& ttTrackRef) {
        const double etaABS = std::abs(ttTrackRef->eta());
        const double dZ = setup_->trC0() + setup_->trC1() * etaABS + setup_->trC2() * std::pow(etaABS, 2);
        const int zMin = std::floor((ttTrackRef->z0() - dZ / 2) / (-TTTrack_TrackWord::minZ0) * setup_->trNumBins());
        const int zMax = std::floor((ttTrackRef->z0() + dZ / 2) / (-TTTrack_TrackWord::minZ0) * setup_->trNumBins());
        const int binMin = std::max(zMin + setup_->trNumBins() / 2, 0);
        const int binMax = std::min(zMax + setup_->trNumBins() / 2, setup_->trNumBins() - 1);
        bool valid(false);
        for (int bin = binMin; bin <= binMax; bin++)
          if (bin % setup_->trNumChannel() == channelOut)
            valid = true;
        return valid ? &ttTrackRef : nullptr;
      };
      // input streams of tracks
      std::vector<std::deque<const TTTrackRef*>> inputs(setup_->tfNumChannel());
      for (int channelIn = 0; channelIn < setup_->tfNumChannel(); channelIn++) {
        const tt::StreamTT& streamTT = input[channelIn];
        std::transform(streamTT.begin(), streamTT.end(), std::back_inserter(inputs[channelIn]), valid);
      }
      // fifo for each stream
      std::vector<std::deque<const TTTrackRef*>> stacks(setup_->tfNumChannel());
      // output stream
      std::deque<const TTTrackRef*> outputs;
      // clock accurate firmware emulation, each while trip describes one clock tick, numMerged tracks in per tick and one track out per tick
      while (!std::all_of(inputs.begin(), inputs.end(), [](const std::deque<const TTTrackRef*>& tracks) {
        return tracks.empty();
      }) || !std::all_of(stacks.begin(), stacks.end(), [](const std::deque<const TTTrackRef*>& tracks) {
        return tracks.empty();
      })) {
        // fill input fifos
        for (int channelIn = 0; channelIn < setup_->tfNumChannel(); channelIn++) {
          std::deque<const TTTrackRef*>& stack = stacks[channelIn];
          const TTTrackRef* track = pop_front(inputs[channelIn]);
          if (track) {
            // buffer overflow
            if (setup_->enableTruncation() && static_cast<int>(stack.size()) == setup_->trDepthMemory() - 1)
              pop_front(stack);
            stack.push_back(track);
          }
        }
        // merge input fifos to one stream, prioritizing higher input channel over lower channel
        bool nothingToRoute(true);
        for (int channelIn = 0; channelIn < setup_->tfNumChannel(); channelIn++) {
          const TTTrackRef* track = pop_front(stacks[channelIn]);
          if (track) {
            nothingToRoute = false;
            outputs.push_back(track);
            break;
          }
        }
        if (nothingToRoute)
          outputs.push_back(nullptr);
      }
      // truncate if desired
      if (setup_->enableTruncation() && static_cast<int>(outputs.size()) > setup_->numFrames())
        outputs.resize(setup_->numFrames());
      // remove all gaps between end and last track
      for (auto it = outputs.end(); it != outputs.begin();)
        it = (*--it) ? outputs.begin() : outputs.erase(it);
      // store output
      tt::StreamTT& storage = output[channelOut];
      storage.reserve(outputs.size());
      for (const TTTrackRef* track : outputs)
        storage.emplace_back(track ? *track : TTTrackRef());
    }
  }

  // remove and return first element of deque, returns nullptr if empty
  const TTTrackRef* TrackRouter::pop_front(std::deque<const TTTrackRef*>& tracks) const {
    const TTTrackRef* track = nullptr;
    if (!tracks.empty()) {
      track = tracks.front();
      tracks.pop_front();
    }
    return track;
  }

}  // namespace l1GTT
