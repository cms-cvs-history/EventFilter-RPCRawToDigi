/** \class RPCRawDumper_H
 **  Plug-in module that dump raw data file 
 **/

#include <fstream>
#include <iostream>
#include <iomanip>
#include <bitset>

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/Common/interface/Handle.h"

#include "FWCore/ParameterSet/interface/InputTag.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"

#include "DataFormats/FEDRawData/interface/FEDHeader.h"
#include "DataFormats/FEDRawData/interface/FEDTrailer.h"
#include "DataFormats/FEDRawData/interface/FEDNumbering.h"

#include "EventFilter/RPCRawToDigi/interface/RPCRecordFormatter.h"


#include "TH1D.h"
#include "TFile.h"
#include "TObjArray.h"

typedef uint64_t Word64;
using namespace rpcrawtodigi;
using namespace std;
using namespace edm;

class RPCRawDumper : public edm::EDAnalyzer {
public:
  
  explicit RPCRawDumper( const edm::ParameterSet& cfg) : theConfig(cfg), nEvents(0), nProblems(0) {}
  virtual ~RPCRawDumper();

  virtual void beginJob( const edm::EventSetup& );
  virtual void endJob();

  /// get data, convert to digis attach againe to Event
  virtual void analyze(const edm::Event&, const edm::EventSetup&);

private:
  edm::ParameterSet theConfig;
  TObjArray hList;
  unsigned int nEvents;
  unsigned int nProblems;
  typedef std::map<std::pair<int,int>,std::vector<int> > BXCountMap;
  BXCountMap bxCounts; 
};

RPCRawDumper::~RPCRawDumper() { LogTrace("") << "RPCRawDumper destructor"; }

void RPCRawDumper::beginJob( const edm::EventSetup& )
{
  hList.Add(new TH1D("hDcc790","hDcc790",9, 0.5,9.5));
  hList.Add(new TH1D("hDcc791","hDcc791",9, 0.5,9.5));
  hList.Add(new TH1D("hDcc792","hDcc792",9, 0.5,9.5));
  hList.SetOwner();


  for (int dcc=790; dcc<=792; dcc++) {
    for (int rmb=0; rmb <=35; rmb++)  bxCounts[make_pair(dcc,rmb)]=vector<int>(8,0);
  }
}

void RPCRawDumper::endJob()
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
  for (BXCountMap::const_iterator im=bxCounts.begin(); im!= bxCounts.end(); ++im) {
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
}


void RPCRawDumper::analyze(const  edm::Event& ev, const edm::EventSetup& es) 
{
  nEvents++;
  edm::Handle<FEDRawDataCollection> buffers;
  static edm::InputTag label = theConfig.getParameter<edm::InputTag>("InputLabel");
  ev.getByLabel( label, buffers);

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
      }
    }
  }
  LogTrace("") << "End of Event";
}



#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(RPCRawDumper);

