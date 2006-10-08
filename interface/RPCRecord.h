#ifndef RPCRecord_h
#define RPCRecord_h

/** \class RPCRecord
 *
 *  Class that finds out the type of RPC record,
 *  possible types are: 
 *      StartOfBXData,
 *    	StartOfTbLinkInputNumberData,
 *   	LinkBoardData,
 *      RMBDisabled,
 *    	EmptyWord,
 *      RMBDiscarded,
 *	RMBCorrupted,
 *	DCCDiscarded,
 *      UndefinedType.
 *
 *  $Date: 2006/05/29 08:38:38 $
 *  $Revision: 1.7 $
 *  \author Ilaria Segoni
  */

class RPCRecord {

public:
  enum recordTypes {
        StartOfBXData = 1, 
    	StartOfTbLinkInputNumberData = 2,
    	LinkBoardData = 3,
    	EmptyWord     = 4,
        RMBDiscarded  = 5,
	RMBCorrupted  = 6,
	DCCDiscarded  = 7,
	RMBDisabled   = 8,
        UndefinedType = 9
  };
  
  /// Constructor
  RPCRecord(const unsigned char* index, bool printout,enum RPCRecord::recordTypes previousRecord): 
    word_(reinterpret_cast<const unsigned int*>(index)), verbosity(printout){
    oldRecord=previousRecord;};

  /// Destructor
  virtual ~RPCRecord() {};

  /// List of RPC RecordTypes

  /// Computes record type using pointer to beginning of buffer (* word_)
  enum recordTypes computeType(); 
  /// Record type getter 
  enum recordTypes type(); 
   /// Buffer pointer getter 
  const unsigned int * buf(); 
  
  /// Check that  StartOfBXData is followed by  StartOfTbLinkInputNumberData and that StartOfTbLinkInputNumberData
  /// is followed by LinkBoardData
  bool check(); 
    
  /// Control bits definitions
  static const int MaxLBFlag       = 2;
  static const int controlWordFlag = 3;
  
  static const int BXFlag                  = 1;
  static const int StartOfLBInputDataFlag  = 7;
  static const int EmptyOrDCCDiscardedFlag = 5;
  static const int RMBDiscardedDataFlag    = 6;
  static const int RMBCorruptedDataFlag    = 4;
  static const int RMBDisabledDataFlag     = 161;

  static const int EmptyWordFlag    = 0;
  static const int DCCDiscardedFlag = 1;
 
  static const int RPC_RECORD_BIT_SIZE = 16;

  static const int RECORD_TYPE_MASK  = 0X3;
  static const int RECORD_TYPE_SHIFT = 14;
 
  static const int BX_TYPE_MASK  = 0X3;
  static const int BX_TYPE_SHIFT = 12;

  static const int CONTROL_TYPE_MASK  = 0X7;
  static const int CONTROL_TYPE_SHIFT = 11;

  static const int EMPTY_OR_DCCDISCARDED_MASK  = 0X1;
  static const int EMPTY_OR_DCCDISCARDED_SHIFT = 0;

  static const int RMB_DISABLED_MASK  = 0X8;
  static const int RMB_DISABLED_SHIFT = 6;
private:

  const unsigned int * word_;
  bool verbosity;
  enum recordTypes oldRecord;
  enum recordTypes wordType;

};




#endif
