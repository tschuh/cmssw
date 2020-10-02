#ifndef L1Trigger_TrackerTFP_SeedFilter_h
#define L1Trigger_TrackerTFP_SeedFilter_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <deque>

namespace trackerTFP {

  // Class to find in a region finer rough candidates in r-z
  class SeedFilter {
  public:
    SeedFilter(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup, const DataFormats* dataFormats, int region);
    ~SeedFilter(){}

    // read in and organize input product
    void consume(const TTDTC::Streams& streams);
    // fill output products
    void produce(TTDTC::Streams& accepted, TTDTC::Streams& lost);

  private:
    //
    int layerId(StubMHT* stub) const;
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
    int region_;
    //
    DataFormat cot_;
    //
    DataFormat zT_;
    //
    DataFormat z_;
    // 
    std::vector<StubMHT> stubsMHT_;
    // 
    std::deque<StubSF> stubsSF_;
    //
    std::vector<std::vector<StubMHT*>> input_;
  };

}

#endif