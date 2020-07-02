#ifndef L1Trigger_TrackerTFP_DataFormats_h
#define L1Trigger_TrackerTFP_DataFormats_h

#include "FWCore/Framework/interface/data_default_record_trait.h"
#include "L1Trigger/TrackerTFP/interface/DataFormatsRcd.h"
#include "L1Trigger/TrackerDTC/interface/Setup.h"

#include <vector>
#include <cmath>
#include <initializer_list>
#include <tuple>
#include <iostream>

namespace trackerTFP {

  //enum class Process { begin, fe = begin, dtc, pp, gp, ht, mht, sf, kf, dr, end, x };
  enum class Process { begin, fe = begin, dtc, pp, gp, ht, mht, lr, end, x };
  enum class Variable { begin, r = begin, phi, z, layer, sectorsPhi, sectorEta, sectorPhi, phiT, qOverPt, zT, cot, barrel, psModule, end, x };
  //constexpr std::initializer_list<Process> Processes = {Process::fe, Process::dtc, Process::pp, Process::gp, Process::ht, Process::mht, Process::sf, Process::kf, Process::dr};
  constexpr std::initializer_list<Process> Processes = {Process::fe, Process::dtc, Process::pp, Process::gp, Process::ht, Process::mht, Process::lr};
  constexpr std::initializer_list<Variable> Variables = {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorsPhi, Variable::sectorEta, Variable::sectorPhi, Variable::phiT, Variable::qOverPt, Variable::zT, Variable::cot, Variable::barrel, Variable::psModule};
  inline constexpr int operator+(Process p) { return static_cast<int>(p); }
  inline constexpr int operator+(Variable v) { return static_cast<int>(v); }
  inline constexpr Process operator++(Process p) { return Process(+p + 1); }
  inline constexpr Variable operator++(Variable v) { return Variable(+v + 1); }

  class DataFormat {
  public:
    DataFormat(bool twos) : twos_(twos), width_(0), base_(1.), range_(0.) {}
    ~DataFormat() {}
    TTBV ttBV(int i) const { return TTBV(i, width_, twos_); }
    TTBV ttBV(double d) const { return TTBV(d, base_, width_, twos_); }
    void extract(TTBV& in, int& out) const { out = in.extract(width_, twos_); }
    void extract(TTBV& in, double& out) const { out = in.extract(base_, width_, twos_); }
    void extract(TTBV& in, TTBV& out) const { out = in.slice(width_, twos_); }
    void attach(const int i, TTBV& ttBV) const { ttBV += TTBV(i, width_, twos_); }
    void attach(const double d, TTBV& ttBV) const { ttBV += TTBV(d, base_, width_, twos_); }
    void attach(const TTBV bv, TTBV& ttBV) const { ttBV += bv; }
    double floating(int i) const { return (i + .5) * base_; }
    int integer(double d) const { return std::floor(d / base_); }
    int toSigned(int i) const { return i - integer(range_) / 2; }
    int toUnsigned(int i) const { return i + integer(range_) / 2; }
    bool inRange(double d) const { return d >= -range_ / 2. && d < range_ / 2.; }
    bool inRange(int i) const { return inRange(floating(i)); }
    bool twos() const { return twos_; }
    int width() const { return width_; }
    double base() const { return base_; }
    double range() const { return range_; }
  protected:
    bool twos_;
    int width_;
    double base_;
    double range_;
  };

  template<Variable v, Process p>
  class Format : public DataFormat {
  public:
    Format(const trackerDTC::Setup* setup);
    ~Format() {}
  };

  template<> Format<Variable::phiT, Process::ht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phiT, Process::mht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::qOverPt, Process::ht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::qOverPt, Process::mht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::r, Process::ht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phi, Process::ht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phi, Process::mht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phi, Process::gp>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phi, Process::dtc>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::z, Process::dtc>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::z, Process::gp>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::layer, Process::ht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::sectorEta, Process::gp>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::sectorPhi, Process::gp>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::sectorsPhi, Process::gp>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::phiT, Process::lr>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::qOverPt, Process::lr>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::zT, Process::lr>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::cot, Process::lr>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::barrel, Process::mht>::Format(const trackerDTC::Setup* setup);
  template<> Format<Variable::psModule, Process::mht>::Format(const trackerDTC::Setup* setup);

  class DataFormats {
  private:
    static constexpr std::array<std::array<Process, +Process::end>, +Variable::end> config_ = {{
    //  Process::fe  Process::dtc  Process::pp   Process::gp  Process::ht  Process::mht  Process::sf   Process::kf Process::dr
      /*{{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::ht,  Process::ht,  Process::x, Process::x}}, // Variable::r
      {{Process::x,  Process::dtc, Process::dtc, Process::gp, Process::ht, Process::mht, Process::mht, Process::x, Process::x}}, // Variable::phi
      {{Process::x,  Process::dtc, Process::dtc, Process::gp, Process::gp, Process::gp,  Process::gp,  Process::x, Process::x}}, // Variable::z
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::ht,  Process::ht,  Process::x, Process::x}}, // Variable::layer
      {{Process::x,  Process::dtc, Process::dtc, Process::x,  Process::x,  Process::x,   Process::x,   Process::x, Process::x}}, // Variable::sectorsPhi
      {{Process::x,  Process::gp,  Process::gp,  Process::gp, Process::gp, Process::gp,  Process::gp,  Process::x, Process::x}}, // Variable::sectorEta
      {{Process::x,  Process::x,   Process::x,   Process::gp, Process::gp, Process::gp,  Process::gp,  Process::x, Process::x}}, // Variable::sectorPhi
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::mht, Process::mht, Process::x, Process::x}}, // Variable::phiT
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::mht, Process::mht, Process::x, Process::x}}  // Variable::qOverPt*/
    //  Process::fe  Process::dtc  Process::pp   Process::gp  Process::ht  Process::mht  Process::lr
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::ht,  Process::ht }}, // Variable::r
      {{Process::x,  Process::dtc, Process::dtc, Process::gp, Process::ht, Process::mht, Process::mht}}, // Variable::phi
      {{Process::x,  Process::dtc, Process::dtc, Process::gp, Process::gp, Process::gp,  Process::gp }}, // Variable::z
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::ht,  Process::ht }}, // Variable::layer
      {{Process::x,  Process::dtc, Process::dtc, Process::x,  Process::x,  Process::x,   Process::x  }}, // Variable::sectorsPhi
      {{Process::x,  Process::gp,  Process::gp,  Process::gp, Process::gp, Process::gp,  Process::gp }}, // Variable::sectorEta
      {{Process::x,  Process::x,   Process::x,   Process::gp, Process::gp, Process::gp,  Process::gp }}, // Variable::sectorPhi
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::mht, Process::lr }}, // Variable::phiT
      {{Process::x,  Process::ht,  Process::ht,  Process::ht, Process::ht, Process::mht, Process::lr }}, // Variable::qOverPt
      {{Process::x,  Process::x,   Process::x,   Process::x,  Process::x,  Process::x,   Process::lr }}, // Variable::zT
      {{Process::x,  Process::x,   Process::x,   Process::x,  Process::x,  Process::x,   Process::lr }}, // Variable::cot
      {{Process::x,  Process::x,   Process::x,   Process::x,  Process::x,  Process::mht, Process::x  }}, // Variable::barrel
      {{Process::x,  Process::x,   Process::x,   Process::x,  Process::x,  Process::mht, Process::x  }}, // Variable::psModule
    }};
    static constexpr std::array<std::initializer_list<Variable>, +Process::end> stubs_ = {{
      /*{},                                                                                                                                                               // Process::fe
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorsPhi, Variable::sectorEta, Variable::sectorEta, Variable::qOverPt, Variable::qOverPt}, // Process::dtc
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorsPhi, Variable::sectorEta, Variable::sectorEta, Variable::qOverPt, Variable::qOverPt}, // Process::pp
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::qOverPt, Variable::qOverPt},                                                                 // Process::gp
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT},                                             // Process::ht
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT, Variable::qOverPt},                          // Process::mht
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT, Variable::qOverPt},                          // Process::sf
      {},                                                                                                                                                               // Process::kf
      {}*/
      {},                                                                                                                                                                  // Process::fe
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorsPhi, Variable::sectorEta, Variable::sectorEta, Variable::qOverPt, Variable::qOverPt},    // Process::dtc
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorsPhi, Variable::sectorEta, Variable::sectorEta, Variable::qOverPt, Variable::qOverPt},    // Process::pp
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::qOverPt, Variable::qOverPt},                                                                    // Process::gp
      {Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT},                                                // Process::ht
      {Variable::barrel, Variable::psModule, Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT, Variable::qOverPt},                             // Process::mht
      //{Variable::r, Variable::phi, Variable::z, Variable::layer, Variable::sectorPhi, Variable::sectorEta, Variable::phiT, Variable::qOverPt, Variable::zT, Variable::cot} // Process::lr
      {Variable::phiT, Variable::qOverPt, Variable::zT, Variable::cot} // Process::lr
    }};
  public:
    DataFormats();
    DataFormats(const trackerDTC::Setup* setup);
    ~DataFormats(){}
    template<typename ...Ts>
    void convert(const TTDTC::BV& bv, std::tuple<Ts...>& data, Process p) const;
    template<typename... Ts>
    void convert(const std::tuple<Ts...>& data, TTDTC::BV& bv, Process p) const;
    const trackerDTC::Setup* setup() const { return setup_; }
    int width(Variable v, Process p) const { return formats_[+v][+p]->width(); }
    double base(Variable v, Process p) const { return formats_[+v][+p]->base(); }
    int numUnusedBits(Process p) const { return numUnusedBits_[+p]; }
    int numChannel(Process p) const { return numChannel_[+p]; }
    int numStreams(Process p) const { return numStreams_[+p]; }
    const DataFormat& format(Variable v, Process p) const { return *formats_[+v][+p]; }
  private:
    int numDataFormats_;
    template<Variable v = Variable::begin, Process p = Process::begin>
    void countFormats();
    template<Variable v = Variable::begin, Process p = Process::begin>
    void fillDataFormats();
    template<Variable v, Process p, Process it = Process::begin>
    void fillFormats();
    template<int it = 0, typename ...Ts>
    void extract(TTBV& ttBV, std::tuple<Ts...>& data, Process p) const;
    template<int it = 0, typename... Ts>
    void attach(const std::tuple<Ts...>& data, TTBV& ttBV, Process p) const;
    const trackerDTC::Setup* setup_;
    std::vector<DataFormat> dataFormats_;
    std::vector<std::vector<DataFormat*>> formats_;
    std::vector<int> numUnusedBits_;
    std::vector<int> numChannel_;
    std::vector<int> numStreams_;
  };

  template<typename ...Ts>
  class Stub {
  public:
    Stub(const TTDTC::Frame& frame, const DataFormats* dataFormats, Process p);
    template<typename ...Others>
    Stub(const Stub<Others...>& stub, Ts... data);
    Stub() {}
    ~Stub() {}
    explicit operator bool() const { return frame_.first.isNonnull(); }
    const DataFormats* dataFormats() const { return dataFormats_; }
    Process p() const { return p_; }
    const TTDTC::Frame& frame() const { return frame_; }
    const TTStubRef& ttStubRef() const { return frame_.first; }
    const TTDTC::BV& bv() const { return frame_.second; }
    int trackId() const { return trackId_; }
  protected:
    int width(Variable v) const { return dataFormats_->width(v, p_); }
    double base(Variable v) const { return dataFormats_->base(v, p_); }
    const DataFormat& format(Variable v) const { return dataFormats_->format(v, p_); }
    const DataFormats* dataFormats_;
    Process p_;
    TTDTC::Frame frame_;
    std::tuple<Ts...> data_;
    int trackId_;
  };

  class StubPP : public Stub<double, double, double, int, TTBV, int, int, int, int> {
  public:
    StubPP(const TTDTC::Frame& frame, const DataFormats* dataFormats);
    ~StubPP(){}
    bool inSector(int sector) const { return sectors_[sector]; }
    std::vector<int> sectors() const { return sectors_.ids(); }
    double r() const { return std::get<0>(data_); }
    double phi() const { return std::get<1>(data_); }
    double z() const { return std::get<2>(data_); }
    int layer() const { return std::get<3>(data_); }
    TTBV sectorsPhi() const { return std::get<4>(data_); }
    int sectorEtaMin() const { return std::get<5>(data_); }
    int sectorEtaMax() const { return std::get<6>(data_); }
    int qOverPtMin() const { return std::get<7>(data_); }
    int qOverPtMax() const { return std::get<8>(data_); }
  private:
    TTBV sectors_;
  };

  class StubGP : public Stub<double, double, double, int, int, int> {
  public:
    StubGP(const TTDTC::Frame& frame, const DataFormats* dataFormats, int sectorPhi, int sectorEta);
    StubGP(const StubPP& stub, int sectorPhi, int sectorEta);
    ~StubGP(){}
    bool inQoverPtBin(int qOverPtBin) const { return qOverPtBins_[qOverPtBin]; }
    std::vector<int> qOverPtBins() const { return qOverPtBins_.ids(); }
    int sectorPhi() const { return sectorPhi_; }
    int sectorEta() const { return sectorEta_; }
    double r() const { return std::get<0>(data_); }
    double phi() const { return std::get<1>(data_); }
    double z() const { return std::get<2>(data_); }
    int layer() const { return std::get<3>(data_); }
    int qOverPtMin() const { return std::get<4>(data_); }
    int qOverPtMax() const { return std::get<5>(data_); }
  private:
    TTBV qOverPtBins_;
    int sectorPhi_;
    int sectorEta_;
  };

  class StubHT : public Stub<double, double, double, int, int, int, int> {
  public:
    StubHT(const TTDTC::Frame& frame, const DataFormats* dataFormats, int qOverPt);
    StubHT(const StubGP& stub, int qOverPt, int phiT);
    ~StubHT(){}
    int qOverPt() const { return qOverPt_; }
    double r() const { return std::get<0>(data_); };
    double phi() const { return std::get<1>(data_); };
    double z() const { return std::get<2>(data_); };
    int layer() const { return std::get<3>(data_); };
    int sectorPhi() const { return std::get<4>(data_); };
    int sectorEta() const { return std::get<5>(data_); };
    int phiT() const { return std::get<6>(data_); };
  private:
    void fillTrackId();
    int qOverPt_;
  };

  class StubMHT : public Stub<int, int, double, double, double, int, int, int, int, int> {
  public:
    StubMHT(const TTDTC::Frame& frame, const DataFormats* dataFormats);
    StubMHT(const StubHT& stub, int phiT, int qOverPt);
    ~StubMHT(){}
    int barrel() const { return std::get<0>(data_); }
    int psModule() const { return std::get<1>(data_); }
    double r() const { return std::get<2>(data_); }
    double phi() const { return std::get<3>(data_); }
    double z() const { return std::get<4>(data_); }
    int layer() const { return std::get<5>(data_); }
    int sectorPhi() const { return std::get<6>(data_); }
    int sectorEta() const { return std::get<7>(data_); }
    int phiT() const { return std::get<8>(data_); }
    int qOverPt() const { return std::get<9>(data_); }
  private:
    void fillTrackId();
  };

  /*class StubSF {
  public:
    StubSF(const TTDTC::Frame& frame, const DataFormats* dataFormats) {}
    StubSF(const StubMHT& stub) {}
    ~StubSF(){}
  private:
  };

  class StubKF {
  public:
    StubKF(const TTDTC::Frame& frame, const DataFormats* dataFormats) {}
    StubKF(const StubSF& stub) {}
    ~StubKF(){}
  private:
  };

  class Track {
  public:
    Track() {}
    ~Track() {}
    TTDTC::Frame toFrame() const { return TTDTC::Frame(); }
  private:
  };

  class TrackSF : public Track {
  public:
    TrackSF(const DataFormats& dataFormats)  {}
    ~TrackSF(){}
  private:
  };

  class TrackKF : public Track {
  public:
    TrackKF(const DataFormats& dataFormats) {}
    TrackKF(const TrackSF& track) {}
    ~TrackKF(){}
  private:
  };

  class TrackDR : public Track {
  public:
    TrackDR(const DataFormats& dataFormats) {}
    TrackDR(const TrackKF& track) {}
    ~TrackDR(){}
  private:
  };*/

  class StubLR : public Stub<double, double, double, double> {
  public:
    StubLR(const TTDTC::Frame& frame, const DataFormats* dataFormats);
    StubLR(const StubMHT& stub, double phiT, double qOverPt, double zT, double cot);
    ~StubLR() {}
    double phiT() const { return std::get<0>(data_); }
    double qOverPt() const { return std::get<1>(data_); }
    double zT() const { return std::get<2>(data_); }
    double cot() const { return std::get<3>(data_); }
  };

} // namespace trackerTFP

EVENTSETUP_DATA_DEFAULT_RECORD(trackerTFP::DataFormats, trackerTFP::DataFormatsRcd);

#endif