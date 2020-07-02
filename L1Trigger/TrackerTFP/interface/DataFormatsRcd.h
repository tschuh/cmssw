#ifndef L1Trigger_TrackerTFP_DataFormatsRcd_h
#define L1Trigger_TrackerTFP_DataFormatsRcd_h

#include "FWCore/Framework/interface/DependentRecordImplementation.h"
#include "L1Trigger/TrackerDTC/interface/SetupRcd.h"
#include "boost/mpl/vector.hpp"

using namespace trackerDTC;

namespace trackerTFP {

  typedef boost::mpl::vector<SetupRcd> Rcds;

  class DataFormatsRcd : public edm::eventsetup::DependentRecordImplementation<DataFormatsRcd, Rcds> {};

}  // namespace trackerTFP

#endif