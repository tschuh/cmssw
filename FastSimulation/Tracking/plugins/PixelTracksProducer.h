#ifndef FastSimulation_Tracking_PixelTracksProducer_H
#define FastSimulation_Tracking_PixelTracksProducer_H

#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/TrajectorySeed/interface/TrajectorySeedCollection.h"

#include <string>

class PixelFitter;
class PixelTrackFilter;
class TrackingRegionProducer;

namespace edm {
  class ParameterSet;
  class Event;
  class EventSetup;
}  // namespace edm

class PixelTracksProducer : public edm::stream::EDProducer<> {
public:
  explicit PixelTracksProducer(const edm::ParameterSet& conf);

  ~PixelTracksProducer() override;

  void produce(edm::Event& ev, const edm::EventSetup& es) override;

private:
  edm::EDGetTokenT<PixelFitter> fitterToken;
  std::unique_ptr<TrackingRegionProducer> theRegionProducer;

  edm::EDGetTokenT<TrajectorySeedCollection> seedProducerToken;
  edm::EDGetTokenT<PixelTrackFilter> filterToken;
};
#endif
