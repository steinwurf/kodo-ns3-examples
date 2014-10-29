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

// This example shows how to use the Kodo library in a ns-3 simulation.
// The code below is based on the wifi-simple-adhoc example, which can
// be found here ns-3-dev/examples/wireless/wifi-simple-adhoc.cc in the
// ns-3 source code.

// In the script below the sender transmits encoded packets in a non-systematic
// way from a block of data to N receivers (N =  2 by default). The sender
// continues until the receiver has all the packets. The description below is
// from the original example, we modified it at bit fit our scenario.

// This script configures two nodes on a 802.11b physical layer, with
// 802.11b NICs in adhoc mode, and by default, sends one generation of
// 5 packets and 1000 (application) bytes to the other nodes. The physical
// layer is configured to receive at a fixed RSS (regardless of the distance
// and transmit power); therefore, changing position of the nodes has no effect.
//
// For instance, for this configuration, the physical layer will
// stop of successfully receiving packets when rss (receiver signal strength)
// drops below -96 dBm.

// To see this effect, try by changing the rss parameter on the simulation
// by typing ./build/linux/wifi_broadcast/wifi_broadcast --rss=-96
// With this value (or higher), the erasure rate goes to 1 and the packets can
// not be recovered.
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// When you are done, you will notice N + 1 trace files in your directory, one
// per device. You can review the files with Wireshark or tcpdump. If you have
// tcpdump installed, you can try this:
//
// tcpdump -r wifi-broadcast-rlnc-0-0.pcap -nn -tt (source node)

#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/config-store-module.h>
#include <ns3/wifi-module.h>
#include <ns3/internet-module.h>

#include <iostream>
#include <vector>
#include <string>
#include <ctime>

#include "../broadcast-rlnc.h" // Contains the broadcast topology class

using namespace ns3;

int main (int argc, char *argv[])
{
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

  // disable fragmentation for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold",
                      StringValue ("2200"));

  // turn off RTS/CTS for frames below 2200 bytes
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                      StringValue ("2200"));

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  // Source and destination
  NodeContainer c;
  c.Create (1 + users); // Sender + receivers

  // The below set of helpers will help us to put together the wifi NICs we want
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

  // Add a non-QoS upper mac, and disable rate control
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  // Set WiFi type and configuration parameters for MAC
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");

  // Create the net devices
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

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

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  uint16_t port = 80;

  // Transmitter socket
  Ptr<Socket> source = Socket::CreateSocket (c.Get (0), tid);

  // Transmitter socket connections. Set transmitter for broadcasting
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Receiver sockets
  std::vector<Ptr<Socket>> sinks (users);

  for (uint32_t n = 0; n < users; n++)
    {
      sinks[n] = Socket::CreateSocket (c.Get (1+n), tid);
    }

  // The field and traces types we will use. Here we consider GF(2). For GF(2^8)
  // just change "binary" for "binary8"
  using field = fifi::binary;
  using encoderTrace = kodo::disable_trace;
  using decoderTrace = kodo::enable_trace;

  // Creates the broadcast topology class for the current example
  BroadcastRlnc<field, encoderTrace, decoderTrace> wifiBroadcast (
    users,
    generationSize,
    packetSize,
    sinks);

  // Receiver socket connections
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);
  for (const auto sink : sinks)
    {
      sink->Bind (local);
      sink->SetRecvCallback (MakeCallback (
        &BroadcastRlnc <field, encoderTrace, decoderTrace>::ReceivePacket,
        &wifiBroadcast));
    }

  // Turn on global static routing so we can actually be routed across the star
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Pcap tracing
  wifiPhy.EnablePcap ("wifi-broadcast-rlnc", devices);

  Simulator::ScheduleWithContext (
    source->GetNode ()->GetId (), Seconds (1.0),
    &BroadcastRlnc <field, encoderTrace, decoderTrace>::SendPacket,
    &wifiBroadcast,
    source,
    interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}