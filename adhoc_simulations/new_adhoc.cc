#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>
#include "pep-wifi-helper.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>


NS_LOG_COMPONENT_DEFINE ("NewAdhoc");

using namespace ns3;

void ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_DEBUG ("Received one packet!");
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  
  double rss = -80; // -dBm
  uint32_t packetSize = 100; // bytes
  uint32_t numPackets = 1000;
  double interval = 1.0; // seconds
  bool verbose = false;
  bool EnableCode=1;
  int symbols=1;
  int EnableRencode=0;
  int RelayActivity=10;
  int seed;
  int distance=0;
  
  CommandLine cmd;
  
  cmd.AddValue ("RelayActivity", "Number of symbols in each generation",RelayActivity);
  cmd.AddValue ("EnableCode", "enable ncoding 1, disable coding 0",EnableCode );
  cmd.AddValue ("Symbols", "Number of symbols in each generation",symbols);
  cmd.AddValue ("EnableRencode", "enable rencoding 1, disable rencoding 0",EnableRencode);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("distance", "distance ", distance);
  cmd.AddValue ("seed", "seed ", seed);

  cmd.Parse (argc, argv);

 SeedManager::SetSeed(seed);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));
 // Config::SetDefault ("ns3::PepWifiNetDevice::max_symbols", DoubleValue (2.0));



  NodeContainer c;
  c.Create (3);
  Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSsrc",
StringValue("1"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::MaxSlrc",
StringValue("1"));
  // The below set of helpers will help us to put together the wifi NICs we want
  PepWifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents (); // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  //wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  // wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel","m2",DoubleValue(0.025));
 
   wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel","m2",DoubleValue(0.025));
   //wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel");
   
   wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
   NS_LOG_DEBUG ("RelayActivity " <<  RelayActivity);
  //std::cout << "RelayActivity " << RelayActivity << endl;
 // int a=symbols;
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c, EnableCode, symbols , EnableRencode, RelayActivity);
  


  //wifi1->SetCode(true);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (500.0, 0.0, 0.0));
  positionAlloc->Add (Vector (-500, 0.0, 0.0));
  positionAlloc->Add (Vector (distance, 0,.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  NS_LOG_INFO ("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);


  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  // Output what we are doing
  NS_LOG_DEBUG ("Testing " << numPackets << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
