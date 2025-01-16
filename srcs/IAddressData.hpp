#ifndef IADDRESS_DATA_HPP
#define IADDRESS_DATA_HPP

#include <netinet/in.h>

//IAddressDataクラス=InterfaceAddressDadaクラス
class IAddressData {
 public:
  virtual ~IAddressData() {}
  virtual void set_address_data() = 0;
};

#endif
