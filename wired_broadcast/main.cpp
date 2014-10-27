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

// This example shows how to use the Kodo library in a broadcast rlnc scenario
// within a ns-3 simulation. The code below is based on the wifi_broadcast
// example, which can be found in the ns-3-dev repository.

// In the script below the sender transmits encoded packets from a block of
// data to N receivers with the same packet erasure rate. The sender
// continues until all receivers have decoded all packets. Here the packets
// are sent using the binary field, GF(2) with a generation of 5 packets and
// 1000 (application) bytes and an erasure rate of 30% for all the nodes.

// You can change any parameter, by running (for example with a different
// generation size):
// ./build/linux/wired_broadcast/wired_broadcast --generationSize=GENERATION_SIZE

// When you are done, you will notice several pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try (for example) this:
//
// tcpdump -r wired-broadcast-rlnc-0-0.pcap -nn -tt

#include <ns3/point-to-point-star.h>
#include <ns3/internet-module.h>
#include <ns3/config-store-module.h>
#include <ns3/core-module.h>
#include <ns3/network-module.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#include "KodoSimulation.hpp" // Contains the simulation class

using namespace ns3;

int main (int argc, char *argv[])
{
  // Default values
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 5; // RLNC generation size
  double errorRate = 0.3; // Error rate for all the links
  uint32_t users = 2; // Number of users

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);
  cmd.AddValue ("users", "Number of receivers", users);

  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  // Set the basic helper for a single link
  PointToPointHelper pointToPoint;

  // N receivers against a centralized hub.
  PointToPointStarHelper star (users, pointToPoint);

  // Set error model for the net devices
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));

  std::vector<Ptr<RateErrorModel>> errorModel (users,
                                               CreateObject<RateErrorModel> ());

  for (uint32_t n = 0; n < users; n++)
  {
    errorModel[n]->SetAttribute ("ErrorRate", DoubleValue (errorRate));
    star.GetSpokeNode (n)->GetDevice (0)->
      SetAttribute ("ReceiveErrorModel", PointerValue (errorModel[n]));
    errorModel[n]->Enable ();
  }

  // Setting IP protocol stack
  InternetStackHelper internet;
  star.InstallStack(internet);

  // Set IP addresses
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  // Setting up application socket parameters for transmitter and
  // receiver sockets
  uint16_t port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  // Transmitter socket
  Ptr<Socket> source = Socket::CreateSocket (star.GetHub (), tid);

  // Transmitter socket connections. Set transmitter for broadcasting
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"),
                                                port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Receiver sockets
  std::vector<Ptr<Socket>> sinks (users);

  for (uint32_t n = 0; n < users; n++)
    {
      sinks[n] = Socket::CreateSocket (star.GetSpokeNode (n), tid);
    }

  // The field and traces types we will use. Here we consider GF(2). For GF(2^8)
  // just change "binary" for "binary8"
  typedef fifi::binary field;
  typedef kodo::disable_trace encoderTrace;
  typedef kodo::enable_trace decoderTrace;

  // Create the simulation
  KodoSimulation<field, encoderTrace, decoderTrace> kodoSimulator (
    users,
    generationSize,
    packetSize,
    sinks);

  // Receiver socket connections
  for (const auto sink : sinks)
    {
      sink->Bind (local);
      sink->SetRecvCallback (MakeCallback (
        &KodoSimulation <field, encoderTrace, decoderTrace>::ReceivePacket,
        &kodoSimulator));
    }

  // Turn on global static routing so we can actually be routed across the star
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes
  pointToPoint.EnablePcapAll ("wired-broadcast-rlnc");

  Simulator::ScheduleWithContext (
    source->GetNode ()->GetId (), Seconds (1.0),
    &KodoSimulation <field, encoderTrace, decoderTrace>::GenerateTraffic,
    &kodoSimulator,
    source,
    interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
