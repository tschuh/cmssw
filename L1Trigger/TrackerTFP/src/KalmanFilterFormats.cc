#include "L1Trigger/TrackerTFP/interface/KalmanFilterFormats.h"

#include <vector>
#include <deque>
#include <cmath>
#include <tuple>
#include <iterator>
#include <algorithm>
#include <limits>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  KalmanFilterFormats::KalmanFilterFormats() :
    iConfig_(),
    dataFormats_(nullptr),
    setup_(nullptr)
  {
    formats_.reserve(+VariableKF::end);
  }

  KalmanFilterFormats::KalmanFilterFormats(const ParameterSet& iConfig, const DataFormats* dataFormats) :
    iConfig_(iConfig),
    dataFormats_(dataFormats),
    setup_(dataFormats_->setup())
  {
    formats_.reserve(+VariableKF::end);
    fillFormats();
  }

  template<VariableKF it = VariableKF::begin>
  void KalmanFilterFormats::fillFormats() {
    formats_.emplace_back(FormatKF<it>(dataFormats_, iConfig_));
    if constexpr(++it != VariableKF::end)
      fillFormats<++it>();
  }

  DataFormatKF::DataFormatKF(bool twos) :
    twos_(twos),
    width_(0),
    base_(1.),
    range_(0.),
    rangeActual_(numeric_limits<double>::max(), numeric_limits<double>::lowest()) {}

  void DataFormatKF::updateRangeActual(double d) {
    rangeActual_ = make_pair(min(rangeActual_.first, d), max(rangeActual_.second, d));
  }

  template<>
  FormatKF<VariableKF::x0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const DataFormat& input = dataFormats->format(Variable::qOverPt, Process::sf);
    width_ = iConfig.getParameter<int>("WidthQoverPt");
    const int shift = input.width() - width_;
    base_ = input.base() * pow(2., shift);
    range_ = input.range();
  }

  template<>
  FormatKF<VariableKF::x1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const Setup* setup = dataFormats->setup();
    const DataFormat& input = dataFormats->format(Variable::phiT, Process::sf);
    const DataFormat& r = dataFormats->format(Variable::r, Process::sf);
    const DataFormat& qOverPt = dataFormats->format(Variable::qOverPt, Process::sf);
    const int widthPhi0 = iConfig.getParameter<int>("WidthPhi0");
    const double rangePhi0 = 2. * M_PI / (double)(setup->numRegions()) + qOverPt.range() / r.range() / 2.;
    const int shift = ceil(log2(rangePhi0 / input.base() * pow( 2, -widthPhi0)));
    base_ = input.base() * pow(2., shift);
    width_ = ceil(log2(input.range() / base_));
  }

  template<>
  FormatKF<VariableKF::x2>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const Setup* setup = dataFormats->setup();
    const int cotWidth = iConfig.getParameter<int>("WidthCot");
    const double cotRange = 2. * sinh(setup->maxEta());
    const int cotShift = ceil(log2(cotRange)) - cotWidth;
    base_ = pow(2., cotShift);
    for (int eta = 0; eta < setup->numSectorsEta(); eta++)
      range_ = max(range_, (sinh(setup->boundarieEta(eta + 1)) - sinh(setup->boundarieEta(eta))));
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  FormatKF<VariableKF::x3>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const Setup* setup = dataFormats->setup();
    const DataFormat& z = dataFormats->format(Variable::z, Process::sf);
    width_ = iConfig.getParameter<int>("WidthZ0");
    range_ = 2. * setup->beamWindowZ();
    const int shift = ceil(log2(range_ / z.base() * pow(2., -width_)));
    base_ = z.base() * pow(2., shift);
  }

  template<>
  FormatKF<VariableKF::H00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const DataFormat& sf = dataFormats->format(Variable::r, Process::sf);
    base_ = sf.base();
    width_ = sf.width();
    range_ = sf.range();
  }

  template<>
  FormatKF<VariableKF::H12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const Setup* setup = dataFormats->setup();
    const DataFormat& sf = dataFormats->format(Variable::r, Process::sf);
    base_ = sf.base();
    range_ = setup->outerRadius();
    width_ = ceil(log2(range_ / base_));
  }

  template<>
  FormatKF<VariableKF::m0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const DataFormat& sf = dataFormats->format(Variable::phi, Process::sf);
    base_ = sf.base();
    width_ = sf.width();
    range_ = sf.range();
  }

  template<>
  FormatKF<VariableKF::m1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const DataFormat& sf = dataFormats->format(Variable::z, Process::sf);
    base_ = sf.base();
    width_ = sf.width();
    range_ = sf.range();
  }

  template<>
  FormatKF<VariableKF::v0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftv0");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::v1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftv1");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::r0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftr0");
    base_ = pow(2., baseShift) * x.base();
  }

  template<>
  FormatKF<VariableKF::r1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftr1");
    base_ = pow(2., baseShift) * x.base();
  }

  template<>
  FormatKF<VariableKF::r02>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftr02");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::r12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftr12");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::S00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x0> x0(dataFormats, iConfig);
    const FormatKF<VariableKF::x1> x1(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftS00");
    base_ = pow(2., baseShift) * x0.base() * x1.base();
  }

  template<>
  FormatKF<VariableKF::S01>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x1> x1(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftS01");
    base_ = pow(2., baseShift) * x1.base() * x1.base();
  }

  template<>
  FormatKF<VariableKF::S12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x2> x2(dataFormats, iConfig);
    const FormatKF<VariableKF::x3> x3(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftS12");
    base_ = pow(2., baseShift) * x2.base() * x3.base();
  }

  template<>
  FormatKF<VariableKF::S13>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x3> x3(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftS13");
    base_ = pow(2., baseShift) * x3.base() * x3.base();
  }

  template<>
  FormatKF<VariableKF::K00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x0> x0(dataFormats, iConfig);
    const FormatKF<VariableKF::x1> x1(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftK00");
    base_ = pow(2., baseShift) * x0.base() / x1.base();
  }

  template<>
  FormatKF<VariableKF::K10>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const int baseShift = iConfig.getParameter<int>("BaseShiftK10");
    base_ = pow(2., baseShift);
  }

  template<>
  FormatKF<VariableKF::K21>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x2> x2(dataFormats, iConfig);
    const FormatKF<VariableKF::x3> x3(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftK21");
    base_ = pow(2., baseShift) * x2.base() / x3.base();
  }

  template<>
  FormatKF<VariableKF::K31>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const int baseShift = iConfig.getParameter<int>("BaseShiftK31");
    base_ = pow(2., baseShift);
  }

  template<>
  FormatKF<VariableKF::R00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftR00");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::R11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftR11");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::invR00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftInvR00");
    base_ = pow(2., baseShift) / x.base() / x.base();
  }

  template<>
  FormatKF<VariableKF::invR11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftInvR11");
    base_ = pow(2., baseShift) / x.base() / x.base();
  }

  template<>
  FormatKF<VariableKF::chi20>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const int baseShift = iConfig.getParameter<int>("BaseShiftChi20");
    base_ = pow(2., baseShift);
  }

  template<>
  FormatKF<VariableKF::chi21>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const int baseShift = iConfig.getParameter<int>("BaseShiftChi21");
    base_ = pow(2., baseShift);
  }

  template<>
  FormatKF<VariableKF::chi2>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const int baseShift = iConfig.getParameter<int>("BaseShiftChi2");
    base_ = pow(2., baseShift);
  }

  template<>
  FormatKF<VariableKF::C00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x0> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC00");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::C01>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x0> x0(dataFormats, iConfig);
    const FormatKF<VariableKF::x1> x1(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC01");
    base_ = pow(2., baseShift) * x0.base() * x1.base();
  }

  template<>
  FormatKF<VariableKF::C11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x1> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC11");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::C22>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x2> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC22");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

  template<>
  FormatKF<VariableKF::C23>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(true) {
    const FormatKF<VariableKF::x2> x2(dataFormats, iConfig);
    const FormatKF<VariableKF::x3> x3(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC23");
    base_ = pow(2., baseShift) * x2.base() * x3.base();
  }

  template<>
  FormatKF<VariableKF::C33>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig) : DataFormatKF(false) {
    const FormatKF<VariableKF::x3> x(dataFormats, iConfig);
    const int baseShift = iConfig.getParameter<int>("BaseShiftC33");
    base_ = pow(2., baseShift) * x.base() * x.base();
  }

} // namespace trackerTFP