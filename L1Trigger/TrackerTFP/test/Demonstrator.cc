#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/Common/interface/Handle.h"

#include "L1Trigger/TrackerTFP/interface/DataFormats.h"

#include <vector>
#include <sstream>
#include <string>
#include <fstream>

using namespace std;
using namespace edm;
using namespace trackerDTC;

namespace trackerTFP {

  /*! \class  trackerTFP::Demonstrator
   *  \brief  Class to demontrate correctness of track trigger emulators
   *  \author Thomas Schuh
   *  \date   2020, June
   */
  class Demonstrator : public one::EDAnalyzer<one::WatchRuns> {
  public:
    Demonstrator(const ParameterSet& iConfig);
    void beginJob() override {}
    void beginRun(const Run& iEvent, const EventSetup& iSetup) override;
    void analyze(const Event& iEvent, const EventSetup& iSetup) override;
    void endRun(const Run& iEvent, const EventSetup& iSetup) override {}
    void endJob() override {}

  private:
    string header(int numChannel) const;
    string infraGap(int& nFrame, int numChannel) const;
    string frame(int& nFrame) const;
    string hex(const TTDTC::BV& bv) const;
    // ED input token of mht stubs
    EDGetTokenT<TTDTC::Streams> edGetTokenStubsMHT_;
    // ED input token of lr stubs
    EDGetTokenT<TTDTC::Streams> edGetTokenStubsLR_;
    // ED input token of lr track info
    EDGetTokenT<TTDTC::Streams> edGetTokenTracksLR_;
    // Setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // DataFormats token
    ESGetToken<DataFormats, DataFormatsRcd> esGetTokenDataFormats_;
    // stores, calculates and provides run-time constants
    const Setup* setup_;
    // helper class to extract structured data from TTDTC::Frames
    const DataFormats* dataFormats_;
    //
    string dirIPBB_;
    double runTime_;
    string dirIn_;
    string dirOut_;
    string dirPre_;
    string dirDiff_;
  };

  Demonstrator::Demonstrator(const ParameterSet& iConfig) :
    dirIPBB_(iConfig.getParameter<string>("DirIPBB")),
    runTime_(iConfig.getParameter<double>("RunTime")),
    dirIn_(dirIPBB_ + "in.txt"),
    dirOut_(dirIPBB_ + "out.txt"),
    dirPre_(dirIPBB_ + "pre.txt"),
    dirDiff_(dirIPBB_ + "diff.txt")
  {
    // book ED products
    edGetTokenStubsMHT_ = consumes<TTDTC::Streams>(InputTag("TrackerTFPProducerMHT", "StubAccepted"));
    edGetTokenStubsLR_ = consumes<TTDTC::Streams>(InputTag("TrackerTFPProducerLR", "StubAccepted"));
    edGetTokenTracksLR_ = consumes<TTDTC::Streams>(InputTag("TrackerTFPProducerLR", "TrackAccepted"));
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
    esGetTokenDataFormats_ = esConsumes<DataFormats, DataFormatsRcd, Transition::BeginRun>();
    // initial ES products
    setup_ = nullptr;
    dataFormats_ = nullptr;
  }

  void Demonstrator::beginRun(const Run& iEvent, const EventSetup& iSetup) {
    // helper class to store configurations
    setup_ = &iSetup.getData(esGetTokenSetup_);
    // helper class to extract structured data from TTDTC::Frames
    dataFormats_ = &iSetup.getData(esGetTokenDataFormats_);
  }

  void Demonstrator::analyze(const Event& iEvent, const EventSetup& iSetup) {
    Handle<TTDTC::Streams> handleStubsMHT;
    iEvent.getByToken<TTDTC::Streams>(edGetTokenStubsMHT_, handleStubsMHT);
    stringstream ss;
    ss << header(dataFormats_->numChannel(Process::mht));
    int nFrame(0);
    for (int region = 0; region < setup_->numRegions(); region++) {
      ss << infraGap(nFrame, dataFormats_->numChannel(Process::mht));
      for (int frame = 0; frame < setup_->numFrames() + setup_->numFramesInfra(); frame++) {
        ss << this->frame(nFrame);
        for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
          const TTDTC::Stream& stream = handleStubsMHT->at(region * dataFormats_->numChannel(Process::mht) + channel);
          TTDTC::BV bv;
          if (frame < (int)stream.size())
            bv = stream[frame].second;
          ss << hex(bv);
        }
        ss << endl;
      }
    }
    fstream fs;
    fs.open(dirIn_.c_str(), fstream::out);
    fs << ss.rdbuf();
    fs.close();
    Handle<TTDTC::Streams> handleStubsLR;
    iEvent.getByToken<TTDTC::Streams>(edGetTokenStubsLR_, handleStubsLR);
    Handle<TTDTC::Streams> handleTracksLR;
    iEvent.getByToken<TTDTC::Streams>(edGetTokenTracksLR_, handleTracksLR);
    ss.str("");
    ss.clear();
    ss << header(dataFormats_->numChannel(Process::lr));
    nFrame = 0;
    for (int region = 0; region < setup_->numRegions(); region++) {
      ss << infraGap(nFrame, dataFormats_->numChannel(Process::lr));
      for (int frame = 0; frame < setup_->numFrames() + setup_->numFramesInfra(); frame++) {
        ss << this->frame(nFrame);
        for (int channel = 0; channel < dataFormats_->numChannel(Process::mht); channel++) {
          const TTDTC::Stream& streamT = handleTracksLR->at(region * dataFormats_->numChannel(Process::mht) + channel);
          TTDTC::BV bv;
          if (frame < (int)streamT.size())
            bv = streamT[frame].second;
          ss << hex(bv);
          bv.reset();
          const TTDTC::Stream& streamS = handleStubsLR->at(region * dataFormats_->numChannel(Process::mht) + channel);
          if (frame < (int)streamS.size())
            bv = streamS[frame].second;
          ss << hex(bv);
        }
        ss << endl;
      }
    }
    fs.open(dirPre_.c_str(), fstream::out);
    fs << ss.rdbuf();
    fs.close();
    stringstream cmd;
    cmd << "cd " << dirIPBB_ << " && ./vsim -quiet -c work.top -do 'run " << runTime_ << "us' -do 'quit' &> /dev/null";
    system(cmd.str().c_str());
    const string c = "diff " + dirPre_ + " " + dirOut_ + " &> " + dirDiff_;
    system(c.c_str());
    fs.open(dirDiff_.c_str(), fstream::in);
    ss.str("");
    ss.clear();
    ss << fs.rdbuf();
    fs.close();
    int n(0);
    string token;
    while (getline(ss, token))
      n++;
    if (n != 4) {
      cms::Exception exception("RunTimeError.");
      exception.addContext("trackerTFP::Demonstrator::analyze");
      exception << "Bit error detected.";
      throw exception;
    }
  }

  //
  string Demonstrator::header(int numLinks) const {
    stringstream ss;
    ss << "Board CMSSW" << endl << " Quad/Chan :";
    for (int link = 0; link < numLinks; link++)
      ss << "        q" << setfill('0') << setw(2) << link / 4 << "c" << link % 4 << "      ";
    ss << endl << "      Link :";
    for (int link = 0; link < numLinks; link++)
      ss << "         " << setfill('0') << setw(3) << link << "       ";
    ss << endl;
    return ss.str();
  }

  //
  string Demonstrator::infraGap(int& nFrame, int numLinks) const {
    stringstream ss;
    for (int gap = 0; gap < setup_->numFramesInfra(); gap++) {
      ss << frame(nFrame);
      for (int link = 0; link < numLinks; link++)
        ss << " 0v" << string(TTBV::S / 4, '0' );
      ss << endl;
    }
    return ss.str();
  }

  //
  string Demonstrator::frame(int& nFrame) const {
    stringstream ss;
    ss << "Frame " << setfill('0') << setw(4) << nFrame++ << " :";
    return ss.str();
  }

  //
  string Demonstrator::hex(const TTDTC::BV& bv) const {
    stringstream ss;
    ss << " 1v" << setfill('0') << setw(TTBV::S / 4) << std::hex << bv.to_ullong();
    return ss.str();
  }

}  // namespace trackerTFP

DEFINE_FWK_MODULE(trackerTFP::Demonstrator);