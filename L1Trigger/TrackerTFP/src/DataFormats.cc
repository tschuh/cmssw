#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <cmath>
#include <tuple>
#include <iterator>
#include <algorithm>

using namespace std;
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

  DataFormats::DataFormats(const Setup* setup) : DataFormats() {
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
    numChannel_[+Process::lr] = 2 * setup_->htNumBinsQoverPt();
    transform(numChannel_.begin(), numChannel_.end(), back_inserter(numStreams_), [this](int channel){ return channel * setup_->numRegions(); });
  }

  template<Variable v = Variable::begin, Process p = Process::begin>
  void DataFormats::fillDataFormats() {
    if constexpr(config_[+v][+p] == p) {
      dataFormats_.emplace_back(Format<v, p>(setup_));
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
    TTBV ttBV;
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
    //dataFormats_->convert(data_, frame_.second, p_);
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
    Stub(stub, 0, 0, stub.r(), stub.phi(), stub.z(), stub.layer(), stub.sectorPhi(), stub.sectorEta(), stub.phiT(), stub.qOverPt())
  {
    const Setup* setup = dataFormats_->setup();
    get<8>(data_) = this->phiT() * setup->mhtNumBinsPhiT() + phiT;
    get<9>(data_) = this->qOverPt() * setup->mhtNumBinsQoverPt() + qOverPt;
    get<3>(data_) += base(Variable::qOverPt) * (qOverPt - .5) * r() - base(Variable::phiT) * (phiT - .5);
    get<0>(data_) = setup->barrel(stub.ttStubRef());
    get<1>(data_) = setup->psModule(stub.ttStubRef());
    dataFormats_->convert(data_, frame_.second, p_);
    fillTrackId();
  }

  void StubMHT::fillTrackId() {
    TTBV ttBV(bv());
    trackId_ = ttBV.extract(width(Variable::sectorPhi) + width(Variable::sectorEta) + width(Variable::phiT) + width(Variable::qOverPt));
  }

  StubLR::StubLR(const TTDTC::Frame& frame, const DataFormats* formats) :
    Stub(frame, formats, Process::lr) {}

  StubLR::StubLR(const StubMHT& stub, double phiT, double qOverPt, double zT, double cot) :
    Stub(stub, phiT, qOverPt, zT, cot) { dataFormats_->convert(data_, frame_.second, p_); }

  template<>
  Format<Variable::phiT, Process::ht>::Format(const Setup* setup) : DataFormat(true) {
    range_ = 2. * M_PI / (double)(setup->numRegions() * setup->numSectorsPhi());
    base_ = range() / (double)setup->htNumBinsPhiT();
    width_ = ceil(log2(setup->htNumBinsPhiT()));
  }

  template<>
  Format<Variable::phiT, Process::mht>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> ht(setup);
    range_ = ht.range();
    base_ = ht.base() / setup->mhtNumBinsPhiT();
    width_ = ceil(log2(setup->htNumBinsPhiT() * setup->mhtNumBinsPhiT()));
  }

  template<>
  Format<Variable::qOverPt, Process::ht>::Format(const Setup* setup) : DataFormat(true) {
    range_ = 2. * setup->invPtToDphi() / setup->minPt();
    base_ = range_ / (double)setup->htNumBinsQoverPt();
    width_ = ceil(log2(setup->htNumBinsQoverPt()));
  }

  template<>
  Format<Variable::qOverPt, Process::mht>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::qOverPt, Process::ht> ht(setup);
    range_ = ht.range();
    base_ = ht.base() / setup->mhtNumBinsQoverPt();
    width_ = ceil(log2(setup->htNumBinsQoverPt() * setup->mhtNumBinsQoverPt()));
  }

  template<>
  Format<Variable::r, Process::ht>::Format(const Setup* setup) : DataFormat(true) {
    width_ = setup->widthR();
    range_ = 2. * max(abs(setup->outerRadius() - setup->chosenRofPhi()), abs(setup->innerRadius() - setup->chosenRofPhi()));
    const Format<Variable::phiT, Process::ht> phiT(setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(setup);
    base_ = phiT.base() / qOverPt.base();
    const int shift = ceil(log2(range_ / base_ / pow(2., width_)));
    base_ *= pow(2., shift);
  }

  template<>
  Format<Variable::phi, Process::gp>::Format(const Setup* setup) : DataFormat(true) {
    width_ = setup->widthPhi();
    const Format<Variable::phiT, Process::ht> phiT(setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(setup);
    const Format<Variable::r, Process::ht> r(setup);
    range_ = phiT.range() + qOverPt.range() * r.base() * pow(2., r.width()) / 4.;
    base_ = range_ * pow(2, -width_);
    const int shift = ceil(log2(range_ / phiT.base() / pow(2., width_)));
    base_ = phiT.base() * pow(2., shift);
  }

  template<>
  Format<Variable::phi, Process::dtc>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> phiT(setup);
    const Format<Variable::qOverPt, Process::ht> qOverPt(setup);
    const Format<Variable::r, Process::ht> r(setup);
    range_ = 2. * M_PI / (double)setup->numRegions() + qOverPt.range() * r.base() * pow(2., r.width()) / 4.;
    const Format<Variable::phi, Process::gp> gp(setup);
    base_ = gp.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::phi, Process::ht>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::ht> phiT(setup);
    range_ = 2. * phiT.base();
    const Format<Variable::phi, Process::gp> gp(setup);
    base_ = gp.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::phi, Process::mht>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::mht> phiT(setup);
    range_ = 2. * phiT.base();
    const Format<Variable::phi, Process::ht> ht(setup);
    base_ = ht.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::z, Process::dtc>::Format(const Setup* setup) : DataFormat(true) {
    width_ = setup->widthZ();
    range_ = 2. * setup->halfLength();
    const Format<Variable::r, Process::ht> r(setup);
    const int shift = ceil(log2(range_ / r.base() / pow(2., width_)));
    base_ = r.base() * pow(2., shift);
  }

  template<>
  Format<Variable::z, Process::gp>::Format(const Setup* setup) : DataFormat(true) {
    range_ = setup->neededRangeChiZ();
    const Format<Variable::z, Process::dtc> dtc(setup);
    base_ = dtc.base();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::layer, Process::ht>::Format(const Setup* setup) : DataFormat(false) {
    range_ = setup->numLayers();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorEta, Process::gp>::Format(const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsEta();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorPhi, Process::gp>::Format(const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsPhi();
    width_ = ceil(log2(range_));
  }

  template<>
  Format<Variable::sectorsPhi, Process::dtc>::Format(const Setup* setup) : DataFormat(false) {
    range_ = setup->numSectorsPhi();
    width_ = setup->numSectorsPhi();
  }

  template<>
  Format<Variable::phiT, Process::lr>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::phiT, Process::mht> mht(setup);
    range_ = mht.range() * 4.;
    base_ = mht.base() * pow(2, setup->lrBaseDiffPhiT());
    width_ = mht.width() + 1 - setup->lrBaseDiffPhiT();
  }

  template<>
  Format<Variable::qOverPt, Process::lr>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::qOverPt, Process::mht> mht(setup);
    range_ = mht.range() * 4.;
    base_ = mht.base() * pow(2, setup->lrBaseDiffQoverPt());
    width_ = mht.width() + 1 - setup->lrBaseDiffQoverPt();
  }

  template<>
  Format<Variable::zT, Process::lr>::Format(const Setup* setup) : DataFormat(true) {
    const Format<Variable::z, Process::gp> z(setup);
    base_ = z.base() * pow(2, setup->lrBaseDiffZT());
    range_ = -1.;
    for (int eta = 0; eta < setup->numSectorsEta(); eta++)
      range_ = max((sinh(setup->boundarieEta(eta + 1)) - sinh(setup->boundarieEta(eta))) * setup->chosenRofZ(), range_);
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::cot, Process::lr>::Format(const Setup* setup) : DataFormat(true) {
    base_ = pow(2, setup->lrBaseDiffCot());
    const Format<Variable::zT, Process::lr> zT(setup);
    range_ = (zT.range() + 2. * setup->beamWindowZ()) / setup->chosenRofZ();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  Format<Variable::barrel, Process::mht>::Format(const Setup* setup) : DataFormat(false) {
    width_ = 1;
  }

  template<>
  Format<Variable::psModule, Process::mht>::Format(const Setup* setup) : DataFormat(false) {
    width_ = 1;
  }

} // namespace trackerTFP