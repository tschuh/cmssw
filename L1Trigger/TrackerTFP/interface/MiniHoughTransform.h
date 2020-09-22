#ifndef L1Trigger_TrackerTFP_MiniHoughTransform_h
#define L1Trigger_TrackerTFP_MiniHoughTransform_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <set>
#include <deque>

namespace trackerTFP {

  // Class to find in a region finer rough candidates in r-phi
  class MiniHoughTransform {
  public:
    MiniHoughTransform(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup, const DataFormats* dataFormats, int region);
    ~MiniHoughTransform(){}

    // read in and organize input product
    void consume(const TTDTC::Streams& streams);
    // fill output products
    void produce(TTDTC::Streams& accepted, TTDTC::Streams& lost);

  private:
    // remove and return first element of deque, returns nullptr if empty
    template<class T>
    T* pop_front(std::deque<T*>& ts) const;
    //
    void fill(const std::vector<StubHT*>& input, std::vector<std::deque<StubMHT*>>& output, int channel);
    //
    void slb(std::vector<std::deque<StubMHT*>>& inputs, std::vector<StubMHT*>& accepted, TTDTC::Stream& lost) const;
    //
    void dlb(std::vector<std::vector<StubMHT*>>& streams) const;

    //
    bool enableTruncation_;
    // 
    const trackerDTC::Setup* setup_;
    //
    const DataFormats* dataFormats_;
    //
    DataFormat qOverPt_;
    //
    DataFormat phiT_;
    //
    int region_;
    //
    int numBinsQoverPt_;
    //
    int numCells_;
    //
    int numNodes_;
    //
    int numChannel_;
    // 
    std::vector<StubHT> stubsHT_;
    // 
    std::vector<StubMHT> stubsMHT_;
    //
    std::vector<std::vector<StubHT*>> input_;
    //
    std::vector<StubMHT*> test_;
  };

}

#endif