#ifndef L1Trigger_TrackerTFP_KalmanFilterFormatsRcd_h
#define L1Trigger_TrackerTFP_KalmanFilterFormatsRcd_h

#include "FWCore/Framework/interface/DependentRecordImplementation.h"
#include "L1Trigger/TrackerTFP/interface/DataFormatsRcd.h"
#include "boost/mpl/vector.hpp"

namespace trackerTFP {

  typedef boost::mpl::vector<DataFormatsRcd> RcdsKalmanFilterFormats;

  class KalmanFilterFormatsRcd : public edm::eventsetup::DependentRecordImplementation<KalmanFilterFormatsRcd, RcdsKalmanFilterFormats> {};

}  // namespace trackerTFP

#endif