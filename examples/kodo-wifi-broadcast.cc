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

// This example shows how to use the Kodo library in a ns-3 simulation.
// The code below is inspired by the wifi-simple-adhoc example
// (ns-3-dev/examples/wireless/wifi-simple-adhoc.cc).
//
// In this scenario, the sender transmits encoded packets from a single
// block of data to N receivers (N=2 by default).
// The sender continues until each receiver has all the packets.
//
// We define N + 1 nodes using the 802.11b physical layer in adhoc mode.
// By default, the sender transmits one generation of 5 packets and 1000
// (application) bytes to the other nodes.
//
// The considered topology is the following:
//! [0]
//                             +-------------------+
//                             |  Encoder (Node 0) |
//                             |                   |
//                             |    Net Device 1   |
//                             |    IP: 10.1.1.1   |
//                             |                   |
//                             |        +---+      |
//                             |        |   |      |
//                             +--------+-+-+------+
//                                        |
//                                        |
//                  +---------------------v--------------------+
//                  |                                          |
//                  |  WiFi Standard: 802.11b                  |
//                  |  Modulation: DSSS 1Mbps at 2.4 GHz       |
//                  |  WiFi Channel: YansWiFi                  |
//                  |  Propagation: Random loss                |
//                  |  Delay: Constant                         |
//                  |  WiFi MAC: Ad-Hoc                        |
//                  |  RTS / CTS threshold: 2200 bytes         |
//                  |  Framentation threshold: 2200 bytes      |
//                  |  Non-unicast data rate: Same as unicast  |
//                  |                                          |
//                  +--------+--------------------------+------+
//                           |                          |
//                           |  loss                    |  loss
//                +--------+-v-+-------+     +--------+-v-+---------+
//                |        |   |       |     |        |   |         |
//                |        +---+       |  .. |        +---+         |
//                |                    |  .. |                      |
//                | Decoder 1 (Node 1) |     | Decoder N (Node N+1) |
//                |                    |     |                      |
//                |    Net Device 1    |     |    Net Device 1      |
//                |    IP: 10.1.1.2    |     |    IP: 10.1.1.N+1    |
//                +--------------------+     +----------------------+
//
//                              N: number of decoders
//! [1]
//
// The wifi channel uses the RandomPropagationLossModel with a
// UniformRandomVariable that can be configured with the minLoss/maxLoss
// parameters. Note that changing the position of the nodes has no effect.
//
// When setting a loss value, you need to consider the relevant values of the
// physical layer implementation. These are listed in the "Detailed Description"
// section here: https://www.nsnam.org/doxygen/classns3_1_1_wifi_phy.html
//
// EnergyDetectionThreshold: -96 dBm
// TxPowerStart/TxPowerEnd: 16.0206 dBm
//
// The TxPower starts at 16.0206 dBm and the transmisson can be successfully
// received if the signal strength stays above the -96 dBm the threshold.
// If we apply 112.0206 dBm signal loss, then we reach that threshold. If
// a lower signal loss value is set, then all transmissions will be received.
// But with a higher value, all packets will be dropped. We use a random
// variable that covers a range around this threshold to simulate a randomized
// loss pattern.
//
// You can lower the effective packet loss rate by decreasing the minLoss
// parameter of the simulation:
//
// python waf --run kodo-wifi-broadcast --command-template="%s --minLoss=90"
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

#include "kodo-broadcast.h"
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
  double interval = 1.0; // seconds
  uint32_t generationSize = 5;
  uint32_t users = 2; // Number of users
  std::string field = "binary"; // Finite field used

  // Create a map for the field values
  std::map<std::string, fifi::api::field> fieldMap;
  fieldMap["binary"] = fifi::api::field::binary;
  fieldMap["binary4"] = fifi::api::field::binary4;
  fieldMap["binary8"] = fifi::api::field::binary8;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("minLoss", "Lower bound for receiver random loss", minLoss);
  cmd.AddValue ("maxLoss", "Higher bound for receiver random loss", maxLoss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("users", "Number of receivers", users);
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
  Broadcast wifiBroadcast (fieldMap[field], users, generationSize, packetSize,
      source, sinks);
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
      sink->SetRecvCallback (MakeCallback (&Broadcast::ReceivePacket,
        &wifiBroadcast));
    }

  // Turn on global static routing so we can be routed across the network
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //! [13]
  // Pcap tracing
  // wifiPhy.EnablePcap ("kodo-wifi-broadcast", devices);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0),
    &Broadcast::SendPacket, &wifiBroadcast, source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
  //! [14]
}
