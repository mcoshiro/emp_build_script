#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/Exception.h"
#include "DataFormats/Common/interface/Handle.h"

#include "L1Trigger/TrackerTFP/interface/Demonstrator.h"
#include "L1Trigger/TrackFindingTracklet/interface/ChannelAssignment.h"

#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

using namespace std;
using namespace edm;
using namespace tt;
using namespace trackerTFP;

namespace trklet {

  /*! \class  trklet::AnalyzerDemonstrator2
   *  \brief  Class to output EMP style output from track trigger emulation
   *          software for comparison with firmware in silico or in situ
   *  \author Thomas Schuh, modified by Michael Oshiro
   *  \date   2023, January
   */
  class AnalyzerDemonstrator2 : public one::EDAnalyzer<one::WatchRuns> {
  public:
    AnalyzerDemonstrator2(const ParameterSet& iConfig);
    void beginJob() override {}
    void beginRun(const Run& iEvent, const EventSetup& iSetup) override;
    void analyze(const Event& iEvent, const EventSetup& iSetup) override;
    void endRun(const Run& iEvent, const EventSetup& iSetup) override {}
    void endJob() override;

  private:
    // extracts bits from tracks
    void convert(const Event& iEvent,
                 const EDGetTokenT<StreamsTrack>& tokenTracks,
                 vector<vector<Frame>>& bits) const;
    // helper function to extract bits from collection
    template <typename T>
    void convert(const T& collection, vector<vector<Frame>>& bits) const;
    // write event in emp format
    void write_event(const vector<vector<Frame>>& bits, 
                     stringstream& ss, 
                     bool& first, 
                     int& nframe) const;
    // write emp header
    string header(int numLinks) const;
    // write inter-packet 6 frame gap
    string infraGap(int& nFrame, int numLinks) const;
    // creates frame number
    string frame(int& nFrame) const;
    // converts bv into hex
    string hex(const Frame& bv, bool first) const;
    // writes EMP-format output to a file on disk
    void write_output(stringstream& ss) const;

    // tracks token
    EDGetTokenT<StreamsTrack> edGetTokenTracksOut_;
    // setup token
    ESGetToken<Setup, SetupRcd> esGetTokenSetup_;
    // setup object
    const Setup* setup_ = nullptr;
    // output to be written to file
    stringstream ss_;
    // frame number
    int nframe_ = 0;
    // flag for first event
    bool first_ = true;
    // directory to write output
    const string dirIPBB_;
  };

  AnalyzerDemonstrator2::AnalyzerDemonstrator2(const ParameterSet& iConfig) {
    // book in- and output ED products
    const string& labelOut = iConfig.getParameter<string>("LabelOut");
    const string& branchTracks = iConfig.getParameter<string>("BranchAcceptedTracks");
    if (labelOut != "TrackFindingTrackletProducerIRin")
      edGetTokenTracksOut_ = consumes<StreamsTrack>(InputTag(labelOut, branchTracks));
    // book ES products
    esGetTokenSetup_ = esConsumes<Setup, SetupRcd, Transition::BeginRun>();
  }

  void AnalyzerDemonstrator2::beginRun(const Run& iEvent, const EventSetup& iSetup) {
    setup_ = &iSetup.getData(esGetTokenSetup_);
  }

  void AnalyzerDemonstrator2::analyze(const Event& iEvent, const EventSetup& iSetup) {
    vector<vector<Frame>> output;
    convert(iEvent, edGetTokenTracksOut_, output);
    write_event(output, ss_, first_, nframe_);
  }

  void AnalyzerDemonstrator2::endJob() {
    stringstream log;
    LogPrint("L1Trigger/TrackerTFP") << "Successfully wrote to pre.txt";
    write_output(ss_);
  }

  // extracts bits from tracks
  void AnalyzerDemonstrator2::convert(const Event& iEvent,
                                      const EDGetTokenT<StreamsTrack>& tokenTracks,
                                      vector<vector<Frame>>& bits) const {
    const bool tracks = !tokenTracks.isUninitialized();
    Handle<StreamsTrack> handleTracks;
    int numChannelTracks(0);
    if (tracks) {
      iEvent.getByToken<StreamsTrack>(tokenTracks, handleTracks);
      numChannelTracks = handleTracks->size();
    }
    numChannelTracks /= setup_->numRegions();
    bits.reserve(numChannelTracks);
    for (int region = 0; region < setup_->numRegions(); region++) {
      if (tracks) {
        const int offsetTracks = region * numChannelTracks;
        for (int channelTracks = 0; channelTracks < numChannelTracks; channelTracks++) {
          convert(handleTracks->at(offsetTracks + channelTracks), bits);
        }
      }
    }
  }

  // helper function to extract bits from collection
  template <typename T>
  void AnalyzerDemonstrator2::convert(const T& collection, vector<vector<Frame>>& bits) const {
    bits.emplace_back();
    vector<Frame>& bvs = bits.back();
    bvs.reserve(collection.size());
    transform(collection.begin(), collection.end(), back_inserter(bvs), [](const auto& frame) { return frame.second; });
  }

  // write event in emp format
  void AnalyzerDemonstrator2::write_event(const vector<vector<Frame>>& bits, 
                                          stringstream& ss, 
                                          bool& first,
                                          int& nframe) const {
    const int numChannel = bits.size();
    // write header if this is the first event
    if (first) {
      ss << header(numChannel);
      first = false;
    }
    bool firstEventFrame = true;
    // write inter-event 6 frame gap
    ss << infraGap(nframe, numChannel);
    for (int frame = 0; frame < setup_->numFramesIO(); frame++) {
      // write one frame for all channels
      ss << this->frame(nframe);
      for (int channel = 0; channel < numChannel; channel++) {
        const vector<Frame>& bvs = bits[channel];
        if (frame < (int)bvs.size())
          ss << hex(bvs[frame], firstEventFrame);
        else
          ss << hex(Frame(), firstEventFrame);
      }
      ss << endl;
      firstEventFrame = false;
    }
  }

  // write emp header
  string AnalyzerDemonstrator2::header(int numLinks) const {
    stringstream ss;
    // file header
    ss << "Board CMSSW" << endl
       << "Metadata: (strobe,) start of orbit, start of packet, end of packet, valid" << endl
       << endl;
    // link header
    ss << "      Link  ";
    for (int link = 0; link < numLinks; link++)
      ss << "            " << setfill('0') << setw(3) << link << "        ";
    ss << endl;
    return ss.str();
  }

  // write inter-packet 6 frame gap
  string AnalyzerDemonstrator2::infraGap(int& nFrame, int numLinks) const {
    stringstream ss;
    for (int gap = 0; gap < setup_->numFramesInfra(); gap++) {
      ss << frame(nFrame);
      for (int link = 0; link < numLinks; link++)
        ss << "  0000 " << string(TTBV::S_ / 4, '0');
      ss << endl;
    }
    return ss.str();
  }

  // creates frame number
  string AnalyzerDemonstrator2::frame(int& nFrame) const {
    stringstream ss;
    ss << "Frame " << setfill('0') << setw(4) << nFrame++ << "  ";
    return ss.str();
  }

  // converts bv into hex
  string AnalyzerDemonstrator2::hex(const Frame& bv, bool first) const {
    stringstream ss;
    ss << (first ? "  1001 " : "  0001 ") << setfill('0') << setw(TTBV::S_ / 4) << std::hex << bv.to_ullong();
    return ss.str();
  }

  // writes EMP-format output to a file on disk
  void AnalyzerDemonstrator2::write_output(stringstream& ss) const {
    fstream fs;
    fs.open("pre.txt", fstream::out);
    fs << ss.rdbuf();
    fs.close();
  }

}  // namespace trklet

DEFINE_FWK_MODULE(trklet::AnalyzerDemonstrator2);
