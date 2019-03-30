#ifndef __CAEN_MANAGER__
#define __CAEN_MANAGER__


#include <CAENDigitizerType.h>


class caen_manager {
public:

  caen_manager( const int linknumber);
    
  ~caen_manager();

  int SetChannelDCOffset ( const unsigned int value);
  int SetChannelDCOffset ( const unsigned int index, const unsigned int value);
  int GetChannelDCOffset(const unsigned int channel) const;


  int SetMaxNumEventsBLT ( const unsigned int i);
  unsigned int GetMaxNumEventsBLT () const;

  int SetExtTriggerInputMode ( const unsigned int i);
  int GetExtTriggerInputMode () const;


  int SetTriggerPolarity ( const int pol);
  int SetTriggerPolarity ( const unsigned int channel, const int i);
  int GetTriggerPolarity (const unsigned int channel) const;
 
  int SetFastTriggerDigitizing(const int i);
  int GetFastTriggerDigitizing() const;

  int SetFastTriggerMode(const int i);
  int GetFastTriggerMode() const;

  int SetOutputSignalMode (const int i);
  int GetOutputSignalMode () const;

  int SetDRS4SamplingFrequency (const int i);
  int GetDRS4SamplingFrequency () const;

  int SetRecordLength (const int i);
  unsigned int GetRecordLength () const;

  int SetPostTriggerSize(const unsigned int i);
  int GetPostTriggerSize() const;

  int SetIOLevel(const int i);
  int GetIOLevel() const;

  int SetAcquisitionMode(const int i);
  int GetAcquisitionMode() const;

  int SetGroupEnableMask(const int i);
  unsigned int GetGroupEnableMask() const;

  int LoadDRS4CorrectionData(const int i);
  // ? not available int GetDRS4CorrectionData()const;

  int EnableDRS4Correction(const int i);
  // not available int GetDRS4Correction() const;

  int WriteRegister (const unsigned int adr, const unsigned int value);
  int ReadRegister (const unsigned int adr);

  void identify(std::ostream& os = std::cout) const;

  int init();
  
  int GetStatus() const { return _broken;};
 protected:

  int ClearConfigRegisterBit( const int bit);
  int SetConfigRegisterBit( const int bit);
  float getGS() const;

  int _broken;
  int handle;

  int _linknumber;

};


#endif
