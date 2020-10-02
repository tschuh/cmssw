#include "L1Trigger/TrackerTFP/interface/LayerEncoding.h"

#include <vector>
#include <set>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace trackerDTC;

namespace trackerTFP {

  LayerEncoding::LayerEncoding(const DataFormats* dataFormats) :
    dataFormats_(dataFormats),
    setup_(dataFormats->setup()),
    zT_(&dataFormats->format(Variable::zT, Process::sf)),
    cot_(&dataFormats->format(Variable::cot, Process::sf)),
    layerEncoding_(setup_->numSectorsEta(), vector<vector<vector<int>>>(pow(2, zT_->width()), vector<vector<int>>(pow(2, cot_->width())))),
    maybeLayer_(setup_->numSectorsEta(), vector<vector<vector<int>>>(pow(2, zT_->width()), vector<vector<int>>(pow(2, cot_->width()))))
  {
    static constexpr int boundaries = 2;
    vector<const SensorModule*> sensorModules;
    sensorModules.reserve(setup_->sensorModules().size());
    for (const SensorModule& sm : setup_->sensorModules())
      sensorModules.push_back(&sm);
    auto smallerR = [](const SensorModule* lhs, const SensorModule* rhs){ return lhs->r() < rhs->r(); };
    auto smallerZ = [](const SensorModule* lhs, const SensorModule* rhs){ return lhs->z() < rhs->z(); };
    auto equalRZ = [](const SensorModule* lhs, const SensorModule* rhs){ return abs(lhs->r() - rhs->r()) < 1.e-3 && abs(lhs->z() - rhs->z()) < 1.e-3; };
    stable_sort(sensorModules.begin(), sensorModules.end(), smallerR);
    stable_sort(sensorModules.begin(), sensorModules.end(), smallerZ);
    sensorModules.erase(unique(sensorModules.begin(), sensorModules.end(), equalRZ), sensorModules.end());
    for (int binEta = 0; binEta < setup_->numSectorsEta(); binEta++) {
      const double sectorCot = (sinh(setup_->boundarieEta(binEta + 1)) + sinh(setup_->boundarieEta(binEta))) / 2.;
      const double sectorZT = sectorCot * setup_->chosenRofZ();
      const double rangeZT = (sinh(setup_->boundarieEta(binEta + 1)) - sinh(setup_->boundarieEta(binEta))) / 2. * setup_->chosenRofZ();
      for (int binZT = 0; binZT < pow(2, zT_->width()); binZT++) {
        const double zT = zT_->floating(zT_->toSigned(binZT));
        if (abs(zT) > rangeZT + zT_->base() / 2.)
          continue;
        for (int binCot = 0; binCot < pow(2, cot_->width()); binCot++) {
          const double cot = cot_->floating(cot_->toSigned(binCot));
          const double z0 = zT - cot * setup_->chosenRofZ();
          if (abs(z0) > setup_->beamWindowZ() + cot_->base() * setup_->chosenRofZ() / 2.)
            continue;
          vector<set<int>> layers(2);
          const vector<double> zTs = {sectorZT + zT - zT_->base() / 2., sectorZT + zT + zT_->base() / 2.};
          const vector<double> cots = {sectorCot + cot - cot_->base() / 2., sectorCot + cot + cot_->base() / 2.};
          for (const SensorModule* sm : sensorModules) {
            for (int i = 0; i < boundaries; i++) {
              const int j = boundaries - i - 1;
              const double zTi = zTs[sm->r() > setup_->chosenRofZ() ? i : j];
              const double coti = cots[sm->r() > setup_->chosenRofZ() ? j : i];
              const double d = (zTi - sm->z() + (sm->r() - setup_->chosenRofZ()) * coti) / (sm->cos() - sm->sin() * coti);
              if (abs(d) < sm->numColumns() * sm->pitchCol() / 2.)
                layers[i].insert(sm->layerId());
            }
          }
          set<int> maybeLayer;
          set_symmetric_difference(layers[0].begin(), layers[0].end(), layers[1].begin(), layers[1].end(), inserter(maybeLayer, maybeLayer.end()));
          set<int> layerEncoding;
          set_union(layers[0].begin(), layers[0].end(), layers[1].begin(), layers[1].end(), inserter(layerEncoding, layerEncoding.end()));
          layerEncoding_[binEta][binZT][binCot] = vector<int>(layerEncoding.begin(), layerEncoding.end());
          maybeLayer_[binEta][binZT][binCot] = vector<int>(maybeLayer.begin(), maybeLayer.end());
        }
      }
    }
  }
  
  const int LayerEncoding::layerIdKF(int binEta, int binZT, int binCot, int layerId) const {
    const vector<int>& layers = layerEncoding_[binEta][binZT][binCot];
    const int layer = distance(layers.begin(), find(layers.begin(), layers.end(), layerId));
    return layer;
  }

  //
  TTBV LayerEncoding::hitPattern(const vector<TTStubRef>& ttStubRefs, int binEta, int binZT, int binCot) const {
    TTBV pattern(0, setup_->numLayers());
    const vector<int>& layers = layerEncoding_[binEta][binZT][binCot];
    for (const TTStubRef& ttStubRef : ttStubRefs) {
      const int layerId = setup_->layerId(ttStubRef);
      const int layer = distance(layers.begin(), find(layers.begin(), layers.end(), layerId));
      pattern.set(layer);
    }
    return pattern;
  }

  //
  void LayerEncoding::addTTStubRefs(TrackKF& track) const {
    const DataFormat& zT = dataFormats_->format(Variable::zT, Process::kfin);
    const DataFormat& cot = dataFormats_->format(Variable::cot, Process::kfin);
    const int binEta = track.sectorEta();
    const int binZT = zT.toUnsigned(zT.integer(track.ttTrackRef()->z0()));
    const int binCot = cot.toUnsigned(cot.integer(track.ttTrackRef()->tanL()));
    const vector<int>& layers = layerEncoding_[binEta][binZT][binCot];
    vector<vector<TTStubRef>> layerStubs(setup_->numLayers());
    for (vector<TTStubRef>& stubs : layerStubs)
      stubs.reserve(setup_->kfMaxStubsPerLayer());
    for (const TTStubRef& ttStubRef : track.frame().first->getStubRefs()) {
      const int layerId = setup_->layerId(ttStubRef);
      const int layerKF = distance(layers.begin(), find(layers.begin(), layers.end(), layerId));
      layerStubs[layerKF].push_back(ttStubRef);
    }
    vector<TTStubRef> ttstubRefs;
    ttstubRefs.reserve(track.hitPattern().count());
    for (int layer = 0; layer < setup_->numLayers(); layer++)
      if (track.hitPattern(layer))
        ttstubRefs.push_back(layerStubs[layer][track.layerMap(layer)]);
    track.ttStubRefs(ttstubRefs);
  }

} // namespace trackerTFP