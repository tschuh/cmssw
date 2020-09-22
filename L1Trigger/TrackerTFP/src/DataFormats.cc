#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <deque>
#include <cmath>
#include <tuple>
#include <iterator>
#include <algorithm>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  DataFormats::DataFormats() :
    numDataFormats_(0),
    formats_(+Variable::end, std::vector<DataFormat*>(+Process::end, nullptr)),
    numUnusedBits_(+Process::end, TTBV::S),
    numChannel_(+Process::end, 0)
  {
    setup_ = nullptr;
    countFormats();
    dataFormats_.reserve(numDataFormats_);
    numStreams_.reserve(+Process::end);
  }

  template<Variable v = Variable::begin, Process p = Process::begin>
  void DataFormats::countFormats() {
    if constexpr(config_[+v][+p] == p)
      numDataFormats_++;
    if constexpr(++p != Process::end)
      countFormats<v, ++p>();
    else if constexpr(++v != Variable::end)
      countFormats<++v>();
  }

  DataFormats::DataFormats(const ParameterSet& iConfig, const Setup* setup) : DataFormats() {
    iConfig_ = iConfig;
    setup_ = setup;
    fillDataFormats();
    for (const Process p : Processes)
      for (const Variable v : stubs_[+p])
        numUnusedBits_[+p] -= formats_[+v][+p] ? formats_[+v][+p]->width() : 0;
    numChannel_[+Process::dtc] = setup_->numDTCsPerRegion();
    numChannel_[+Process::pp] = setup_->numDTCsPerTFP();
    numChannel_[+Process::gp] = setup_->numSectors();
    numChannel_[+Process::ht] = setup_->htNumBinsQoverPt();
    numChannel_[+Process::mht] = setup_->htNumBinsQoverPt();
    numChannel_[+Process::sf] = setup_->htNumBinsQoverPt();
    numChannel_[+Process::kfin] = setup_->numSectorsPhi() * setup_->numLayers();
    numChannel_[+Process::kf] = setup_->kfNumWorker();
    transform(numChannel_.begin(), numChannel_.end(), back_inserter(numStreams_), [this](int channel){ return channel * setup_->numRegions(); });
  }

  template<Variable v = Variable::begin, Process p = Process::begin>
  void DataFormats::fillDataFormats() {
    if constexpr(config_[+v][+p] == p) {
      dataFormats_.emplace_back(Format<v, p>(iConfig_, setup_));
      fillFormats<v, p>();
    }
    if constexpr(++p != Process::end)
      fillDataFormats<v, ++p>();
    else if constexpr(++v != Variable::end)
      fillDataFormats<++v>();
  }

  template<Variable v, Process p, Process it = Process::begin>
  void DataFormats::fillFormats() {
    if (config_[+v][+it] == p) {
      formats_[+v][+it] = &dataFormats_.back();
    }
    if constexpr(++it != Process::end)
      fillFormats<v, p, ++it>();
  }

  template<typename ...Ts>
  void DataFormats::convert(const TTDTC::BV& bv, tuple<Ts...>& data, Process p) const {
    TTBV ttBV(bv);
    extract(ttBV, data, p);
  }

  template<int it = 0, typename ...Ts>
  void DataFormats::extract(TTBV& ttBV, std::tuple<Ts...>& data, Process p) const {
    Variable v = *next(stubs_[+p].begin(), sizeof...(Ts) - 1 - it);
    formats_[+v][+p]->extract(ttBV, get<sizeof...(Ts) - 1 - it>(data));
    if constexpr(it + 1 != sizeof...(Ts))
      extract<it + 1>(ttBV, data, p);
  }

  template<typename... Ts>
  void DataFormats::convert(const std::tuple<Ts...>& data, TTDTC::BV& bv, Process p) const {
    TTBV ttBV(1, TTBV::S);
    attach(data, ttBV, p);
    bv = ttBV.bs();
  }

  template<int it = 0, typename... Ts>
  void DataFormats::attach(const tuple<Ts...>& data, TTBV& ttBV, Process p) const {
    Variable v = *next(stubs_[+p].begin(), it);
    formats_[+v][+p]->attach(get<it>(data), ttBV);
    if constexpr(it + 1 != sizeof...(Ts))
      attach<it + 1>(data, ttBV, p);
  }

  template<typename ...Ts>
  Stub<Ts...>::Stub(const TTDTC::Frame& frame, const DataFormats* dataFormats, Process p) :
    dataFormats_(dataFormats),
    p_(p),
    frame_(frame),
    trackId_(0)
  {
    dataFormats_->convert(frame.second, data_, p_);
  }

  template<typename ...Ts>
  template<typename ...Others>
  Stub<Ts...>::Stub(const Stub<Others...>& stub, Ts... data) :
    dataFormats_(stub.dataFormats()),
    p_(++stub.p()),
    frame_(stub.frame().first, TTDTC::BV()),
    data_(data...),
    trackId_(0)
  {
  }

  StubPP::StubPP(const TTDTC::Frame& frame, const DataFormats* formats) :
    Stub(frame, formats, Process::pp)
  {
    for(int sectorEta = sectorEtaMin(); sectorEta <= sectorEtaMax(); sectorEta++)
      for(int sectorPhi = 0; sectorPhi < width(Variable::sectorsPhi); sectorPhi++)
        sectors_[sectorEta * width(Variable::sectorsPhi) + sectorPhi] = sectorsPhi()[sectorPhi];
  }

  StubGP::StubGP(const TTDTC::Frame& frame, const DataFormats* formats, int sectorPhi, int sectorEta) :
    Stub(frame, formats, Process::gp), sectorPhi_(sectorPhi), sectorEta_(sectorEta)
  {
    const Setup* setup = dataFormats_->setup();
    qOverPtBins_ = TTBV(0, setup->htNumBinsQoverPt());
    for (int qOverPt = qOverPtMin(); qOverPt <= qOverPtMax(); qOverPt++)
      qOverPtBins_.set(qOverPt + qOverPtBins_.size() / 2);
  }

  StubGP::StubGP(const StubPP& stub, int sectorPhi, int sectorEta) :
    Stub(stub, stub.r(), stub.phi(), stub.z(), stub.layer(), stub.qOverPtMin(), stub.qOverPtMax()),
    sectorPhi_(sectorPhi),
    sectorEta_(sectorEta)
  {
    const Setup* setup = dataFormats_->setup();
    get<1>(data_) -= (sectorPhi_ - .5) * setup->baseSector();
    get<2>(data_) -= (r() + setup->chosenRofPhi()) * setup->sectorCot(sectorEta_);
    dataFormats_->convert(data_, frame_.second, p_);
  }

  StubHT::StubHT(const TTDTC::Frame& frame, const DataFormats* formats, int qOverPt) :
    Stub(frame, formats, Process::ht), qOverPt_(qOverPt)
  {
    fillTrackId();
  }

  StubHT::StubHT(const StubGP& stub, int phiT, int qOverPt) :
    Stub(stub, stub.r(), stub.phi(), stub.z(), stub.layer(), stub.sectorPhi(), stub.sectorEta(), phiT),
    qOverPt_(qOverPt)
  {
    get<1>(data_) += format(Variable::qOverPt).floating(this->qOverPt()) * r() - format(Variable::phiT).floating(this->phiT());
    fillTrackId();
    dataFormats_->convert(data_, frame_.second, p_);
  }

  void StubHT::fillTrackId() {
    TTBV ttBV(bv());
    trackId_ = ttBV.extract(width(Variable::sectorPhi) + width(Variable::sectorEta) + width(Variable::phiT));
  }

  StubMHT::StubMHT(const TTDTC::Frame& frame, const DataFormats* formats) :
    Stub(frame, formats, Process::mht)
  {
    fillTrackId();
  }

  StubMHT::StubMHT(const StubHT& stub, int phiT, int qOverPt) :
    Stub(stub, stub.r(), stub.phi(), stub.z(), stub.layer(), stub.sectorPhi(), stub.sectorEta(), stub.phiT(), stub.qOverPt())
  {
    const Setup* setup = dataFormats_->setup();
    get<6>(data_) = this->phiT() * setup->mhtNumBinsPhiT() + phiT;
    get<7>(data_) = this->qOverPt() * setup->mhtNumBinsQoverPt() + qOverPt;
    get<1>(data_) += base(Variable::qOverPt) * (qOverPt - .5) * r() - base(Variable::phiT) * (phiT - .5);
    dataFormats_->convert(data_, frame_.second, p_);
    fillTrackId();
  }

  void StubMHT::fillTrackId() {
    TTBV ttBV(bv());
    trackId_ = ttBV.extract(width(Variable::sectorPhi) + width(Variable::sectorEta) + width(Variable::phiT) + width(Variable::qOverPt));
  }

  StubSF::StubSF(const TTDTC::Frame& frame, const DataFormats* formats) :
    Stub(frame, formats, Process::sf)
  {
    fillTrackId();
  }

  StubSF::StubSF(const StubMHT& stub, int z0, int cot) :
    Stub(stub, stub.r(), stub.phi(), stub.z(), stub.layer(), stub.sectorPhi(), stub.sectorEta(), stub.phiT(), stub.qOverPt(), z0, cot)
  {
    dataFormats_->convert(data_, frame_.second, p_);
    fillTrackId();
  }

  void StubSF::fillTrackId() {
    TTBV ttBV(bv());
    trackId_ = ttBV.extract(width(Variable::sectorPhi) + width(Variable::sectorEta) + width(Variable::phiT) + width(Variable::qOverPt) + width(Variable::z0) + width(Variable::cot));
  }

  StubKFin::StubKFin(const TTDTC::Frame& frame, const DataFormats* formats, int layer) :
    Stub(frame, formats, Process::kfin),
    layer_(layer)
  {
    trackId_ = trackId();
  }

  StubKFin::StubKFin(const StubSF& stub, int layer, int trackId) :
    Stub(stub, stub.r(), stub.phi(), stub.z(), trackId),
    layer_(layer)
  {
    dataFormats_->convert(data_, frame_.second, p_);
    trackId_ = trackId;
  }

  template<typename ...Ts>
  Track<Ts...>::Track(const FrameTrack& frame, const DataFormats* dataFormats, Process p) :
    dataFormats_(dataFormats),
    p_(p),
    frame_(frame)
  {
    dataFormats_->convert(frame.second, data_, p_);
  }

  template<typename ...Ts>
  template<typename ...Others>
  Track<Ts...>::Track(const Track<Others...>& track, Ts... data) :
    dataFormats_(track.dataFormats()),
    p_(++track.p()),
    frame_(track.frame().first, TTDTC::BV()),
    data_(data...) {}

  template<typename ...Ts>
  template<typename ...Others>
  Track<Ts...>::Track(const Stub<Others...>& stub, const TTTrackRef& ttTrackRef, Ts... data) :
    dataFormats_(stub.dataFormats()),
    p_(++stub.p()),
    frame_(ttTrackRef, TTDTC::BV()),
    data_(data...) {}

  TrackKFin::TrackKFin(const FrameTrack& frame, const DataFormats* dataFormats, const vector<StubKFin*>& stubs) :
    Track(frame, dataFormats, Process::kfin),
    stubs_(setup()->numLayers())
  {
    deque<StubKFin*> myStubs;
    for (StubKFin* stub : stubs)
      if (stub->trackId() == trackId())
        myStubs.push_back(stub);
    vector<int> nStubs(stubs_.size(), 0);
    for (StubKFin* stub : myStubs)
      nStubs[stub->layer()]++;
    for (int layer = 0; layer < dataFormats->setup()->numLayers(); layer++)
      stubs_[layer].reserve(nStubs[layer]);
    for (StubKFin* stub : myStubs)
      stubs_[stub->layer()].push_back(stub);
  }

  TrackKFin::TrackKFin(const StubSF& stub, const TTTrackRef& ttTrackRef, const TTBV& hitPattern, const TTBV& layerMap) :
    Track(stub, ttTrackRef, hitPattern, layerMap, stub.phiT(), stub.qOverPt(), stub.z0(), stub.cot(), stub.sectorPhi(), stub.sectorEta(), ttTrackRef->hitPattern()),
    stubs_(setup()->numLayers())
  {
    dataFormats_->convert(data_, frame_.second, p_);
  }

  TrackKF::TrackKF(const FrameTrack& frame, const DataFormats* dataFormats, const vector<TTStubRef>& ttStubRefs) :
    Track(frame, dataFormats, Process::kf),
    ttStubRefs_(ttStubRefs) {}

  TrackKF::TrackKF(const TrackKFin& track, double phiT, double qOverPt, double z0, double cot, const TTBV& hitPattern, const TTBV& layerMap, const vector<TTStubRef>& ttStubRefs) :
    Track(track, hitPattern, layerMap, 0., 0., 0., 0., track.sectorPhi(), track.sectorEta(), false),
    ttStubRefs_(ttStubRefs)
  {
    get<2>(data_) = track.phiT() + phiT;
    get<3>(data_) = track.qOverPt() + qOverPt;
    get<4>(data_) = track.z0() + z0;
    get<5>(data_) = track.cot() + cot;
    get<8>(data_) = abs(qOverPt) < dataFormats_->format(Variable::qOverPt, Process::sf).base() / 2. && abs(phiT) < dataFormats_->format(Variable::phiT, Process::sf).base() / 2.;
    dataFormats_->convert(data_, frame_.second, p_);
  }

  TTTrack<Ref_Phase2TrackerDigi_> TrackKF::ttTrack() const {
    const double qOverPt = this->qOverPt();
    const double phi0 = this->phiT() + qOverPt * setup()->chosenRofPhi() + dataFormats_->format(Variable::phi, Process::gp).range() * (this->sectorPhi() - .5);
    const double cot = this->cot() + setup()->sectorCot(this->sectorEta());
    const double z0 = this->z0();
    const double chi2phi = 0.;
    const double chi2z = 0;
    const double trkMVA1 = 0.;
    const double trkMVA2 = 0.;
    const double trkMVA3 = 0.;
    const int hitPattern = this->hitPattern().val();
    const int nPar = 4;
    const double bField = 0.;
    const int sectorPhi = frame_.first->phiSector();
    const int sectorEta = frame_.first->etaSector();
    TTTrack<Ref_Phase2TrackerDigi_> ttTrack(qOverPt, phi0, cot, z0, chi2phi, chi2z, trkMVA1, trkMVA2, trkMVA3, hitPattern, nPar, bField);
    ttTrack.setStubRefs(ttStubRefs_);
    ttTrack.setPhiSector(sectorPhi);
    ttTrack.setEtaSector(sectorEta);
    return ttTrack;
  }

  TrackDR::TrackDR(const FrameTrack& frame, const DataFormats* dataFormats, const vector<TTStubRef>& ttStubRefs) :
    Track(frame, dataFormats, Process::dr),
    ttStubRefs_(ttStubRefs) {}

  TrackDR::TrackDR(const TrackKF& track) :
    Track(track, track.hitPattern(), setup()->layerMap(track.layerMap()), 0., 0., 0., 0.),
    ttStubRefs_(track.ttStubRefs())
  {
    get<2>(data_) = track.phiT() + track.qOverPt() * setup()->chosenRofPhi() + dataFormats_->format(Variable::phi, Process::gp).range() * (track.sectorPhi() - .5);
    get<3>(data_) = track.qOverPt();
    get<4>(data_) = track.z0();
    get<5>(data_) = track.cot() + setup()->sectorCot(track.sectorEta());
    dataFormats_->convert(data_, frame_.second, p_);
  }

  TTTrack<Ref_Phase2TrackerDigi_> TrackDR::ttTrack() const {
    const double qOverPt = this->qOverPt();
    const double phi0 = this->phi0();
    const double cot = this->cot();
    const double z0 = this->z0();
    const double chi2phi = 0.;
    const double chi2z = 0;
    const double trkMVA1 = 0.;
    const double trkMVA2 = 0.;
    const double trkMVA3 = 0.;
    const int hitPattern = this->hitPattern().val();
    const int nPar = 4;
    const double bField = 0.;
    const int sectorPhi = frame_.first->phiSector();
    const int sectorEta = frame_.first->etaSector();
    TTTrack<Ref_Phase2TrackerDigi_> ttTrack(qOverPt, phi0, cot, z0, chi2phi, chi2z, trkMVA1, trkMVA2, trkMVA3, hitPattern, nPar, bField);
    ttTrack.setStubRefs(ttStubRefs_);
    ttTrack.setPhiSector(sectorPhi);
    ttTrack.setEtaSector(sectorEta);
    return ttTrack;
  }

  template<>
  Format<Variable::phiT, Process::ht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    range_ = 2. * M_PI / (double)(setup->numRegions() * setup->numSectorsPhi());
    base_ = range_ / (double)setup->htNumBinsPhiT();
    width_ = ceil(log2(setup->htNumBinsPhiT()));
  }

  template<>
  Format<Variable::phiT, Process::mht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> ht(iConfig, setup);
    range_ = ht.range();
    base_ = ht.base() / setup->mhtNumBinsPhiT();
    width_ = ceil(log2(setup->htNumBinsPhiT() * setup->mhtNumBinsPhiT()));
  }

  template<>
  Format<Variable::qOverPt, Process::ht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    range_ = 2. * setup->invPtToDphi() / setup->minPt();
    base_ = range_ / (double)setup->htNumBinsQoverPt();
    width_ = ceil(log2(setup->htNumBinsQoverPt()));
  }

  template<>
  Format<Variable::qOverPt, Process::mht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::qOverPt, Process::ht> ht(iConfig, setup);
    range_ = ht.range();
    base_ = ht.base() / setup->mhtNumBinsQoverPt();
    width_ = ceil(log2(setup->htNumBinsQoverPt() * setup->mhtNumBinsQoverPt()));
  }

  template<>
  Format<Variable::r, Process::ht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    width_ = setup->widthR();
    range_ = 2. * max(abs(setup->outerRadius() - setup->chosenRofPhi()), abs(setup->innerRadius() - setup->chosenRofPhi()));
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(iConfig, setup);
    base_ = phiT.base() / qOverPt.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::phi, Process::gp>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    width_ = setup->widthPhi();
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(iConfig, setup);
    const Format<Variable::r, Process::ht> r(iConfig, setup);
    range_ = phiT.range() + qOverPt.range() * r.base() * pow(2., r.width()) / 4.;
    const int shift = ceil(log2(range_ / phiT.base() / pow(2., width_)));
    base_ = phiT.base() * pow(2., shift);
  }

  template<>
  Format<Variable::phi, Process::dtc>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(iConfig, setup);
    const Format<Variable::r, Process::ht> r(iConfig, setup);
    range_ = 2. * M_PI / (double)setup->numRegions() + qOverPt.range() * r.base() * pow(2., r.width()) / 4.;
    const Format<Variable::phi, Process::gp> gp(iConfig, setup);
    base_ = gp.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::phi, Process::ht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    range_ = 2. * phiT.base();
    const Format<Variable::phi, Process::gp> gp(iConfig, setup);
    base_ = gp.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::phi, Process::mht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::mht> phiT(iConfig, setup);
    range_ = 2. * phiT.base();
    const Format<Variable::phi, Process::ht> ht(iConfig, setup);
    base_ = ht.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::z, Process::dtc>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    width_ = setup->widthZ();
    range_ = 2. * setup->halfLength();
    const Format<Variable::r, Process::ht> r(iConfig, setup);
    const int shift = ceil(log2(range_ / r.base() / pow(2., width_)));
    base_ = r.base() * pow(2., shift);
  }

  template<>
  Format<Variable::z, Process::gp>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    range_ = setup->neededRangeChiZ();
    const Format<Variable::z, Process::dtc> dtc(iConfig, setup);
    base_ = dtc.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::z0, Process::sf>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    width_ = iConfig.getParameter<ParameterSet>("SeedFilter").getParameter<int>("WidthZ0");
    range_ = 2. * setup->beamWindowZ();
    const Format<Variable::z, Process::dtc> z(iConfig, setup);
    const int shift = ceil(log2(range_ / z.base() / pow(2., width_)));
    base_ = z.base() * pow(2., shift);
  }

  template<>
  Format<Variable::cot, Process::sf>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    width_ = iConfig.getParameter<ParameterSet>("SeedFilter").getParameter<int>("WidthCot");
    range_ = -1.;
    for (int eta = 0; eta < setup->numSectorsEta(); eta++)
      range_ = max(range_, (sinh(setup->boundarieEta(eta + 1)) - sinh(setup->boundarieEta(eta))));
    const int shift = ceil(log2(range_)) - width_;
    base_ = pow(2., shift);
  }

  template<>
  Format<Variable::z, Process::sf>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(true) {
    const Format<Variable::z0, Process::sf> z0(iConfig, setup);
    const Format<Variable::cot, Process::sf> cot(iConfig, setup);
    range_ = z0.base() + cot.range() * setup->outerRadius() + 2. * setup->maxLength();
    const Format<Variable::z, Process::dtc> dtc(iConfig, setup);
    base_ = dtc.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::layer, Process::ht>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(false) {
    range_ = setup->numLayers();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorEta, Process::gp>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsEta();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorPhi, Process::gp>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsPhi();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorsPhi, Process::dtc>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsPhi();
    width_ = setup->numSectorsPhi();
  }

  template<>
  Format<Variable::trackId, Process::kfin>::Format(const ParameterSet& iConfig, const Setup* setup) : DataFormat(false) {
    range_ = setup->htNumBinsQoverPt() * setup->sfMaxTracks();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::match, Process::kf>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(false) {
    width_ = 1;
    range_ = 1.;
  }

  template<>
  Format<Variable::hitPattern, Process::kf>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(false) {
    range_ = setup->numLayers();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::layerMap, Process::kf>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(false) {
    range_ = setup->numLayers() * ceil(log2(setup->kfMaxStubsPerLayer()));
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::phi0, Process::dr>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::qOverPt, Process::ht> qOverPt(iConfig, setup);
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    width_ = iConfig.getParameter<ParameterSet>("DuplicateRemoval").getParameter<int>("WidthPhi0");
    range_ = 2. * M_PI / (double)setup->numRegions() + qOverPt.range() * setup->chosenRofPhi();
    base_ = phiT.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::qOverPt, Process::dr>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::qOverPt, Process::ht> qOverPt(iConfig, setup);
    width_ = iConfig.getParameter<ParameterSet>("DuplicateRemoval").getParameter<int>("WidthQoverPt");
    range_ = qOverPt.range();
    base_ = qOverPt.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::z0, Process::dr>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::z0, Process::sf> z0(iConfig, setup);
    width_ = iConfig.getParameter<ParameterSet>("DuplicateRemoval").getParameter<int>("WidthZ0");
    range_ = z0.range();
    base_ = z0.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::cot, Process::dr>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::cot, Process::sf> cot(iConfig, setup);
    width_ = iConfig.getParameter<ParameterSet>("DuplicateRemoval").getParameter<int>("WidthCot");
    range_ = cot.range();
    base_ = cot.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::phiT, Process::kf>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::phi0, Process::dr> phi0(iConfig, setup);
    const Format<Variable::phiT, Process::ht> phiT(iConfig, setup);
    range_ = phiT.range();
    base_ = phi0.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::cot, Process::kf>::Format(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup) : DataFormat(true) {
    const Format<Variable::cot, Process::dr> dr(iConfig, setup);
    const Format<Variable::cot, Process::sf> sf(iConfig, setup);
    range_ = sf.range();
    base_ = dr.base();
    width_ = ceil(log2(range_ / base_));
  }

} // namespace trackerTFP