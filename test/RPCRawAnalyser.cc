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


#include "TH1D.h"
#include "TFile.h"
#include "TObjArray.h"

typedef uint64_t Word64;
using namespace rpcrawtodigi;
using namespace std;
using namespace edm;

class RPCRawAnalyser : public edm::EDAnalyzer {
public:
  
  explicit RPCRawAnalyser( const edm::ParameterSet& cfg) : theConfig(cfg) {}
  virtual ~RPCRawAnalyser();

  virtual void beginJob( const edm::EventSetup& );
  virtual void endJob();

  /// get data, convert to digis attach againe to Event
  virtual void analyze(const edm::Event&, const edm::EventSetup&);

private:
  edm::ParameterSet theConfig;
  TObjArray hList;
  RPCRawDataCounts theCounts;
};

RPCRawAnalyser::~RPCRawAnalyser() { LogTrace("") << "RPCRawAnalyser destructor"; }

void RPCRawAnalyser::beginJob( const edm::EventSetup& )
{
}

void RPCRawAnalyser::endJob()
{
  std::ostringstream str;
  TFile f("dccEvents.root","RECREATE");
  TH1D h1 = theCounts.recordTypeHisto(790);
  TH1D h2 = theCounts.recordTypeHisto(791);
  TH1D h3 = theCounts.recordTypeHisto(792);
  h1.Write();
  h2.Write();
  h3.Write();
  TH1D e = theCounts.readoutErrorHisto();
  e.Write();
  f.Close();
  edm::LogError("") << theCounts.print();
  edm::LogInfo(" END JOB, histos saved!");
}


void RPCRawAnalyser::analyze(const  edm::Event& ev, const edm::EventSetup& es) 
{
  edm::Handle<RPCRawDataCounts> counts;
  ev.getByType( counts);
  LogInfo("") << "RPC RawAnalyser - End of Event";
  const RPCRawDataCounts * aCounts = counts.product();
  theCounts += *aCounts;
}



#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(RPCRawAnalyser);

