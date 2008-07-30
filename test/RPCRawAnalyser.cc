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

#include "EventFilter/RPCRawToDigi/interface/RPCRawDataCounts.h"
#include "EventFilter/RPCRawToDigi/interface/RPCRecordFormatter.h"
#include "EventFilter/RPCRawToDigi/interface/RPCRawSynchro.h"

#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/ESWatcher.h"
#include "CondFormats/RPCObjects/interface/RPCReadOutMapping.h"
#include "CondFormats/RPCObjects/interface/RPCEMap.h"
#include "CondFormats/DataRecord/interface/RPCEMapRcd.h"


#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TObjArray.h"

typedef uint64_t Word64;
using namespace rpcrawtodigi;
using namespace std;
using namespace edm;

class RPCRawAnalyser : public edm::EDAnalyzer {
public:
  
  explicit RPCRawAnalyser( const edm::ParameterSet& cfg) : theConfig(cfg), theCabling(0) {}
  virtual ~RPCRawAnalyser();

  virtual void beginJob( const edm::EventSetup& );
  virtual void endJob();

  /// get data, convert to digis attach againe to Event
  virtual void analyze(const edm::Event&, const edm::EventSetup&);

private:
  edm::ParameterSet theConfig;
  TObjArray hList;
  const RPCReadOutMapping* theCabling;
  RPCRawDataCounts theCounts;
  RPCRawSynchro theSynchro; 
};

RPCRawAnalyser::~RPCRawAnalyser() { LogTrace("") << "RPCRawAnalyser destructor"; }

void RPCRawAnalyser::beginJob( const edm::EventSetup& )
{
}

void RPCRawAnalyser::endJob()
{
  TFile f("analysis.root","RECREATE");

  TH1D h1 = theCounts.recordTypeHisto(790);
  TH1D h2 = theCounts.recordTypeHisto(791);
  TH1D h3 = theCounts.recordTypeHisto(792);
  h1.Write();
  h2.Write();
  h3.Write();
  TH1D e = theCounts.readoutErrorHisto();
  e.Write();

  
  TH1D hBX("hBX","hBX",8,-3.5,4.5);
  std::string str;
  for (int dcc=790; dcc <=792; ++dcc) {
    str  += theSynchro.dumpDccBx(dcc,&hBX);
  }
  edm::LogInfo("")<< str;

  TH2D hBX2("hBX2","hBX2",80,-3.5,4.5, 30, 0.,3.);
  edm::LogInfo("")<< theSynchro.dumpDelays(theCabling, &hBX2);


  hBX.Write();
  hBX2.Write();

//  std::ostringstream str;
//  edm::LogError("") << theCounts.print();
  f.Close();
  edm::LogInfo(" END JOB, histos saved!");
}


void RPCRawAnalyser::analyze(const  edm::Event& ev, const edm::EventSetup& es) 
{
  edm::Handle<RPCRawDataCounts> rawCounts;
  ev.getByType( rawCounts);
  edm::Handle<RPCRawSynchro::ProdItem> synchroCounts;
  ev.getByType(synchroCounts);

  static edm::ESWatcher<RPCEMapRcd> recordWatcher;
  if (recordWatcher.check(es)) {
    delete theCabling;
    ESHandle<RPCEMap> readoutMapping;
    es.get<RPCEMapRcd>().get(readoutMapping);
    theCabling = readoutMapping->convert();
    LogTrace("") <<" READOUT MAP VERSION: " << theCabling->version() << endl;
  }

  LogInfo("") << "RPC RawAnalyser - End of Event";
  const RPCRawDataCounts * aCounts = rawCounts.product();
  theCounts += *aCounts;
  theSynchro.add( *synchroCounts.product());
}



#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(RPCRawAnalyser);

