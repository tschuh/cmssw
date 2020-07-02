#ifndef L1Trigger_TrackerTFP_GeometricProcessor_h
#define L1Trigger_TrackerTFP_GeometricProcessor_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <deque>

namespace trackerTFP {

  // Class to route Stubs of one region to one stream per sector
  class GeometricProcessor {
  public:
    GeometricProcessor(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup_, const DataFormats* dataFormats, int region);
    ~GeometricProcessor(){}

    // read in and organize input product
    void consume(const TTDTC& ttDTC);
    // fill output products
    void produce(TTDTC::Streams& accepted, TTDTC::Streams& lost);

  private:
    // remove and return first element of deque, returns nullptr if empty
    template<class T>
    T* pop_front(std::deque<T*>& ts) const;

    //
    bool enableTruncation_;
    // 
    const trackerDTC::Setup* setup_;
    //
    const DataFormats* dataFormats_;
    // 
    const int region_;
    // 
    std::vector<StubPP> stubsPP_;
    // 
    std::vector<StubGP> stubsGP_;
    // 
    std::vector<std::vector<std::deque<StubPP*>>> input_;
  };

}

#endif