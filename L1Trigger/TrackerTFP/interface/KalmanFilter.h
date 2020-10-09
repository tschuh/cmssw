#ifndef L1Trigger_TrackerTFP_KalmanFilter_h
#define L1Trigger_TrackerTFP_KalmanFilter_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"
#include "L1Trigger/TrackerTFP/interface/KalmanFilterFormats.h"
#include "L1Trigger/TrackerTFP/interface/State.h"

#include <deque>

#include <TH1F.h>

namespace trackerTFP {

  // Class to fit in a region tracks
  class KalmanFilter {
  public:
    KalmanFilter(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup, const DataFormats* dataFormats, const KalmanFilterFormats* kalmanFilterFormats, int region, std::vector<TH1F*> histos);
    ~KalmanFilter(){}

    // read in and organize input stubs
    void consume(const TTDTC::Streams& stubs);
    // read in and organize input tracks
    void consume(const StreamsTrack& tracks);
    // fill output products
    void produce(StreamTrack& accepted, StreamTrack& lost);

  private:
    // remove and return first element of deque, returns nullptr if empty
    template<class T>
    T* pop_front(std::deque<T*>& ts) const;
    // remove and return first element of vector, returns nullptr if empty
    template<class T>
    T* pop_front(std::vector<T*>& ts) const;

    // hit pattern check
    bool valid(TrackKFin* track) const;
    // adds a layer to states
    void layer(std::deque<State*>& stream);
    // repicks combinatoric stubs for state
    void comb(State*& state);
    // best state selection
    void accumulator(std::deque<State*>& stream);
    // updates state
    void update(State*& state);

    //
    bool enableTruncation_;
    // 
    const trackerDTC::Setup* setup_;
    //
    const DataFormats* dataFormats_;
    //
    const KalmanFilterFormats* kalmanFilterFormats_;
    //
    int region_;
    //
    std::vector<StubKFin> stubs_;
    //
    std::vector<TrackKFin> tracks_;
    //
    std::deque<State> states_;
    //
    std::vector<std::vector<StubKFin*>> inputStubs_;
    //
    std::vector<std::vector<TrackKFin*>> inputTracks_;
    //
    std::vector<std::vector<StubKFin*>> channelStubs_;
    //
    int layer_;
    //
    DataFormatKF x0_;
    DataFormatKF x1_;
    DataFormatKF x2_;
    DataFormatKF x3_;
    DataFormatKF H00_;
    DataFormatKF H12_;
    DataFormatKF m0_;
    DataFormatKF m1_;
    DataFormatKF v0_;
    DataFormatKF v1_;
    DataFormatKF r0_;
    DataFormatKF r1_;
    DataFormatKF r02_;
    DataFormatKF r12_;
    DataFormatKF S00_;
    DataFormatKF S01_;
    DataFormatKF S12_;
    DataFormatKF S13_;
    DataFormatKF K00_;
    DataFormatKF K10_;
    DataFormatKF K21_;
    DataFormatKF K31_;
    DataFormatKF R00_;
    DataFormatKF R11_;
    DataFormatKF invR00_;
    DataFormatKF invR11_;
    DataFormatKF chi20_;
    DataFormatKF chi21_;
    DataFormatKF C00_;
    DataFormatKF C01_;
    DataFormatKF C11_;
    DataFormatKF C22_;
    DataFormatKF C23_;
    DataFormatKF C33_;
    DataFormatKF chi2_;
    //
    std::vector<TH1F*> histos_;
  };

}

#endif