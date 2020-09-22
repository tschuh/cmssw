#ifndef L1Trigger_TrackerTFP_DataFormatsRcd_h
#define L1Trigger_TrackerTFP_DataFormatsRcd_h

#include "FWCore/Framework/interface/DependentRecordImplementation.h"
#include "L1Trigger/TrackerDTC/interface/SetupRcd.h"
#include "boost/mpl/vector.hpp"

namespace trackerTFP {

  typedef boost::mpl::vector<trackerDTC::SetupRcd> RcdsDataFormats;

  class DataFormatsRcd : public edm::eventsetup::DependentRecordImplementation<DataFormatsRcd, RcdsDataFormats> {};

}  // namespace trackerTFP

#endif