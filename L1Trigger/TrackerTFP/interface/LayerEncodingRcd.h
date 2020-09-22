#ifndef L1Trigger_TrackerTFP_LayerEncodingRcd_h
#define L1Trigger_TrackerTFP_LayerEncodingRcd_h

#include "FWCore/Framework/interface/DependentRecordImplementation.h"
#include "L1Trigger/TrackerTFP/interface/DataFormatsRcd.h"
#include "boost/mpl/vector.hpp"

namespace trackerTFP {

  typedef boost::mpl::vector<DataFormatsRcd> RcdsLayerEncoding;

  class LayerEncodingRcd : public edm::eventsetup::DependentRecordImplementation<LayerEncodingRcd, RcdsLayerEncoding> {};

}  // namespace trackerTFP

#endif