#include "RecoLocalCalo/HcalRecAlgos/interface/HcalHF_PETalgorithm.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "Geometry/HcalTowerAlgo/src/HcalHardcodeGeometryData.h" // for eta bounds
#include "RecoLocalCalo/HcalRecAlgos/interface/HcalCaloFlagLabels.h"
#include "CondFormats/HcalObjects/interface/HcalChannelQuality.h"
#include "RecoLocalCalo/HcalRecAlgos/interface/HcalSeverityLevelComputer.h"
#include "RecoLocalCalo/HcalRecAlgos/interface/HcalSeverityLevelComputerRcd.h"

#include <algorithm> // for "max"
#include <cmath>
#include <iostream>
using namespace std;

HcalHF_PETalgorithm::HcalHF_PETalgorithm()
{
  // no params given; revert to old 'algo 1' fixed settings
  short_R.clear();
  short_R.push_back(0.995);
  short_Energy_Thresh.clear();
  short_Energy_Thresh.push_back(50);
  short_ET_Thresh.clear();
  short_ET_Thresh.push_back(0);
  
  long_R.clear();
  long_R.push_back(0.995);
  long_Energy_Thresh.clear();
  long_Energy_Thresh.push_back(50);
  long_ET_Thresh.clear();
  long_ET_Thresh.push_back(0);
}

HcalHF_PETalgorithm::HcalHF_PETalgorithm(std::vector<double> shortR, 
					 std::vector<double> shortEnergyParams, 
					 std::vector<double> shortETParams, 
					 std::vector<double> longR, 
					 std::vector<double> longEnergyParams, 
					 std::vector<double> longETParams)
{
  // R is parameterized depending on the energy of the cells, so just store the parameters here
  short_R=shortR;
  long_R=longR;

  // Energy and ET cuts are ieta-dependent, and only need to be calculated once!
  short_Energy_Thresh.clear();
  short_ET_Thresh.clear();
  long_Energy_Thresh.clear();
  long_ET_Thresh.clear();
  for (int i=29;i<=41;++i)
    {
      short_Energy_Thresh.push_back(CalcThreshold(1.*i,shortEnergyParams));
      short_ET_Thresh.push_back(CalcThreshold(1.*i,shortETParams));
      long_Energy_Thresh.push_back(CalcThreshold(1.*i,longEnergyParams));
      long_ET_Thresh.push_back(CalcThreshold(1.*i,longETParams));
    }
}

HcalHF_PETalgorithm::~HcalHF_PETalgorithm(){}



void HcalHF_PETalgorithm::HFSetFlagFromPET(HFRecHit& hf,
					   HFRecHitCollection& rec,
					   HcalChannelQuality* myqual,
					   const HcalSeverityLevelComputer* mySeverity)
{
  /*  Set the HFLongShort flag by comparing the ratio |L-S|/|L+S|.  Channels must first pass energy and ET cuts, and channels whose partners are known to be dead are skipped, since those channels can never satisfy the ratio cut.  */

  int ieta=hf.id().ieta(); // get coordinates of rechit being checked
  int depth=hf.id().depth();
  int iphi=hf.id().iphi();
  double fEta = 0.5*(theHFEtaBounds[abs(ieta)-29] + theHFEtaBounds[abs(ieta)-28]); // calculate eta as average of eta values at ieta boundaries
  double energy=hf.energy();
  double ET = energy/fabs(cosh(fEta));

  // Step 1:  Check energy and ET cuts
  double ETthresh=0, Energythresh=0; // set ET, energy thresholds
  if (depth==1)  // set thresholds for long fibers
    {
      Energythresh = long_Energy_Thresh[abs(ieta)-29];
      ETthresh     = long_ET_Thresh[abs(ieta)-29];
    }
  else if (depth==2) // short fibers
    {
      Energythresh = short_Energy_Thresh[abs(ieta)-29];
      ETthresh     = short_ET_Thresh[abs(ieta)-29];
    }
  if (energy<Energythresh || ET < ETthresh)
    {
      hf.setFlagField(0, HcalCaloFlagLabels::HFLongShort); // shouldn't be necessary, but set bit to 0 just to be sure
      return;
    }

  // Step 2:  Get partner info, check if partner is excluded from rechits already
  HcalDetId partner(HcalForward, ieta, iphi, 3-depth); //  if depth=1, 3-depth=2, and vice versa
  DetId detpartner=DetId(partner);
  const HcalChannelStatus* partnerstatus=myqual->getValues(detpartner.rawId());
  if (mySeverity->dropChannel(partnerstatus->getValue() ) )
    {
      hf.setFlagField(0, HcalCaloFlagLabels::HFLongShort); // shouldn't be necessary, but set bit to 0 just to be sure
      return;
    }
  // Step 3:  Compute ratio
  double Ratio=1;
  HFRecHitCollection::const_iterator part=rec.find(partner);
  if (part!=rec.end())
    Ratio=(energy-part->energy())/(energy+part->energy());

  double RatioThresh=0;
  // Allow for the ratio cut to be parameterized in terms of energy
  if (depth==1) RatioThresh=CalcThreshold(energy,long_R);
  else if (depth==2) RatioThresh=CalcThreshold(energy,short_R);

  if (Ratio<=RatioThresh)
    {
      hf.setFlagField(0, HcalCaloFlagLabels::HFLongShort); // shouldn't be necessary, but set bit to 0 just to be sure
      return;
    }
  // Made it this far -- ratio is > threshold, and cell should be flagged!
  hf.setFlagField(1, HcalCaloFlagLabels::HFLongShort);

}//void HcalHF_PETalgorithm::HFSetFlagFromPET



double HcalHF_PETalgorithm::CalcThreshold(double abs_x,std::vector<double> params)
{
  /* CalcEnergyThreshold calculates the polynomial [0]+[1]*x + [2]*x^2 + ....,
     where x is an integer provided by the first argument (int double abs_x),
     and [0],[1],[2] is a vector of doubles provided by the second (std::vector<double> params).
     The output of the polynomial calculation (threshold) is returned by the function.
  */
  double threshold=0;
  for (std::vector<double>::size_type i=0;i<params.size();++i)
    {
      threshold+=params[i]*pow(abs_x, (int)i);
    }
  return threshold;
} //double HcalHF_PETalgorithm::CalcThreshold(double abs_x,std::vector<double> params)

