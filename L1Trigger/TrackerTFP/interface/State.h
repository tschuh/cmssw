#ifndef L1Trigger_TrackerTFP_State_h
#define L1Trigger_TrackerTFP_State_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>


namespace trackerTFP {

  // 
  class State {
  public:
    //
    State(State* state);
    // proto state constructor
    State(const DataFormats* dataFormats, TrackKFin* track);
    // combinatoric state constructor
    State(State* state, StubKFin* stub);
    // updated state constructor
    State(State* state, const std::vector<double>& doubles);
    ~State(){}

    TrackKFin* track() const { return track_; }
    State* parent() const { return parent_; }
    StubKFin*  stub() const { return stub_; }
    double r() const { return stub_->r(); }
    double phi() const { return stub_->phi(); }
    double z() const { return stub_->z(); }
    int sectorPhi() const { return track_->sectorPhi(); }
    int sectorEta() const { return track_->sectorEta(); }
    const TTBV& hitPattern() const { return hitPattern_; }
    int trackId() const { return track_->trackId(); }
    const std::vector<int>& layerMap() const { return layerMap_; }
    double chi2() const { return chi20_ + chi21_; }
    bool barrel() const { return setup_->barrel(stub_->ttStubRef()); }
    bool psModule() const { return setup_->psModule(stub_->ttStubRef()); }
    int layer() const { return stub_->layer(); }
    double quali() const { return (chi20_ / 8. + chi21_) * pow(2, (hitPattern_.count(0, hitPattern_.pmEncode(), false))); }
    double x0() const { return x0_; }
    double x1() const { return x1_; }
    double x2() const { return x2_; }
    double x3() const { return x3_; }
    double C00() const { return C00_; }
    double C01() const { return C01_; }
    double C11() const { return C11_; }
    double C22() const { return C22_; }
    double C23() const { return C23_; }
    double C33() const { return C33_; }
    double chi20() const { return chi20_; }
    double chi21() const { return chi21_; }
    double H12() const { return r() + setup_->chosenRofPhi() - setup_->chosenRofZ(); }
    double H00() const { return -r(); }
    double m0() const { return stub_->phi(); }
    double m1() const { return stub_->z(); }
    double v0() const { return setup_->v0(stub_->ttStubRef(), track_->qOverPt()); }
    //double v1() const { return setup_->v1(stub_->ttStubRef(), track_->cot()); }
    double v1() const;
    FrameTrack frame() const;

  private:
    //
    const DataFormats* dataFormats_;
    //
    const trackerDTC::Setup* setup_;
    // found mht track
    TrackKFin* track_;
    // previous state, nullptr for first states
    State* parent_;
    // stub to add
    StubKFin* stub_;
    // shows which stub on each layer has been added so far
    std::vector<int> layerMap_;
    // shows which layer has been added so far
    TTBV hitPattern_;
    double x0_;
    double x1_;
    double x2_;
    double x3_;
    double C00_;
    double C01_;
    double C11_;
    double C22_;
    double C23_;
    double C33_;
    double chi20_;
    double chi21_;
  };

}

#endif