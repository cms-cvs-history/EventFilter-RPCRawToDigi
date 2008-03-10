/** \class RPCRawDumper_H
 *  Plug-in module that dump raw data file 
 *  for pixel subdetector
 */

#include <fstream>
#include <iostream>
#include <iomanip>
#include <bitset>

#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/Common/interface/Handle.h"


#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "DataFormats/FEDRawData/interface/FEDRawDataCollection.h"
#include "DataFormats/FEDRawData/interface/FEDRawData.h"

#include "DataFormats/FEDRawData/interface/FEDHeader.h"
#include "DataFormats/FEDRawData/interface/FEDTrailer.h"

#include "DataFormats/FEDRawData/interface/FEDNumbering.h"

#include "EventFilter/RPCRawToDigi/interface/RPCRecordFormatter.h"
//.#include "EventFilter/RPCRawToDigi/interface/EventRecords.h"
//.#include "EventFilter/RPCRawToDigi/interface/LBRecord.h"


//.#include "TH1D.h"
//.#include "TFile.h"

typedef uint64_t Word64;
using namespace rpcrawtodigi;
using namespace std;

class RPCRawDumper : public edm::EDAnalyzer {
public:
  
  explicit RPCRawDumper( const edm::ParameterSet& cfg) : theConfig(cfg) {}
  virtual ~RPCRawDumper();

  virtual void beginJob( const edm::EventSetup& ) {}
  virtual void endJob() {}

  /// get data, convert to digis attach againe to Event
  virtual void analyze(const edm::Event&, const edm::EventSetup&);

private:
  edm::ParameterSet theConfig;
};

RPCRawDumper::~RPCRawDumper()
{
  LogTrace("") << "RPCRawDumper destructor";
}

void RPCRawDumper::analyze(const  edm::Event& ev, const edm::EventSetup& es) 
{
  edm::Handle<FEDRawDataCollection> buffers;
  static std::string label = theConfig.getUntrackedParameter<std::string>("InputLabel","source");
  static std::string instance = theConfig.getUntrackedParameter<std::string>("InputInstance","");
  ev.getByLabel( label, instance, buffers);

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
      if ( !fedHeader.check() ) break; // throw exception?
      if ( fedHeader.sourceID() != fedId) throw cms::Exception("PROBLEM with header!");
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
      if ( !fedTrailer.check()) { trailer++; break; } // throw exception?
      if ( fedTrailer.lenght()!= nWords) throw cms::Exception("PROBLEM with trailer!!");
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

        if (event.complete()) {
          LogTrace(" ")
             << "dccId: "<<fedId
             << " dccInputChannelNum: " <<event.recordSLD().rmb()
             << " tbLinkInputNum: "<<event.recordSLD().tbLinkInputNumber()
             << " lbNumInLink: "<<event.recordCD().chamber()
             << " partition "<<event.recordCD().partitionNumber()
             << " cdData "<<event.recordCD().partitionData();
        }
      }
    }
  }
  LogTrace("") << "End of Event";
}



#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(RPCRawDumper);

