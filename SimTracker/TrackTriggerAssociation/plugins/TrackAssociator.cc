#include "FWCore/Framework/interface/stream/EDProducer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/EDPutToken.h"
#include "DataFormats/Common/interface/Handle.h"

#include "SimTracker/TrackTriggerAssociation/interface/TrackAssociation.h"
#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"
#include "L1Trigger/TrackTrigger/interface/Setup.h"

#include <vector>
#include <map>
#include <string>
#include <utility>
#include <set>
#include <algorithm>
#include <iterator>

namespace tt {

  /*! \class  tt::TrackAssociator
   *  \brief  Class to associate reconstrucable TrackingVertices with TTTracks and vice versa
   *  \author Thomas Schuh
   *  \date   2026, June
   */
  class TrackAssociator : public edm::stream::EDProducer<> {
  public:
    explicit TrackAssociator(const edm::ParameterSet&);
    ~TrackAssociator() override = default;

  private:
    void produce(edm::Event&, const edm::EventSetup&) override;
    //
    void produce(const StubAssociation&, const std::vector<TTTrackRef>&, TrackAssociation&) const;
    // helper classe to store configurations
    const Setup* setup_;
    // ED input token of TTTracks
    edm::EDGetTokenT<TTTracks> getTokenTTTracks_;
    // ED input token of reconstructable stub association
    edm::EDGetTokenT<StubAssociation> getTokenReconstructable_;
    // ED input token of selected stub association
    edm::EDGetTokenT<StubAssociation> getTokenSelection_;
    // ED output token for recosntructable track association
    edm::EDPutTokenT<TrackAssociation> putTokenReconstructable_;
    // ED output token for selected track association
    edm::EDPutTokenT<TrackAssociation> putTokenSelection_;
    // Setup token
    edm::ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // configurstion
    TrackAssociation::Config config_;
  };

  TrackAssociator::TrackAssociator(const edm::ParameterSet& iConfig) {
    const edm::InputTag& inputTagTTTracks = iConfig.getParameter<edm::InputTag>("InputTagTTtracks");
    const std::string& prodStubAssociation = iConfig.getParameter<std::string>("InputTagStubAssociation");
    const std::string& branchReconstructable = iConfig.getParameter<std::string>("BranchReconstructable");
    const std::string& branchSelection = iConfig.getParameter<std::string>("BranchSelection");
    // book in- and output ed products
    getTokenTTTracks_ = consumes<TTTracks>(inputTagTTTracks);
    getTokenReconstructable_ = consumes<StubAssociation>(edm::InputTag(prodStubAssociation, branchReconstructable));
    getTokenSelection_ = consumes<StubAssociation>(edm::InputTag(prodStubAssociation, branchSelection));
    putTokenReconstructable_ = produces<TrackAssociation>(branchReconstructable);
    putTokenSelection_ = produces<TrackAssociation>(branchSelection);
    // book ES product
    esGetTokenSetup_ = esConsumes();
    // configuration
    config_.minTPs_ = iConfig.getParameter<int>("MinTPs");
    config_.maxR_ = iConfig.getParameter<double>("MaxR");
    config_.maxZ_ = iConfig.getParameter<double>("MaxZ");
    config_.minFrac_ = iConfig.getParameter<double>("MinFrac");
    config_.maxFrac_ = iConfig.getParameter<double>("MaxFrac");
  }

  void TrackAssociator::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    // prep input data
    const Setup* setup = &iSetup.getData(esGetTokenSetup_);
    const StubAssociation* saReconstructable = &iEvent.get(getTokenReconstructable_);
    const StubAssociation* saSelection = &iEvent.get(getTokenSelection_);
    edm::Handle<TTTracks> handleTTTracks;
    iEvent.getByToken(getTokenTTTracks_, handleTTTracks);
    std::vector<TTTrackRef> ttTrackRefs;
    ttTrackRefs.reserve(handleTTTracks->size());
    for (int i = 0; i < static_cast<int>(handleTTTracks->size()); i++)
      ttTrackRefs.emplace_back(handleTTTracks, i++);
    // produce product
    TrackAssociation taReconstructable(setup, &config_, saReconstructable, ttTrackRefs);
    TrackAssociation taSelection(setup, &config_, saSelection, ttTrackRefs);
    // store product
    iEvent.emplace(putTokenReconstructable_, std::move(taReconstructable));
    iEvent.emplace(putTokenSelection_, std::move(taSelection));
  }

}  // namespace tt

DEFINE_FWK_MODULE(tt::TrackAssociator);
