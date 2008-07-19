/** \class RPCRawSynchro_H
 **  Plug-in module that dump raw data file 
 **/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <bitset>
#include <cmath>

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESWatcher.h"

#include "FWCore/ParameterSet/interface/InputTag.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"

#include "DataFormats/FEDRawData/interface/FEDHeader.h"
#include "DataFormats/FEDRawData/interface/FEDTrailer.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"

#include "EventFilter/RPCRawToDigi/interface/RPCRecordFormatter.h"
#include "CondFormats/RPCObjects/interface/RPCReadOutMapping.h"
#include "CondFormats/RPCObjects/interface/LinkBoardElectronicIndex.h"

#include "CondFormats/RPCObjects/interface/RPCEMap.h"
#include "CondFormats/DataRecord/interface/RPCEMapRcd.h"



#include "TH1D.h"
#include "TFile.h"
#include "TObjArray.h"

typedef uint64_t Word64;
using namespace rpcrawtodigi;
using namespace std;
using namespace edm;


struct lessLB {
  bool operator () (const LinkBoardElectronicIndex& lb1, const LinkBoardElectronicIndex& lb2) const {
    if (lb1.dccId < lb2.dccId) return true; 
    if (    (lb1.dccId==lb2.dccId) 
         && (lb1.dccInputChannelNum< lb2.dccInputChannelNum) ) return true; 
    if (    (lb1.dccId==lb2.dccId) 
         && (lb1.dccInputChannelNum == lb2.dccInputChannelNum) 
         && (lb1.tbLinkInputNum < lb2.tbLinkInputNum ) ) return true;
    if (    (lb1.dccId==lb2.dccId) 
         && (lb1.dccInputChannelNum == lb2.dccInputChannelNum) 
         && (lb1.tbLinkInputNum == lb2.tbLinkInputNum ) 
         && (lb1.lbNumInLink < lb2.lbNumInLink) ) return true;
    return false;
  }
};

struct lessVectSum {
  bool operator () (const pair<unsigned int, unsigned int> & o1,
                    const pair<unsigned int, unsigned int> & o2) {
    return o1.second < o2.second;
  };
}; 

class RPCRawSynchro : public edm::EDAnalyzer {
public:
  
  explicit RPCRawSynchro( const edm::ParameterSet& cfg) : theConfig(cfg), nEvents(0), nProblems(0) {}
  virtual ~RPCRawSynchro();

  virtual void beginJob( const edm::EventSetup& );
  virtual void endJob();

  /// get data, convert to digis attach againe to Event
  virtual void analyze(const edm::Event&, const edm::EventSetup&);

private:
  int bxDifference(const EventRecords & ev) const;
  edm::ParameterSet theConfig;
  TObjArray hList;
  unsigned int nEvents;
  unsigned int nProblems;
  typedef std::map<std::pair<int,int>,std::vector<int> > BXRMB_CountMap;
  BXRMB_CountMap bxCounts; 
  typedef std::map<LinkBoardElectronicIndex, std::vector<int>, lessLB > BXLB_CountMap;
  BXLB_CountMap lbCounts;
  const RPCReadOutMapping* theCabling ;
};

RPCRawSynchro::~RPCRawSynchro() { LogTrace("") << "RPCRawSynchro destructor"; }

void RPCRawSynchro::beginJob( const edm::EventSetup& )
{
  theCabling = 0;

  hList.Add(new TH1D("hDcc790","hDcc790",9, 0.5,9.5));
  hList.Add(new TH1D("hDcc791","hDcc791",9, 0.5,9.5));
  hList.Add(new TH1D("hDcc792","hDcc792",9, 0.5,9.5));
  hList.SetOwner();


  for (int dcc=790; dcc<=792; dcc++) {
    for (int rmb=0; rmb <=35; rmb++)  bxCounts[make_pair(dcc,rmb)]=vector<int>(8,0);
  }
}

void RPCRawSynchro::endJob()
{
  std::ostringstream str;
  
  for (int ih=0; ih<3; ih++) {
   string hname;
   if (ih==0) hname="hDcc790";   
   if (ih==1) hname="hDcc791";   
   if (ih==2) hname="hDcc792";   
   TH1* histo = static_cast<TH1*>(hList.FindObject(hname.c_str()));
   histo->SetEntries(nEvents);
   str<<hname<<" record count,";
   str<<"  BX: "<<setw(9)<<histo->GetBinContent(1);
   str<<"  SLD: "<<setw(9)<<histo->GetBinContent(2);
   str<<"  CD: "<<setw(9)<<histo->GetBinContent(3);
   str<<"  EMPTY: "<<setw(9)<<histo->GetBinContent(4);
   str<<"  RDDM: "<<histo->GetBinContent(5);
   str<<"  SDDM: "<<histo->GetBinContent(6);
   str<<"  RCDM: "<<histo->GetBinContent(7);
   str<<"  RDM: "<<histo->GetBinContent(8);
   str<<" unknown: "<<histo->GetBinContent(9);
   str<< std::endl;
  }
  str <<" OTHER PROBLEMS: " << nProblems << std::endl;
  TFile f("dccEvents.root","RECREATE");
  hList.Write();
  f.Close();
  for (BXRMB_CountMap::const_iterator im=bxCounts.begin(); im!= bxCounts.end(); ++im) {
    int totCount=0;
    for (unsigned int i=0; i<im->second.size(); i++) totCount +=  im->second[i];
    if (totCount>0) {
      str<<"dcc="<<setw(3)<<im->first.first<<" rmb="<<setw(3)<<im->first.second<<" counts: ";
      for (unsigned int i=0; i<im->second.size(); i++) str<<" "<<setw(6)<<im->second[i];
      str<<endl;
    }
  }
  edm::LogInfo("END OF JOB, HISTO DUMP")<<str.str();
  edm::LogInfo(" END JOB, histos saved!");

  
  std::ostringstream sss;
  edm::LogInfo("") <<" READOUT MAP VERSION: " << theCabling->version() << endl;

  vector< std::vector<int> > vectRes;
  vector< string > vectNamesCh;
  vector< string > vectNamesPart;
  vector< string > vectNamesLink;
  vector< float > vectMean;
  vector< pair<unsigned int, unsigned int> > vectSums;
  vector< float > vectRMS;
  
  vector<unsigned int> good, bad;

  for (BXLB_CountMap::const_iterator im = lbCounts.begin(); im != lbCounts.end(); ++im) {
    const LinkBoardSpec* linkBoard = theCabling->location(im->first);
    if (linkBoard==0) continue; 
    float sumW =0.; 
    unsigned  int stat = 0;
    for (unsigned int i=0; i<im->second.size(); i++) { 
      stat += im->second[i];
      sumW += i*im->second[i];
    }
    float mean = sumW/stat;
    float rms2 = 0.;
    for (unsigned int i=0; i<im->second.size(); i++) rms2 += im->second[i]*(mean-i)*(mean-i);

//    vector<string> chnames;
//    for (vector<FebConnectorSpec>::const_iterator ifs = linkBoard->febs().begin(); ifs !=  linkBoard->febs().end(); ++ifs) {
//      vector<string>::iterator immm = find(chnames.begin(),chnames.end(),ifs->chamber().chamberLocationName); 
//      if (immm == chnames.end()) chnames.push_back(ifs->chamber().chamberLocationName);
//    }
    const ChamberLocationSpec & chamber = linkBoard->febs().front().chamber();
    const FebLocationSpec & feb =  linkBoard->febs().front().feb(); 
    sss <<chamber.chamberLocationName 
        <<" "<< feb.localEtaPartition
        <<" mean: "<<mean <<" rms: " << sqrt(rms2/stat) 
        << im->first.print();
        for (unsigned int i=0; i<im->second.size(); i++) sss<<" "<<setw(6)<<im->second[i];
//      if (chnames.size() >1) { sss<<" *****"; for (unsigned int i=0; i<chnames.size();++i) sss<<" "<<chnames[i]; }
        sss<<std::endl;

    unsigned int idx = 0;
    while (idx < vectNamesCh.size()) { 
      if (vectNamesCh[idx] == chamber.chamberLocationName && vectNamesPart[idx] == feb.localEtaPartition) break; 
      idx++;
    }
    if (idx == vectNamesCh.size()) {

      vectRes.push_back(im->second);
      vectNamesCh.push_back(chamber.chamberLocationName);
      vectNamesPart.push_back(feb.localEtaPartition);
      vectNamesLink.push_back(im->first.print());
      vectSums.push_back(make_pair(idx,stat));
      vectMean.push_back(mean);
      vectRMS.push_back(sqrt(rms2/stat));
    }
  }
  edm::LogInfo("") << sss.str();

  std::ostringstream sss2;
  sss2 <<"GOING TO WRITE: " << endl;
  sort(vectSums.begin(), vectSums.end(), lessVectSum() );
//   sort(vectSums.begin(), vectSums.end());

//  std::<<"cout INDEX result: " << lbCounts[1].first << endl;
  for (vector<std::pair<unsigned int, unsigned int> >::const_iterator it = vectSums.begin();
       it != vectSums.end(); ++it) {

     unsigned int iindex = it->first;
  sss2 << vectNamesCh[iindex] <<" "<<
      vectNamesPart[iindex]  <<" mean: "<<vectMean[iindex]<<" rms: " 
          <<vectRMS[iindex] << vectNamesLink[iindex]; 
     for (unsigned int i=0;  i< vectRes[iindex].size(); ++i)
       sss2 <<" "<<setw(6)<<vectRes[iindex][i];
       
    sss2 <<endl;
       
  }
  edm::LogInfo("") << sss2.str(); 
  

}


void RPCRawSynchro::analyze(const  edm::Event& ev, const edm::EventSetup& es) 
{
  nEvents++;
  edm::Handle<FEDRawDataCollection> buffers;
  static edm::InputTag label = theConfig.getParameter<edm::InputTag>("InputLabel");
  ev.getByLabel( label, buffers);

  static edm::ESWatcher<RPCEMapRcd> recordWatcher;
  if (recordWatcher.check(es)) {
    delete theCabling;
    ESHandle<RPCEMap> readoutMapping;
    LogTrace("") << "record has CHANGED!!, initialise readout map!";
    es.get<RPCEMapRcd>().get(readoutMapping);
    theCabling = readoutMapping->convert();
    LogTrace("") <<" READOUT MAP VERSION: " << theCabling->version() << endl;
  }

  std::map<LinkBoardElectronicIndex, int, lessLB > tmpMap;

  FEDNumbering fednum;
  std::pair<int,int> fedIds = fednum.getRPCFEDIds();
  for (int fedId = fedIds.first; fedId <= fedIds.second; fedId++) {
    RPCRecordFormatter interpreter(fedId, 0) ;
    int currentBX =0;
    const FEDRawData & rawData = buffers->FEDData(fedId);
    int nWords = rawData.size()/sizeof(Word64);
    if (nWords==0) continue;

    //
    // check headers
    //
    const Word64* header = reinterpret_cast<const Word64* >(rawData.data()); header--;
    bool moreHeaders = true;
    while (moreHeaders) {
      header++;
      FEDHeader fedHeader( reinterpret_cast<const unsigned char*>(header));
      if (!fedHeader.check()) {
        LogError(" ** PROBLEM **, header.check() failed, break");
        nProblems++;
        break;
      }
      if ( fedHeader.sourceID() != fedId) {
        nProblems++;
        LogError(" ** PROBLEM **, fedHeader.sourceID() != fedId")
            << "fedId = " << fedId<<" sourceID="<<fedHeader.sourceID();
      }
      currentBX = fedHeader.bxID(); 
      moreHeaders = fedHeader.moreHeaders();
      {
          std::ostringstream str;
          str <<"  header: "<< *reinterpret_cast<const std::bitset<64>*> (header) << std::endl;
          str <<"  header triggerType: " << fedHeader.triggerType()<<std::endl;
          str <<"  header lvl1ID:      " << fedHeader.lvl1ID() << std::endl;
          str <<"  header bxID:        " << fedHeader.bxID() << std::endl;
          str <<"  header sourceID:    " << fedHeader.sourceID() << std::endl;
          str <<"  header version:     " << fedHeader.version() << std::endl;
          LogTrace("") << str.str();
      }
    }

    //
    // check trailers
    //
    const Word64* trailer=reinterpret_cast<const Word64* >(rawData.data())+(nWords-1); 
    trailer++;
    bool moreTrailers = true;
    while (moreTrailers) {
      trailer--;
      FEDTrailer fedTrailer(reinterpret_cast<const unsigned char*>(trailer));
      if ( !fedTrailer.check()) {
        LogError(" ** PROBLEM **, trailer.check() failed, break");
        nProblems++;
        break;
      }
      if ( fedTrailer.lenght()!= nWords) {
        LogError(" ** PROBLEM **, fedTrailer.lenght()!= nWords, break");
        nProblems++;
        break;
      }
      moreTrailers = fedTrailer.moreTrailers();
      {
        std::ostringstream str;
        str <<" trailer: "<<  *reinterpret_cast<const std::bitset<64>*> (trailer) << std::endl;
        str <<"  trailer lenght:    "<<fedTrailer.lenght()<<std::endl;
        str <<"  trailer crc:       "<<fedTrailer.crc()<<std::endl;
        str <<"  trailer evtStatus: "<<fedTrailer.evtStatus()<<std::endl;
        str <<"  trailer ttsBits:   "<<fedTrailer.ttsBits()<<std::endl;
        LogTrace("") << str.str();
      }
    }

    //
    // data records
    //
    {
      std::ostringstream str;
      for (const Word64* word = header+1; word != trailer; word++) {
        str<<"    data: "<<*reinterpret_cast<const std::bitset<64>*>(word) << std::endl;
      }
      LogTrace("") << str.str();
    }

    EventRecords event(currentBX);
    for (const Word64* word = header+1; word != trailer; word++) {
      for( int iRecord=1; iRecord<=4; iRecord++){
        typedef DataRecord::RecordType Record;
        const Record* pRecord = reinterpret_cast<const Record* >(word+1)-iRecord;
        DataRecord data(*pRecord);
        std::ostringstream str;
        str <<"record: "     <<data.print();
        str <<" hex: "       <<hex<<*pRecord;
        str <<" type:"<<data.type();
        str <<DataRecord::print(data);
        LogTrace("") << str.str();
        event.add(data);

        // check BX consistency
        if (data.type() == 1) {
          int nOrbits = 3564;
          int diffOrbits = event.triggerBx()-event.recordBX().bx();
          while (diffOrbits >  nOrbits/2) diffOrbits -=nOrbits; 
          while (diffOrbits < -nOrbits/2) diffOrbits +=nOrbits; 
          if( abs(diffOrbits) > 2) {
            LogError(" ** PROBLEM **, wrong BXes");
            nProblems++;
          }
        } 

        //FILL debug histo
        {
          std::ostringstream hname; hname<<"hDcc"<<fedId; 
          TH1* histo = static_cast<TH1*>(hList.FindObject(hname.str().c_str()) );
          histo->Fill(data.type());         
        }

        if (event.complete()) {
          LogTrace(" ")
             << "dccId: "<<fedId
             << " dccInputChannelNum: " <<event.recordSLD().rmb()
             << " tbLinkInputNum: "<<event.recordSLD().tbLinkInputNumber()
             << " lbNumInLink: "<<event.recordCD().lbInLink()
             << " partition "<<event.recordCD().partitionNumber()
             << " cdData "<<event.recordCD().partitionData();
        }
        if (event.complete()) {
         pair<int,int> key = make_pair(fedId, event.recordSLD().rmb());
          if( bxCounts.find(key) == bxCounts.end()) continue;
          vector<int> & v = bxCounts[key];
          int nOrbits = 3564;
          int bxDiff = event.recordBX().bx() - event.triggerBx() + 3;
          if (bxDiff >  nOrbits/2) bxDiff -=nOrbits;
          if (bxDiff < -nOrbits/2) bxDiff +=nOrbits;
          if (bxDiff < 0) continue;
          if (bxDiff >= static_cast<int>(v.size()) ) continue;
          v[bxDiff]++;
        }
        if (event.complete()) {
          LinkBoardElectronicIndex key;
          key.dccId = fedId;
          key.dccInputChannelNum = event.recordSLD().rmb();
          key.tbLinkInputNum = event.recordSLD().tbLinkInputNumber();
          key.lbNumInLink = event.recordCD().lbInLink();

          if(true) {
//          if (tmpMap.find(key) == tmpMap.end() ) { 
          tmpMap[key]=1; 

          if (lbCounts.find(key)==lbCounts.end()) lbCounts[key]=vector<int>(8,0);
          vector<int> & v =  lbCounts[key];
          int nOrbits = 3564;
          int bxDiff = event.recordBX().bx() - event.triggerBx() + 3;
          if (bxDiff >  nOrbits/2) bxDiff -=nOrbits;
          if (bxDiff < -nOrbits/2) bxDiff +=nOrbits;
          if (bxDiff < 0) continue;
          if (bxDiff >= static_cast<int>(v.size()) ) continue;
          v[bxDiff]++;
          }
        }
      }
    }
  }
  LogTrace("") << "End of Event";
}

int RPCRawSynchro::bxDifference(const EventRecords & event) const 
{
  int nOrbits = 3564;
  int diff = event.recordBX().bx() - event.triggerBx() + 3;
  if (diff >  nOrbits/2) diff -=nOrbits;
  if (diff < -nOrbits/2) diff +=nOrbits;
  return diff;
}



#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(RPCRawSynchro);

