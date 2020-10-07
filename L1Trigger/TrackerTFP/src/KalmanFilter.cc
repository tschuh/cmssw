#include "L1Trigger/TrackerTFP/interface/KalmanFilter.h"

#include <numeric>
#include <algorithm>
#include <iterator>
#include <deque>
#include <vector>
#include <set>
#include <utility>
#include <cmath>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  KalmanFilter::KalmanFilter(const ParameterSet& iConfig, const Setup* setup, const DataFormats* dataFormats, const KalmanFilterFormats* kalmanFilterFormats, int region) :
    enableTruncation_(iConfig.getParameter<bool>("EnableTruncation")),
    setup_(setup),
    dataFormats_(dataFormats),
    kalmanFilterFormats_(kalmanFilterFormats),
    region_(region),
    inputStubs_(dataFormats_->numChannel(Process::kfin)),
    inputTracks_(dataFormats_->numChannel(Process::kf)),
    channelStubs_(dataFormats_->numChannel(Process::kf)),
    layer_(0),
    x0_(kalmanFilterFormats_->format(VariableKF::x0)),
    x1_(kalmanFilterFormats_->format(VariableKF::x1)),
    x2_(kalmanFilterFormats_->format(VariableKF::x2)),
    x3_(kalmanFilterFormats_->format(VariableKF::x3)),
    H00_(kalmanFilterFormats_->format(VariableKF::H00)),
    H12_(kalmanFilterFormats_->format(VariableKF::H12)),
    m0_(kalmanFilterFormats_->format(VariableKF::m0)),
    m1_(kalmanFilterFormats_->format(VariableKF::m1)),
    v0_(kalmanFilterFormats_->format(VariableKF::v0)),
    v1_(kalmanFilterFormats_->format(VariableKF::v1)),
    r0_(kalmanFilterFormats_->format(VariableKF::r0)),
    r1_(kalmanFilterFormats_->format(VariableKF::r1)),
    r02_(kalmanFilterFormats_->format(VariableKF::r02)),
    r12_(kalmanFilterFormats_->format(VariableKF::r12)),
    S00_(kalmanFilterFormats_->format(VariableKF::S00)),
    S01_(kalmanFilterFormats_->format(VariableKF::S01)),
    S12_(kalmanFilterFormats_->format(VariableKF::S12)),
    S13_(kalmanFilterFormats_->format(VariableKF::S13)),
    K00_(kalmanFilterFormats_->format(VariableKF::K00)),
    K10_(kalmanFilterFormats_->format(VariableKF::K10)),
    K21_(kalmanFilterFormats_->format(VariableKF::K21)),
    K31_(kalmanFilterFormats_->format(VariableKF::K31)),
    R00_(kalmanFilterFormats_->format(VariableKF::R00)),
    R11_(kalmanFilterFormats_->format(VariableKF::R11)),
    invR00_(kalmanFilterFormats_->format(VariableKF::invR00)),
    invR11_(kalmanFilterFormats_->format(VariableKF::invR11)),
    chi20_(kalmanFilterFormats_->format(VariableKF::chi20)),
    chi21_(kalmanFilterFormats_->format(VariableKF::chi21)),
    C00_(kalmanFilterFormats_->format(VariableKF::C00)),
    C01_(kalmanFilterFormats_->format(VariableKF::C01)),
    C11_(kalmanFilterFormats_->format(VariableKF::C11)),
    C22_(kalmanFilterFormats_->format(VariableKF::C22)),
    C23_(kalmanFilterFormats_->format(VariableKF::C23)),
    C33_(kalmanFilterFormats_->format(VariableKF::C33)),
    chi2_(kalmanFilterFormats_->format(VariableKF::chi2)) {}

  // read in and organize input stubs
  void KalmanFilter::consume(const TTDTC::Streams& stubs) {
    auto valid = [](int& sum, const TTDTC::Frame& frame){ return sum += (frame.first.isNonnull() ? 1 : 0); };
    int nStubs(0);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kfin); channel++) {
      const TTDTC::Stream& stream = stubs[region_ * dataFormats_->numChannel(Process::kfin) + channel];
      nStubs += accumulate(stream.begin(), stream.end(), 0, valid);
    }
    stubs_.reserve(nStubs);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kfin); channel++) {
      const int layerId = channel % setup_->numLayers();
      const TTDTC::Stream& stream = stubs[region_ * dataFormats_->numChannel(Process::kfin) + channel];
      vector<StubKFin*>& input = inputStubs_[channel];
      input.reserve(stream.size());
      for (const TTDTC::Frame& frame : stream) {
        StubKFin* stub = nullptr;
        if (frame.first.isNonnull()) {
          stubs_.emplace_back(frame, dataFormats_, layerId);
          stub = &stubs_.back();
        }
        input.push_back(stub);
      }
    }
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kf); channel++) {
      const int offset = channel * setup_->numLayers();
      vector<StubKFin*>& stubs = channelStubs_[channel];
      int nStubs(0);
      for (int layer = 0; layer < setup_->numLayers(); layer++) {
        const vector<StubKFin*>& stream = inputStubs_[offset + layer];
        nStubs += accumulate(stream.begin(), stream.end(), 0, [](int& sum, StubKFin* stub){ return sum += stub ? 1 : 0; });
      }
      stubs.reserve(nStubs);
      for (int layer = 0; layer < setup_->numLayers(); layer++)
        for (StubKFin* stub : inputStubs_[offset + layer])
          if (stub)
            stubs.push_back(stub);
    }
  }

  // read in and organize input tracks
  void KalmanFilter::consume(const StreamsTrack& tracks) {
    auto valid = [](int& sum, const FrameTrack& frame){ return sum += (frame.first.isNonnull() ? 1 : 0); };
    int nTracks(0);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kf); channel++) {
      const StreamTrack& stream = tracks[region_ * dataFormats_->numChannel(Process::kf) + channel];
      nTracks += accumulate(stream.begin(), stream.end(), 0, valid);
    }
    tracks_.reserve(nTracks);
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kf); channel++) {
      const vector<StubKFin*>& stubs = channelStubs_[channel];
      const StreamTrack& stream = tracks[region_ * dataFormats_->numChannel(Process::kf) + channel];
      vector<TrackKFin*>& tracks = inputTracks_[channel];
      tracks.reserve(stream.size());
      for (const FrameTrack& frame : stream) {
        TrackKFin* track = nullptr;
        if (frame.first.isNonnull()) {
          tracks_.emplace_back(frame, dataFormats_, stubs);
          track = &tracks_.back();
        }
        tracks.push_back(track);
      }
    }
  }

  // fill output products
  void KalmanFilter::produce(StreamTrack& accepted, StreamTrack& lost) {
    deque<State*> statesLost;
    deque<State*> statesAccepted;
    auto truncate = [&statesLost, this](deque<State*>& stream) {
      if (enableTruncation_ && (int)stream.size() > setup_->numFrames()) {
        const auto it = next(stream.begin(), setup_->numFrames());
        copy_if(it, stream.end(), back_inserter(statesLost), [](State* state){ return state; });
        stream.erase(it, stream.end());
      }
    };
    auto put = [this](const deque<State*>& states, StreamTrack& streamTrack) {
      auto toFrameTrack = [](State* state){ return state->frame(); };
      streamTrack.reserve(states.size());
      transform(states.begin(), states.end(), back_inserter(streamTrack), toFrameTrack);
    };
    vector<deque<State*>> streams(dataFormats_->numChannel(Process::kf));
    for (int channel = 0; channel < dataFormats_->numChannel(Process::kf); channel++) {
      deque<State*>& stream = streams[channel];
      for (TrackKFin* track : inputTracks_[channel]) {
        State* state = nullptr;
        if (track) {
          states_.emplace_back(dataFormats_, track);
          state = &states_.back();
        }
        stream.push_back(state);
      }
      for (layer_ = 0; layer_ < setup_->numLayers(); layer_++)
        layer(stream);
      truncate(stream);
      accumulator(stream);
    }
    for (const deque<State*>& stream : streams)
      copy(stream.begin(), stream.end(), back_inserter(statesAccepted));
    truncate(statesAccepted);
    put(statesAccepted, accepted);
    accumulator(statesLost);
    auto recovered = [statesAccepted](State* state) {
      return find_if(statesAccepted.begin(), statesAccepted.end(), [state](State* it){ return it->track() == state->track(); }) != statesAccepted.end();
    };
    statesLost.erase(remove_if(statesLost.begin(), statesLost.end(), recovered), statesLost.end());
    put(statesLost, lost);
  }

  // hit pattern check
  bool KalmanFilter::valid(TrackKFin* track) const {
    // gap
    if (!track)
      return false;
    const TTBV& hitPattern = track->hitPattern();
    // not enough total layer
    if (hitPattern.count(0, setup_->kfMinLayers() + setup_->kfMaxSkippedLayers()) < setup_->kfMinLayers())
      return false;
    // not enough inner layer
    if (hitPattern.count(0, 3) < 2)
      return false;
    return true;
  }

  // adds a layer to states
  void KalmanFilter::layer(deque<State*>& stream) {
    static const int latency = 5;
    // dynamic state container for clock accurate emulation
    deque<State*> streamOutput;
    deque<State*> stack;
    // static delay container
    vector<State*> delay(latency, nullptr);
    delay.reserve(latency + 1);
    // each trip corresponds to a f/w clock tick
    // done if no states to process left, taking as much time as needed
    while (!stream.empty() || !stack.empty() || !all_of(delay.begin(), delay.end(), [](const State* state){ return state == nullptr; })) {
      State* state = pop_front(stream);
      if (!state)
        state = pop_front(stack);
      streamOutput.push_back(state);
      if (!state || !state->stub() || state->layer() != layer_)
        state = nullptr;
      if (state != nullptr)
        comb(state);
      delay.push_back(state);
      state = pop_front(delay);
      if (state != nullptr)
        stack.push_back(state);
    }
    stream = streamOutput;
    for (State*& state : stream) {
      if (!state || !state->stub() || state->layer() != layer_ )
        continue;
      update(state);
    }
  }

  // repicks combinatoric stubs for state
  void KalmanFilter::comb(State*& state) {
    // picks next stub on layer
    StubKFin* stub = state->stub();
    const int layer = stub->layer();
    TrackKFin* track = state->track();
    const vector<StubKFin*>& stubs = track->layerStubs(layer);
    const TTBV& hitPattern = state->hitPattern();
    const int pos = distance(stubs.begin(), find(stubs.begin(), stubs.end(), stub)) + 1;
    if (pos != (int)stubs.size()) {
      states_.emplace_back(state, stubs[pos]);
      state = &states_.back();
    } else {
      // picks first stub on next layer, nullifies state if skipping layer is not valid
      bool valid(true);
      // would create two skipped layer in a row
      /*if ((layer > 0 && !track->hitPattern(layer - 1)) || !track->hitPattern(layer + 1))
        valid = false;*/
      // not enough inner layers remain after skipping
      /*if (hitPattern.count(0, 3) + track->hitPattern().count(layer + 1, 3) < 2 )
        valid = false;*/
      // not enough layers remain after skipping
      /*if (hitPattern.count() + track->hitPattern().count(layer + 1, setup_->numLayers()) < setup_->kfMaxLayers())
        valid = false;*/
      if (valid) {
        StubKFin* stub = nullptr;
        if (hitPattern.count() != setup_->kfMaxLayers()) {
          for (int nextLayer = layer_ + 1; nextLayer < setup_->numLayers(); nextLayer++) {
            if (track->hitPattern(nextLayer)) {
              stub = track->layerStub(nextLayer);
              break;
            }
          }
        }
        states_.emplace_back(state, stub);
        state = &states_.back();
      } else
        state = nullptr;
    }
  }

  // best state selection
  void KalmanFilter::accumulator(deque<State*>& stream) {
    // accumulator delivers contigious stream of best state per track
    // remove gaps and not final states
    stream.erase(remove_if(stream.begin(), stream.end(), [](State* state){ return !state || state->hitPattern().count() < 4; }), stream.end());
    // sort in quality (combination of chi2 and skipped layers)
    stable_sort(stream.begin(), stream.end(), [](State* lhs, State* rhs){ return lhs->quali() < rhs->quali();  });
    // sort in track id
    stable_sort(stream.begin(), stream.end(), [](State* lhs, State* rhs){ return lhs->trackId() < rhs->trackId(); });
    // keep first state (best due to previous sort) per track id
    stream.erase(unique(stream.begin(), stream.end(), [](State* lhs, State* rhs){ return lhs->track() == rhs->track(); }), stream.end());
  }

  // updates state
  void KalmanFilter::update(State*& state) {
    static const vector<double> chi2Cuts = {9.9e9, 10., 30., 80., 120., 160.};
    const double chi2Cut = chi2Cuts[state->hitPattern().count()] + 9.e9;
    // R-Z plane
    bool valid2(true);
    bool valid3(true);
    const double H12 = H12_.digif(state->H12());
    const double m1 = m1_.digif(state->m1());
    const double v1 = v1_.digif(state->v1());
    double x2 = x2_.digir(state->x2());
    double x3 = x3_.digir(state->x3());
    double C22 = C22_.digif(state->C22());
    double C23 = C23_.digif(state->C23());
    double C33 = C33_.digif(state->C33());
    double chi21 = chi21_.digif(state->chi21());
    v1_.updateRangeActual(v1);
    C22_.updateRangeActual(C22);
    C23_.updateRangeActual(C23);
    C33_.updateRangeActual(C33);
    H12_.updateRangeActual(H12);
    m1_.updateRangeActual(m1);
    const double r1C = r1_.digif(m1  - x3);
    const double r1 = r1_.digif(r1C - x2 * H12);
    const double r12 = r12_.digif(r1 * r1);
    const double S12 = S12_.digif(C23 + H12 * C22);
    const double S13 = S13_.digif(C33 + H12 * C23);
    const double R11C = S13_.digif(v1 + S13);
    const double R11 = R11_.digif(R11C + H12 * S12);
    if (R11 < 0.)
      throw cms::Exception("NumericInstabillity") << "R11 got negative.";
    // dynamic cancelling
    const int msbZ = (int)ceil(log2(R11 / R11_.base()));
    const int shiftZ = msbZ > setup_->kfWidthLutInvZ() ? msbZ - setup_->kfWidthLutInvZ() : 0;
    const double shiftedS12 = S12_.digif(S12 / pow(2, shiftZ));
    const double shiftedS13 = S13_.digif(S13 / pow(2, shiftZ));
    const double shiftedR11 = R11_.digif(R11 / pow(2, shiftZ));
    const double invR11 = invR11_.digif(1. / shiftedR11);
    const double K21 = K21_.digif(shiftedS12 * invR11);
    const double K31 = K31_.digif(shiftedS13 * invR11);
    x2 = x2_.digir(x2 + r1 * K21);
    x3 = x3_.digir(x3 + r1 * K31);
    C22 = C22_.digif(C22 - S12 * K21);
    C23 = C23_.digif(C23 - S13 * K21);
    C33 = C33_.digif(C33 - S13 * K31);
    chi21 = chi21_.digif(chi21 + r12 * invR11);
    if (C22 < 0.)
      throw cms::Exception("NumericInstabillity") << "C22 got negative.";
    if (C33 < 0.)
      throw cms::Exception("NumericInstabillity") << "C33 got negative.";
    // loose slope cut
    /*if (fabs(x2) > dataFormats_->base(Variable::cot, Process::sf))
      valid2 = false;*/
    // loose intercept cut
    /*if (fabs(x3) > dataFormats_->base(Variable::zT, Process::sf))
      valid3 = false;*/
    S12_.updateRangeActual(S12);
    S13_.updateRangeActual(S13);
    K21_.updateRangeActual(K21);
    K31_.updateRangeActual(K31);
    R11_.updateRangeActual(R11);
    invR11_.updateRangeActual(invR11);
    r1_.updateRangeActual(r1);
    r12_.updateRangeActual(r12);
    if (valid2 && valid3)  {
      C22_.updateRangeActual(C22);
      C23_.updateRangeActual(C23);
      C33_.updateRangeActual(C33);
      chi21_.updateRangeActual(chi21);
      x2_.updateRangeActual(x2);
      x3_.updateRangeActual(x3);
    }
    // R-Phi plane
    bool valid0(true);
    bool valid1(true);
    const double H00 = H00_.digif(state->H00());
    const double m0 = m0_.digif(state->m0());
    const double v0 = v0_.digif(state->v0());
    double x0 = x0_.digir(state->x0());
    double x1 = x1_.digir(state->x1());
    double C00 = C00_.digif(state->C00());
    double C01 = C01_.digif(state->C01());
    double C11 = C11_.digif(state->C11());
    double chi20 = chi20_.digif(state->chi20());
    v0_.updateRangeActual(v0);
    C00_.updateRangeActual(C00);
    C01_.updateRangeActual(C01);
    C11_.updateRangeActual(C11);
    H00_.updateRangeActual(H00);
    m0_.updateRangeActual(m0);
    const double r0C = r0_.digif(m0 - x1);
    const double r0 = r0_.digif(r0C - x0 * H00);
    const double r02 = r02_.digif(r0 * r0);
    const double S00 = S00_.digif(C01  + H00 * C00);
    const double S01 = S01_.digif(C11  + H00 * C01);
    const double R00C = S01_.digif(v0 + S01);
    const double R00 = R00_.digif(R00C + H00 * S00);
    if (R00 < 0.)
      throw cms::Exception("NumericInstabillity") << "R00 got negative.";
    // dynamic cancelling
    const int msbPhi = (int)ceil(log2(R00 / R00_.base()));
    const int shiftPhi = msbPhi > setup_->kfWidthLutInvPhi() ? msbPhi - setup_->kfWidthLutInvPhi() : 0;
    const double shiftedS00 = S00_.digif(S00 / pow(2, shiftPhi));
    const double shiftedS01 = S01_.digif(S01 / pow(2, shiftPhi));
    const double shiftedR00 = R00_.digif(R00 / pow(2, shiftPhi));
    const double invR00 = invR00_.digif(1. / shiftedR00);
    const double K00 = K00_.digif(shiftedS00 * invR00);
    const double K10 = K10_.digif(shiftedS01 * invR00);
    x0 = x0_.digir(x0 + r0 * K00);
    x1 = x1_.digir(x1 + r0 * K10);
    C00 = C00_.digif(C00 - S00 * K00);
    C01 = C01_.digif(C01 - S01 * K00);
    C11 = C11_.digif(C11 - S01 * K10);
    chi20 = chi20_.digif(chi20 + r02 * invR00);
    if (C00 < 0.)
      throw cms::Exception("NumericInstabillity") << "C00 got negative.";
    if (C11 < 0.)
      throw cms::Exception("NumericInstabillity") << "C11 got negative.";
    // loose slope cut
    /*if (fabs(x0) > dataFormats_->base(Variable::qOverPt, Process::mht))
      valid0 = false;*/
    // loose intercept cut
    /*if (fabs(x1) > dataFormats_->base(Variable::phiT, Process::mht))
      valid1 = false;*/
    S00_.updateRangeActual(S00);
    S01_.updateRangeActual(S01);
    K00_.updateRangeActual(K00);
    K10_.updateRangeActual(K10);
    R00_.updateRangeActual(R00);
    invR00_.updateRangeActual(invR00);
    r0_.updateRangeActual(r0);
    r02_.updateRangeActual(r02);
    if (valid0 && valid1) {
      C00_.updateRangeActual(C00);
      C01_.updateRangeActual(C01);
      C11_.updateRangeActual(C11);
      chi20_.updateRangeActual(chi20);
      x0_.updateRangeActual(x0);
      x1_.updateRangeActual(x1);
    }
    // global
    const double chi2 = chi20 + chi21;
    // chi2 cut
    const bool valid4 = chi2 < chi2Cut;
    const bool valid = valid0 && valid1 && valid2 && valid3 && valid4;
    if ( !valid ) {
      state = nullptr;
      return;
    }
    states_.emplace_back(State(state, (initializer_list<double>){x0, x1, x2, x3, C00, C11, C22, C33, C01, C23, chi20, chi21}));
    state = &states_.back();
    if (!state->stub()) {
      chi20_.updateRangeActual(chi20);
      chi21_.updateRangeActual(chi21);
      chi2_.updateRangeActual(chi2);
    }
  }

  // remove and return first element of deque, returns nullptr if empty
  template<class T>
  T* KalmanFilter::pop_front(deque<T*>& ts) const {
    T* t = nullptr;
    if (!ts.empty()) {
      t = ts.front();
      ts.pop_front();
    }
    return t;
  }

  // remove and return first element of vector, returns nullptr if empty
  template<class T>
  T* KalmanFilter::pop_front(vector<T*>& ts) const {
    T* t = nullptr;
    if (!ts.empty()) {
      t = ts.front();
      ts.erase(ts.begin());
    }
    return t;
  }

} // namespace trackerTFP