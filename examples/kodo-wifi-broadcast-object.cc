/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Steinwurf ApS
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
 */

// This example is a variation of the kodo-wifi-broadcast example. You can find
// the network topology and the common parameters in that example.
// The main difference is that the sender transmits encoded packets
// from multiple blocks of data. This input data can be a large buffer and
// it could even come from a file, but in this simulation we just use an
// object in memory. The "objectSize" parameter defines the size of this block.
//
// The input data is automatically split into multiple blocks
// and an encoder instance is created for each block. The sender iterates
// over these blocks in a round-robin fashion and it also sends some recovery
// packets for each block (this is controlled by the "extraPackets" parameter)
// to account for some lost packets. The sender continues until all receivers
// complete all the blocks, so the original object is available on all nodes.
//
// To change the number of recovery packets per block, you can set the
// extraPackets parameter of the simulation:
//
// python waf --run kodo-wifi-broadcast-object --command-template="%s --extraPackets=10"
//! [2]

#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>

#include "kodo-broadcast-object.h"
//! [3]
using namespace ns3;

int main (int argc, char *argv[])
{
  //! [4]
  std::string phyMode ("DsssRate1Mbps");
  // The default loss values yield 50% random packet loss
  // A different loss rate can be achieved by moving the lower and upper limits
  // relative to the detection threshold (T=112.0206).
  double minLoss = 112.0206 - 10.0;  // dBm
  double maxLoss = 112.0206 + 10.0;  // dBm
  uint32_t packetSize = 1000; // bytes
  double interval = 0.2; // seconds
  uint32_t generationSize = 20;
  uint32_t extraPackets = 5;
  uint32_t users = 2; // Number of users
  uint32_t objectSize = 100000; // bytes
  std::string field = "binary8"; // Finite field used

  // Create a map for the field values
  std::map<std::string, fifi::finite_field> fieldMap;
  fieldMap["binary"] = fifi::finite_field::binary;
  fieldMap["binary4"] = fifi::finite_field::binary4;
  fieldMap["binary8"] = fifi::finite_field::binary8;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("minLoss", "Lower bound for receiver random loss", minLoss);
  cmd.AddValue ("maxLoss", "Higher bound for receiver random loss", maxLoss);
  cmd.AddValue ("packetSize", "Size of UDP packets", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "The generation size of coded blocks",
                generationSize);
  cmd.AddValue ("users", "Number of receivers", users);
  cmd.AddValue ("objectSize", "Size of the object to transmit", objectSize);
  cmd.AddValue ("extraPackets", "Number of extra coded packets to send "
                "in each generation", extraPackets);
  cmd.AddValue ("field", "Finite field used", field);

  cmd.Parse (argc, argv);

  // Use the binary field in case of errors
  if (fieldMap.find (field) == fieldMap.end ())
    {
      field = "binary";
    }

  // Convert to time object
  Time interPacketInterval = Seconds (interval);
  //! [5]
  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
    StringValue ("2200"));

  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
    StringValue ("2200"));

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
    StringValue (phyMode));
  //! [6]
  // Source and destination
  NodeContainer nodes;
  nodes.Create (1 + users); // Sender + receivers

  // The below set of helpers will help us to put together the wifi NICs
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b); // OFDM at 2.4 GHz

  // The default error rate model is ns3::NistErrorRateModel
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11g
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // When using the RandomPropagationLossModel, the signal strength does not
  // depend on the distance between the two nodes
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();
  random->SetAttribute ("Min", DoubleValue (minLoss));
  random->SetAttribute ("Max", DoubleValue (maxLoss));
  wifiChannel.AddPropagationLoss ("ns3::RandomPropagationLossModel",
    "Variable",  PointerValue (random));
  wifiPhy.SetChannel (wifiChannel.Create ());
  //! [7]
  // Disable rate control
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    "DataMode", StringValue (phyMode), "ControlMode", StringValue (phyMode));

  // Set WiFi Mac type to adhoc mode
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // Create the net devices
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);
  //! [8]
  // Note that the positions are not relevant for the received signal strength.
  // However, they are required for the YansWiFiChannelHelper
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc =
    CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));  // Source node

  for (uint32_t n = 1; n <= users; n++)
    {
      positionAlloc->Add (Vector (5.0, 5.0*n, 0.0));
    }

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);
  //! [9]
  InternetStackHelper internet;
  internet.Install (nodes);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (devices);
  //! [10]
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // Transmitter socket
  Ptr<Socket> source = Socket::CreateSocket (nodes.Get (0), tid);

  // Receiver sockets
  std::vector<Ptr<Socket>> sinks (users);

  for (uint32_t n = 0; n < users; n++)
    {
      sinks[n] = Socket::CreateSocket (nodes.Get (1 + n), tid);
    }
  //! [11]
  // Creates the Broadcast helper for this broadcast topology
  BroadcastObject wifiBroadcast (fieldMap[field], users, objectSize,
      generationSize, packetSize, extraPackets, source, sinks);
  //! [12]
  // Transmitter socket connections. Set transmitter for broadcasting
  uint16_t port = 80;
  InetSocketAddress remote = InetSocketAddress (
    Ipv4Address ("255.255.255.255"), port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Receiver socket connections
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);
  for (const auto sink : sinks)
    {
      sink->Bind (local);
      sink->SetRecvCallback (MakeCallback (&BroadcastObject::ReceivePacket,
        &wifiBroadcast));
    }

  // Turn on global static routing so we can be routed across the network
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //! [13]

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0),
    &BroadcastObject::SendPacket, &wifiBroadcast, source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
  //! [14]
}
