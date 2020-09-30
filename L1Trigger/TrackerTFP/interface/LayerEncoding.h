#ifndef L1Trigger_TrackerTFP_LayerEncoding_h
#define L1Trigger_TrackerTFP_LayerEncoding_h

#include "FWCore/Framework/interface/data_default_record_trait.h"
#include "L1Trigger/TrackerTFP/interface/LayerEncodingRcd.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>

namespace trackerTFP {

  class LayerEncoding {
  public:
    LayerEncoding() {}
    LayerEncoding(const DataFormats* dataFormats);
    ~LayerEncoding(){}
    const std::vector<int>& layerEncoding(int binEta, int binZ0, int binCot) const { return layerEncoding_.at(binEta).at(binZ0).at(binCot); }
    const std::vector<int>& maybeLayer(int binEta, int binZ0, int binCot) const { return maybeLayer_.at(binEta).at(binZ0).at(binCot); }
    const int layerIdKF(int binEta, int binZ0, int binCot, int layerId) const;
    //
    TTBV hitPattern(const std::vector<TTStubRef>& ttStubRefs, int binEta, int binZ0, int binCot) const;
    //
    void addTTStubRefs(TrackKF& track) const;
  private:
    const DataFormats* dataFormats_;
    const trackerDTC::Setup* setup_;
    const DataFormat* z0_;
    const DataFormat* cot_;
    std::vector<std::vector<std::vector<std::vector<int>>>> layerEncoding_;
    std::vector<std::vector<std::vector<std::vector<int>>>> maybeLayer_;
  };

} // namespace trackerTFP

EVENTSETUP_DATA_DEFAULT_RECORD(trackerTFP::LayerEncoding, trackerTFP::LayerEncodingRcd);

#endif