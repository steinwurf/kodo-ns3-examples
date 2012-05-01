
#include "pep-wifi-helper.h"
#include "pep-wifi-net-device.h"
#include <ns3/wifi-helper.h>
#include <ns3/wifi-mac.h>
#include <ns3/wifi-phy.h>
#include <ns3/wifi-remote-station-manager.h>
#include <ns3/wifi-channel.h>
#include <ns3/yans-wifi-channel.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/mobility-model.h>
#include <ns3/log.h>
#include <ns3/config.h>
#include <ns3/simulator.h>
#include <ns3/names.h>

NS_LOG_COMPONENT_DEFINE ("PepWifiHelper");

namespace ns3 {

PepWifiHelper::PepWifiHelper ()
  : m_standard (WIFI_PHY_STANDARD_80211a)
{
}


NetDeviceContainer
PepWifiHelper::Install (const WifiPhyHelper &phyHelper,
                     const WifiMacHelper &macHelper, NodeContainer c, int code, int symbols,int recode,int RelayActivity) const
{
  NetDeviceContainer devices;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      Ptr<Node> node;
      node = *i;
      Ptr<PepWifiNetDevice> device;
      device = CreateObject<PepWifiNetDevice> ();

      device->SetAttribute ("SymbolsNum", UintegerValue (symbols));
      device->SetAttribute ("EnableCode", UintegerValue (code));
      device->SetAttribute ("EnableRecode", UintegerValue (recode));
      device->SetAttribute ("RelayActivity", UintegerValue (RelayActivity));

      Ptr<WifiRemoteStationManager> manager;
      manager = m_stationManager.Create<WifiRemoteStationManager> ();
      Ptr<WifiMac> mac;
      mac = macHelper.Create ();
      Ptr<WifiPhy> phy;
      phy = phyHelper.Create (node, device);
      // set the promiscous mode in the interface
      //mac->SetPromisc();
      //give a new mac address
      mac->SetAddress (Mac48Address::Allocate ());
      //
      mac->ConfigureStandard (m_standard);
      phy->ConfigureStandard (m_standard);
      device->SetMac (mac);
      device->SetPhy (phy);
      device->SetRemoteStationManager (manager);
      node->AddDevice (device);
      devices.Add (device);
      NS_LOG_DEBUG ("node=" << node << ", mob=" << node->GetObject<MobilityModel> ());
    }
  return devices;
}
void
PepWifiHelper::SetRemoteStationManager (std::string type,
                                     std::string n0, const AttributeValue &v0,
                                     std::string n1, const AttributeValue &v1,
                                     std::string n2, const AttributeValue &v2,
                                     std::string n3, const AttributeValue &v3,
                                     std::string n4, const AttributeValue &v4,
                                     std::string n5, const AttributeValue &v5,
                                     std::string n6, const AttributeValue &v6,
                                     std::string n7, const AttributeValue &v7)
{
  m_stationManager = ObjectFactory ();
  m_stationManager.SetTypeId (type);
  m_stationManager.Set (n0, v0);
  m_stationManager.Set (n1, v1);
  m_stationManager.Set (n2, v2);
  m_stationManager.Set (n3, v3);
  m_stationManager.Set (n4, v4);
  m_stationManager.Set (n5, v5);
  m_stationManager.Set (n6, v6);
  m_stationManager.Set (n7, v7);
}

void
PepWifiHelper::SetStandard (enum WifiPhyStandard standard)
{
  m_standard = standard;
}
void
PepWifiHelper::EnableLogComponents (void)
{
  LogComponentEnable ("Aarfcd", LOG_LEVEL_ALL);
  LogComponentEnable ("AdhocWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("AmrrWifiRemoteStation", LOG_LEVEL_ALL);
  LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("ns3::ArfWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("Cara", LOG_LEVEL_ALL);
  LogComponentEnable ("DcaTxop", LOG_LEVEL_ALL);
  LogComponentEnable ("DcfManager", LOG_LEVEL_ALL);
  LogComponentEnable ("DsssErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("EdcaTxopN", LOG_LEVEL_ALL);
  LogComponentEnable ("InterferenceHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("Jakes", LOG_LEVEL_ALL);
  LogComponentEnable ("MacLow", LOG_LEVEL_ALL);
  LogComponentEnable ("MacRxMiddle", LOG_LEVEL_ALL);
  LogComponentEnable ("MsduAggregator", LOG_LEVEL_ALL);
  LogComponentEnable ("MsduStandardAggregator", LOG_LEVEL_ALL);  
  LogComponentEnable ("NistErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("OnoeWifiRemoteStation", LOG_LEVEL_ALL); 
  LogComponentEnable ("PropagationLossModel", LOG_LEVEL_ALL);
  LogComponentEnable ("RegularWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("RraaWifiManager", LOG_LEVEL_ALL);
  LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("SupportedRates", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPhyStateHelper", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiPhy", LOG_LEVEL_ALL);
  LogComponentEnable ("WifiRemoteStationManager", LOG_LEVEL_ALL);
  LogComponentEnable ("YansErrorRateModel", LOG_LEVEL_ALL);
  LogComponentEnable ("YansWifiChannel", LOG_LEVEL_ALL);
  LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);
}

} // namespace ns3
