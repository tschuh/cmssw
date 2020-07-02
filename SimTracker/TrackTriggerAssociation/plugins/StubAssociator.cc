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

#include "SimTracker/TrackTriggerAssociation/interface/TTTypes.h"
#include "SimTracker/TrackTriggerAssociation/interface/StubAssociation.h"
#include "L1Trigger/TrackerDTC/interface/Setup.h"

#include <vector>
#include <map>
#include <utility>
#include <set>
#include <algorithm>
#include <iterator>
#include <cmath>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace tt {

  /*! \class  ttAssociation::TTTrackingParticleAssociator
   *  \brief  Class to associate reconstrucable TrackingParticles with TTStubs
   *  \author Thomas Schuh
   *  \date   2020, Apr
   */
  class StubAssociator : public stream::EDProducer<> {
  public:
    explicit StubAssociator(const ParameterSet&);
    ~StubAssociator() override {}

  private:
    void beginRun(const Run&, const EventSetup&) override;
    void produce(Event&, const EventSetup&) override;
    void endJob() {}

    // helper classe to store configurations
    const trackerDTC::Setup* setup_;
    // ED input token of TTStubs
    EDGetTokenT<TTStubDetSetVec> getTokenTTStubDetSetVec_;
    // ED input token of TTClusterAssociation
    EDGetTokenT<TTClusterAssMap> getTokenTTClusterAssMap_;
    // ED output token for recosntructable stub association
    EDPutTokenT<StubAssociation> putTokenReconstructable_;
    // ED output token for selected stub association
    EDPutTokenT<StubAssociation> putTokenSelection_;
    // Setup token
    ESGetToken<trackerDTC::Setup, trackerDTC::SetupRcd> esGetToken_;
  };

  StubAssociator::StubAssociator(const ParameterSet& iConfig) {
    // book in- and output ed products
    getTokenTTStubDetSetVec_ = consumes<TTStubDetSetVec>(iConfig.getParameter<InputTag>("InputTagTTStubDetSetVec"));
    getTokenTTClusterAssMap_ = consumes<TTClusterAssMap>(iConfig.getParameter<InputTag>("InputTagTTClusterAssMap"));
    putTokenReconstructable_ = produces<StubAssociation>(iConfig.getParameter<string>("BranchReconstructable"));
    putTokenSelection_ = produces<StubAssociation>(iConfig.getParameter<string>("BranchSelection"));
    // book ES product
    esGetToken_ = esConsumes<trackerDTC::Setup, trackerDTC::SetupRcd, Transition::BeginRun>();
  }

  void StubAssociator::beginRun(const Run& iRun, const EventSetup& iSetup) {
    setup_ = &iSetup.getData(esGetToken_);
  }

  void StubAssociator::produce(Event& iEvent, const EventSetup& iSetup) {
    // associate TTStubs with TrackingParticles
    auto isNonnull = [](const TPPtr& tpPtr){ return tpPtr.isNonnull(); };
    Handle<TTClusterAssMap> handleTTClusterAssMap;
    iEvent.getByToken<TTClusterAssMap>(getTokenTTClusterAssMap_, handleTTClusterAssMap);
    Handle<TTStubDetSetVec> handleTTStubDetSetVec;
    iEvent.getByToken<TTStubDetSetVec>(getTokenTTStubDetSetVec_, handleTTStubDetSetVec);
    map<TPPtr, vector<TTStubRef>> mapTPPtrsTTStubRefs;
    for (TTStubDetSetVec::const_iterator ttModule = handleTTStubDetSetVec->begin();
         ttModule != handleTTStubDetSetVec->end();
         ttModule++) {
      for (TTStubDetSet::const_iterator ttStub = ttModule->begin(); ttStub != ttModule->end(); ttStub++) {
        set<TPPtr> tpPtrs;
        for (unsigned int iClus = 0; iClus < 2; iClus++) {
          const vector<TPPtr>& assocPtrs = handleTTClusterAssMap->findTrackingParticlePtrs(ttStub->clusterRef(iClus));
          copy_if(assocPtrs.begin(), assocPtrs.end(), inserter(tpPtrs, tpPtrs.begin()), isNonnull);
        }
        for (const TPPtr& tpPtr : tpPtrs)
          mapTPPtrsTTStubRefs[tpPtr].emplace_back(makeRefTo(handleTTStubDetSetVec, ttStub));
      }
    }
    // associate reconstructable TrackingParticles with TTStubs
    StubAssociation reconstructable(setup_);
    StubAssociation selection(setup_);
    for (const pair<TPPtr, vector<TTStubRef>>& p : mapTPPtrsTTStubRefs) {
      if (!setup_->useForReconstructable(*p.first) || !setup_->reconstructable(p.second))
        continue;
      reconstructable.insert(p.first, p.second);
      if (setup_->useForAlgEff(*p.first))
        selection.insert(p.first, p.second);
    }
    iEvent.emplace(putTokenReconstructable_, move(reconstructable));
    iEvent.emplace(putTokenSelection_, move(selection));
  }

} // namespace tt

DEFINE_FWK_MODULE(tt::StubAssociator);