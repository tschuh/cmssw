#ifndef L1Trigger_TrackerTFP_LinearRegression_h
#define L1Trigger_TrackerTFP_LinearRegression_h

#include "L1Trigger/TrackerDTC/interface/Setup.h"
#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

namespace trackerTFP {

  // Class to find in a region finer rough candidates in r-phi
  class LinearRegression {
  public:
    LinearRegression(const edm::ParameterSet& iConfig, const trackerDTC::Setup* setup, const DataFormats* dataFormats, int region);
    ~LinearRegression(){}

    // read in and organize input product
    void consume(const TTDTC::Streams& streams);
    // fill output products
    void produce(TTDTC::Streams& stubs, TTDTC::Streams& tracks);

  private:
    // fit track
    void fit(std::vector<StubMHT*>& stubs, StubLR* track);
    //
    void initFit(const std::vector<StubMHT*>& stubs);
    //
    bool checkValidity(const std::vector<StubMHT*>& stubs) const;
    //
    int countLayers(const std::vector<StubMHT*>& stubs, bool onlySeed = false) const;
    //
    void calcHelix();
    //
    void calcResidual();
    //
    bool killLargestResidual();
    //
    void findLargestResidual();
    //
    int countStubs(const std::vector<StubMHT*>& stubs, bool onlyPS = false) const;

    // 
    const trackerDTC::Setup* setup_;
    //
    const DataFormats* dataFormats_;
    //
    int region_;
    // 
    std::vector<StubMHT> stubsMHT_;
    // 
    std::vector<StubLR> stubsLR_;
    //
    std::vector<std::vector<StubMHT*>> input_;
    //
    int nIterations_;
    //
    bool valid_;
    //
    std::vector<StubMHT*> stubs_;
    //
    std::vector<std::vector<StubMHT*>> stubMap_;
    //
    std::vector<int> layerPopulation_;
    //
    double phiT_;
    //
    double qOverPt_;
    //
    double zT_;
    //
    double cot_;
    //
    struct Stub {
      double RPhi;
      double Phi;
      double RZ;
      double Z;
      Stub(double RPhi = 0., double Phi = 0., double RZ = 0., double Z = 0.) : RPhi(RPhi), Phi(Phi), RZ(RZ), Z(Z) {}
      Stub(const StubMHT* stub, const trackerDTC::Setup* setup) :
      RPhi(stub->r()),
      Phi(stub->phi()),
      RZ(stub->r() + setup->chosenRofPhi() - setup->chosenRofZ()),
      Z(stub->z()) {}
      Stub& operator<=(const Stub& a) {
        RPhi = std::min(RPhi, a.RPhi);
        Phi = std::min(Phi, a.Phi);
        RZ = std::min(RZ, a.RZ);
        Z = std::min(Z, a.Z);
        return *this;
      }
      Stub& operator>=(const Stub& a) {
        RPhi = std::max(RPhi, a.RPhi);
        Phi = std::max(Phi, a.Phi);
        RZ = std::max(RZ, a.RZ);
        Z = std::max(Z, a.Z);
        return *this;
      }
      Stub operator+(const Stub& a) const {
        return Stub(RPhi + a.RPhi, Phi + a.Phi, RZ + a.RZ, Z + a.Z);
      }
      Stub& operator/=(double a) {
        RPhi /= a;
        Phi /= a;
        RZ /= a;
        Z /= a;
        return *this;
      }
    };
    //
    struct Sum {
      unsigned int n;
      double xy;
      double x;
      double y;
      double xx;
      Sum(unsigned int n = 0, double xy = 0., double x = 0., double y = 0., double xx = 0.) : n(n), xy(xy), x(x), y(y), xx(xx) {}
      Sum& operator +=(const std::pair<double, double>& stub) {
        n ++;
        xy += stub.first * stub.second;
        x += stub.first;
        y += stub.second;
        xx += stub.first * stub.first;
        return *this;
      }
      std::pair<double, double> calcLinearParameter() const {
        double denominator = n * xx - x * x;
        double slope = (n * xy - x * y) / denominator;
        double intercept = (y * xx - x * xy) / denominator;
        return std::make_pair(slope, intercept);
      }
    };
    //
    struct Residual {
      double phi;
      double z;
      StubMHT* stub;
      Residual() : phi(0.), z(0.), stub(nullptr) {}
      Residual(double phi, double z, StubMHT* stub) : phi(phi), z(z), stub(stub) {}
      Residual(double x) : phi(x), z(x), stub(nullptr) {}
      Residual& operator <= (const Residual& a) {
        phi = std::min(phi, a.phi);
        z = std::min(z, a.z);
        return *this;
      }
      Residual& operator-=(const Residual& a) {
        phi -= a.phi;
        z -= a.z;
        return *this;
      }
      double combined() const {
        return phi+z;
      }
      double max() const {
        return std::max(phi, z);
      }
      int layerId() const { return stub->layer();  }
      bool ps() const { return stub->psModule(); }
    };
    //
    std::vector<std::vector<Residual>> residuals_;
    //
    Residual largestResid_;
  };

}

#endif