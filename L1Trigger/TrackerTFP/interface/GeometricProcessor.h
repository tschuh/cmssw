#ifndef L1Trigger_TrackerTFP_GeometricProcessor_h
#define L1Trigger_TrackerTFP_GeometricProcessor_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"

#include <vector>
#include <deque>

namespace trackerTFP {

  // Class to route Stubs to one stream per sector
  class GeometricProcessor {
  public:
    GeometricProcessor(const edm::ParameterSet& iConfig, const trackerDTC::Setup& setup_, int region, int nStubs);
    ~GeometricProcessor(){}

    // 
    void consume(const TTDTC::Stream& stream, int channel);
    // 
    void produce(TTDTC::Streams& accepted, TTDTC::Streams& lost);

  private:
    // 
    struct Stub {
      // 
      TTStubRef ttStubRef_;
      // 
      TTBV ttBV_;
      // 
      TTBV inSector_;
      // 
      Stub(const TTDTC::Frame& frame, trackerDTC::Setup* setup_);
      // 
      TTDTC::Frame toFrame(int sector, trackerDTC::Setup* setup_);
    };
    // 
    Stub* pop_front(std::deque<Stub*>& stubs);

    //
    bool enableTruncation_;
    // 
    const trackerDTC::Setup* setup_;
    // 
    const int region_;
    // 
    std::vector<Stub> stubs_;
    // 
    std::vector<std::vector<Stub*>> input_;
    // 
    std::vector<std::deque<Stub*>> lost_;
  };

}

#endif