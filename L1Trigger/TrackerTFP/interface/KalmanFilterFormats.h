#ifndef L1Trigger_TrackerTFP_KalmanFilterFormats_h
#define L1Trigger_TrackerTFP_KalmanFilterFormats_h

#include "FWCore/Framework/interface/data_default_record_trait.h"
#include "L1Trigger/TrackerTFP/interface/KalmanFilterFormatsRcd.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <cmath>
#include <initializer_list>
#include <tuple>
#include <utility>

namespace trackerTFP {

  enum class VariableKF { begin, x0 = begin, x1, x2, x3, H00, H12, m0, m1, v0, v1, r0, r1, r02, r12, S00, S01, S12, S13, K00, K10, K21, K31, R00, R11, invR00, invR11, chi20, chi21, C00, C01, C11, C22, C23, C33, chi2, end, x };
  inline constexpr int operator+(VariableKF v) { return static_cast<int>(v); }
  inline constexpr VariableKF operator++(VariableKF v) { return VariableKF(+v + 1); }

  class DataFormatKF {
  public:
    DataFormatKF(bool twos);
    ~DataFormatKF() {}
    void updateRangeActual(double d);
    //double digir(double val) const { return std::round(val / base_) * base_; }
    double digir(double val) const { return val; }
    //double digif(double val) const { return (std::floor(val / base_) + .5) * base_; }
    double digif(double val) const { return val; }
    bool twos() const { return twos_; }
    int width() const { return width_; }
    double base() const { return base_; }
    double range() const { return range_; }
    const std::pair<double, double>& rangeActual() const { return rangeActual_; }
  protected:
    bool twos_;
    int width_;
    double base_;
    double range_;
    std::pair<double, double> rangeActual_;
  };

  template<VariableKF v>
  class FormatKF : public DataFormatKF {
  public:
    FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
    ~FormatKF() {}
  };

  template<> FormatKF<VariableKF::x0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::x1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::x2>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::x3>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::H00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::H12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::m0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::m1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::v0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::v1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::r0>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::r1>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::r02>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::r12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::S00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::S01>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::S12>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::S13>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::K00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::K10>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::K21>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::K31>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::R00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::R11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::invR00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::invR11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::chi20>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::chi21>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C00>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C01>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C11>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C22>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C23>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::C33>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);
  template<> FormatKF<VariableKF::chi2>::FormatKF(const DataFormats* dataFormats, const edm::ParameterSet& iConfig);

  class KalmanFilterFormats {
  public:
    KalmanFilterFormats();
    KalmanFilterFormats(const edm::ParameterSet& iConfig, const DataFormats* dataFormats);
    ~KalmanFilterFormats(){}
    const trackerDTC::Setup* setup() const { return setup_; }
    const DataFormats* dataFormats() const { return dataFormats_; }
    int width(VariableKF v) const { return formats_[+v].width(); }
    double base(VariableKF v) const { return formats_[+v].base(); }
    const DataFormatKF& format(VariableKF v) const { return formats_[+v]; }
  private:
    template<VariableKF it = VariableKF::begin>
    void fillFormats();
    const edm::ParameterSet iConfig_;
    const DataFormats* dataFormats_;
    const trackerDTC::Setup* setup_;
    std::vector<DataFormatKF> formats_;
  };

} // namespace trackerTFP

EVENTSETUP_DATA_DEFAULT_RECORD(trackerTFP::KalmanFilterFormats, trackerTFP::KalmanFilterFormatsRcd);

#endif