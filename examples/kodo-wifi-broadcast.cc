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
// The code below is inspired from the wifi-simple-adhoc example, which can
// be found here ns-3-dev/examples/wireless/wifi-simple-adhoc.cc in the
// ns-3 source code.

// In the source code below the sender transmits encoded packets in a
// non-systematic way from a block of data to N receivers (2 by default).
// The sender continues until each receiver has all the packets.

// We consider N + 1 nodes on a 802.11b physical layer, with 802.11b net
// devices in adhoc mode, and by default, sends one generation of
// 5 packets and 1000 (application) bytes to the other nodes. The physical
// layer is configured to receive at a fixed rss (receiver signal strength)
// regardless of the distance and transmit power); therefore, changing position
// of the nodes has no effect.
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
//                  |  Propagation: Fixed propagation loss     |
//                  |  Delay: Constant                         |
//                  |  WiFi MAC: Ad-Hoc                        |
//                  |  RTS / CTS threshold: 2200 bytes         |
//                  |  Framentation threshold: 2200 bytes      |
//                  |  Non-unicast data rate: Same as unicast  |
//                  |                                          |
//                  +--------+--------------------------+------+
//                           |                          |
//                           |  rss                     |  rss
//                +--------+-v-+-------+     +--------+-v-+---------+
//                |        |   |       |     |        |   |         |
//                |        +---+       |  .. |        +---+         |
//                |                    |  .. |                      |
//                | Decoder 1 (Node 1) |     | Decoder N (Node N+1) |
//                |                    |     |                      |
//                |    Net Device 1    |     |    Net Device 1      |
//                |    IP: 10.1.1.2    |     |    IP: 10.1.1.N+1    |
//                +--------------------+     +----------------------+

//                N: number of decoders    rss: Received Signal Strength
//! [1]
// For instance, for this configuration, the physical layer will
// stop of successfully receiving packets when rss (receiver signal strength)
// drops below -96 dBm. This means that -96 dBm is the thresho

// To see this effect, try by changing the rss parameter on the simulation
// by typing:

// python waf --run kodo-wifi-broadcast --command-template="%s --rss=-96"

// With this value (or lower), the erasure rate goes to 1 and the packets can
// not be recovered. Higher rss power values ensure packet reception and
// decoding

// After running, you will notice N + 1 trace files in your directory, one
// per device. You can review the files with Wireshark or tcpdump. If you have
// tcpdump installed, you can try this:
//
// tcpdump -r wifi-broadcast-rlnc-0-0.pcap -nn -tt (source node)
//! [2]
// General comments: E-macs descriptor, ns-3 license and example description

// ns-3 includes
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>

// Simulation includes
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <ctime>

// Kodo includes
#include "broadcast-rlnc.h" // Contains the broadcast topology class
//! [3]
using namespace ns3;

int main (int argc, char *argv[])
{
  //! [4]
  std::string phyMode ("DsssRate1Mbps");
  double rss = -93;  // -dBm
  uint32_t packetSize = 1000; // bytes
  double interval = 1.0; // seconds
  uint32_t generationSize = 5;
  uint32_t users = 2; // Number of users

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("users", "Number of receivers", users);

  cmd.Parse (argc, argv);

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
  NodeContainer c;
  c.Create (1 + users); // Sender + receivers

  // The below set of helpers will help us to put together the wifi NICs we
  // want
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b); // OFDM at 2.4 GHz

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // The default error rate model is ns3::NistErrorRateModel

  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0));

  // ns-3 supports RadioTap and Prism tracing extensions for 802.11g
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",
    DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());
  //! [7]
  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
    "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));

  // Set WiFi type and configuration parameters for MAC
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // Create the net devices
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);
  //! [8]
  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength. However, they are required for the
  // YansWiFiChannelHelper
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
  mobility.Install (c);
  //! [9]
  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  ipv4.Assign (devices);
  //! [10]
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // Transmitter socket
  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);

  // Receiver sockets
  std::vector<Ptr<Socket>> sinks (users);

  for (uint32_t n = 0; n < users; n++)
    {
      sinks[n] = Socket::CreateSocket (c.Get (1+n), tid);
    }
  //! [11]
  // Creates the BroadcastRlnc helper for this broadcast topology
  BroadcastRlnc wifiBroadcast (kodo_full_vector, kodo_binary,
    users, generationSize, packetSize, source, sinks);

  // //! [12]
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
      sink->SetRecvCallback (MakeCallback (&BroadcastRlnc::ReceivePacket,
        &wifiBroadcast));
    }

  // Turn on global static routing so we can be routed across the network
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  //! [13]
  // Pcap tracing
  wifiPhy.EnablePcap ("wifi-broadcast-rlnc", devices);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0),
    &BroadcastRlnc::SendPacket, &wifiBroadcast, source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
  //! [14]
}