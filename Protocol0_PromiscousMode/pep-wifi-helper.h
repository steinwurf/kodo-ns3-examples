
#ifndef PEP_WIFI_HELPER_H
#define PEP_WIFI_HELPER_H

#include <string>
#include <ns3/attribute.h>
#include <ns3/object-factory.h>
#include <ns3/node-container.h>
#include <ns3/wifi-helper.h>
#include "pep-wifi-net-device.h"
#include <ns3/net-device-container.h>
#include <ns3/wifi-phy-standard.h>
#include <ns3/trace-helper.h>



namespace ns3 {

class PepWifiHelper: public WifiHelper

{
public:
  
  PepWifiHelper ();
  
public:
  // From WifiHelper
  virtual void SetRemoteStationManager (std::string type,
                                std::string n0 = "", const AttributeValue &v0 = EmptyAttributeValue (),
                                std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
                                std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
                                std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
                                std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
                                std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
                                std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
                                std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue ());
  virtual void SetStandard (enum WifiPhyStandard standard);
  static void EnableLogComponents (void);
  
public:
  NetDeviceContainer Install (const WifiPhyHelper &phy, const WifiMacHelper &mac, NodeContainer c, int code, int symbols,int recode,int RelayActivity) const;

private:
  ObjectFactory m_stationManager;
  enum WifiPhyStandard m_standard;

};

}
#endif
