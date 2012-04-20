/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

//
// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one packet of 1000
// (application) bytes to the other node.  The physical layer is configured
// to receive at a fixed RSS (regardless of the distance and transmit
// power); therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./waf --run "wifi-simple-adhoc --help"
//
// For instance, for this configuration, the physical layer will
// stop successfully receiving packets when rss drops below -97 dBm.
// To see this effect, try running:
//
// ./waf --run "wifi-simple-adhoc --rss=-97 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-98 --numPackets=20"
// ./waf --run "wifi-simple-adhoc --rss=-99 --numPackets=20"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./waf --run "wifi-simple-adhoc --verbose=1"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-adhoc-0-0.pcap -nn -tt
//

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/internet-module.h>
#include "pep-wifi-net-device.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ns3/propagation-loss-model.h>
NS_LOG_COMPONENT_DEFINE ("WifiSimpleAdhoc");


using namespace ns3;
int i = 0;
void ReceivePacket (Ptr<Socket> socket)
{
  cout << i++ << "\n";
  NS_LOG_UNCOND ("Received one packet!");
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                             uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize,pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{


  int N = atol (argv[1]); //read in run number from command line
  SeedManager::SetSeed (N);
  SeedManager::SetRun (1);
  UniformVariable x (0,10);
  ExponentialVariable y (2902);

  std::string phyMode ("DsssRate1Mbps");
  double rss = -50;  // -dBm
  uint32_t packetSize = 100; // bytes

  uint32_t numPackets = 2002;
  double interval = 5.00; // seconds
  bool verbose = false;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  // Convert to time object
  Time interPacketInterval = Seconds (interval);

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer c;
  int N2 = atol (argv[2]);
  c.Create (N2);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  /* YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
   // This is one parameter that matters when using FixedRssLossModel
   // set it to zero; otherwise, gain will be added
   //wifiPhy.Set ("RxGain", DoubleValue (0) );
   // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
   wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

   YansWifiChannelHelper wifiChannel;
   wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
   // The below FixedRssLossModel will cause the rss to be fixed regardless
   // of the distance between the two stations, and the transmit power
   wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
   wifiPhy.SetChannel (wifiChannel.Create ());*/

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // set it to zero; otherwise, gain will be added
//wifiPhy.Set ("RxGain", DoubleValue (-9.998) );
// ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  FriisPropagationLossModel loss;
  cout << loss.GetSystemLoss  ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel","m2",DoubleValue (0.024));
//wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel","m2",DoubleValue(10));
//wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel","SystemLoss",DoubleValue (21.55 ));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");

  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);
  //wifiChannel.AddPropagationLoss ("ns3::NakagamiPropagationLossModel","m2",DoubleValue(0.02));
// Ptr<NqosWifiMacHelper> wifiChannel2=c.Get (0)->GetObject<NqosWifiMacHelper> ();
//Ptr<WifiNetDevice> wifi2= c.Get (2)->GetObject<WifiNetDevice> (); // Get Ipv4 instance of the node
//wifi2->SetPhy  (&wifiPhy);

  Ptr<Node> node = c.Get (0); // Get pointer to ith node in container
  Ptr<PepWifiNetDevice> wifi1 = node->GetObject<PepWifiNetDevice> (); // Get Ipv4 instance of the node
//wifi1.GetPhy ();
//int a=1;
//wifi1->setcode(&a);

// node = c.Get (1); // Get pointer to ith node in container
//wifi1= node->GetObject<PepWifiNetDevice> (); // Get Ipv4 instance of the node

//wifi1->setcode(&a);

//Ipv4Address addr = ipv4->GetAddress (x, 0).GetLocal (); // Get
//Ipv4InterfaceAddress of xth interface.
// Note that with FixedRssLossModel, the positions below are not
// used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (500.0, 0.0, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  positionAlloc->Add (Vector (250, 2, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);



  InternetStackHelper internet;
  internet.Install (c);
  cout << "\n salam";
  cout << i;
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
//InetSocketAddress remote = InetSocketAddress (i.GetAddress (0, 0), 80);
  source->SetAllowBroadcast (true);

  source->Connect (remote);

  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  // Output what we are doing
  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (10000.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interPacketInterval);

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}

