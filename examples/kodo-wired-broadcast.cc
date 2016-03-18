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

// This example shows how to use the Kodo library in a broadcast rlnc scenario
// within a ns-3 simulation. The code below is inspired in the
// kodo-wifi-broadcast example, which can be found in the ns-3-dev repository.

// In this example, the sender transmits encoded packets from a block of
// data to N receivers with the same packet erasure rate (errorRate).
// The sender continues until all receivers have decoded all packets. By
// default, the packets are sent using the binary field, GF(2) with a
// generation of 5 packets and 1000 (application) bytes and an erasure rate
// of 30% (0.3) for all the nodes. Here we have set the number of
// receivers to 2 by default but it can changed.

// The considered topology is the following:
//! [0]
//                  +-----------------------------------------------+
//                  |             Encoder (Node 0)                  |
//                  |                                               |
//                  | Net Device 1   Net Device 2  ..  Net Device N |
//                  | IP: 10.1.1.1   IP: 10.1.2.1  ..  IP: 10.1.N.1 |
//                  |                                               |
//                  |     +---+         +---+             +---+     |
//                  |     |   |         |   |             |   |     |
//                  +-----+-+-+---------+-+-+-------------+-+-+-----+
//                          |             |                 |
//                          |             |                 |
//          e  +------------+          e  |                 + ------+  e
//             |                          |                         |
//  +--------+-v-+-------+     +--------+-v-+-------+    +--------+-v-+-------+
//  |        |   |       |     |        |   |       |    |        |   |       |
//  |        +---+       |     |        +---+       |    |        +---+       |
//  |                    |     |                    |    |                    |
//  | Decoder 1 (Node 1) |     | Decoder 2 (Node 2) | .. | Decoder N (Node N) |
//  |                    |     |                    |    |                    |
//  |    Net Device 1    |     |    Net Device 1    | .. |    Net Device 1    |
//  |    IP: 10.1.1.2    |     |    IP: 10.1.2.2    | .. |    IP: 10.1.N.N    |
//  +--------------------+     +--------------------+    +--------------------+

//          N: number of decoders              e: errorRate
//! [1]
// By using the previous topology and IP addressing, we ensure that packets
// are properly broadcasted within the network

// You can modify any default parameter, by running (for example with a
// different error rate):

// python waf --run kodo-wired-broadcast --command-template="%s --errorRate=MY_ERROR_RATE"

// The parameters that can be modified are: generationSize, packetSize, ns-3
// simulation interval (for controlling event ocurrences), errorRate in the
// devices and total amount of users. For the field size, refer to the source
// code in the corresponding part below, modify and rebuild the project to
// make it effective (this is essential).

// When you are done, you will notice several pcap trace files in your
// directory. You can review the files with Wireshark or tcpdump. If
// you have tcpdump installed, you can try (for example) this:

// tcpdump -r kodo-wired-broadcast-0-0.pcap -nn -tt

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

#include "kodo-broadcast.h" // Contains the broadcast topology class

using namespace ns3;

int main (int argc, char *argv[])
{
  // Default values
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 5; // RLNC generation size
  double errorRate = 0.3; // Error rate for all the links
  uint32_t users = 2; // Number of users
  std::string field = "binary"; // Finite field used

  // Create a map for the field values
  std::map<std::string,kodocpp::field> fieldMap;
  fieldMap["binary"] = kodocpp::field::binary;
  fieldMap["binary4"] = kodocpp::field::binary4;
  fieldMap["binary8"] = kodocpp::field::binary8;

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
    generationSize);
  cmd.AddValue ("errorRate", "Packet erasure rate for the links", errorRate);
  cmd.AddValue ("users", "Number of receivers", users);
  cmd.AddValue ("field", "Finite field used", field);

  cmd.Parse (argc, argv);

  // Use the binary field in case of errors
  if (fieldMap.find (field) == fieldMap.end ())
    {
      field = "binary";
    }

  Time::SetResolution (Time::NS);
  //! [2]
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
    star.GetSpokeNode (n)->GetDevice (0)->SetAttribute ("ReceiveErrorModel",
      PointerValue (errorModel[n]));
    errorModel[n]->Enable ();
  }

  // Setting IP protocol stack
  InternetStackHelper internet;
  star.InstallStack (internet);

  // Set IP addresses
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));
  //! [3]
  // Setting up application socket parameters for transmitter and
  // receiver sockets
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // Transmitter socket
  Ptr<Socket> source = Socket::CreateSocket (star.GetHub (), tid);

  // Transmitter socket connections. Set transmitter for broadcasting
  uint16_t port = 80;
  InetSocketAddress remote = InetSocketAddress (
    Ipv4Address ("255.255.255.255"), port);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Receiver sockets
  std::vector<Ptr<Socket>> sinks (users);

  for (uint32_t n = 0; n < users; n++)
    {
      sinks[n] = Socket::CreateSocket (star.GetSpokeNode (n), tid);
    }

  // Check for finite field employed and
  // Creates the Broadcast helper for this broadcast topology

  Broadcast wiredBroadcast (kodocpp::codec::full_vector, fieldMap[field],
    users, generationSize, packetSize, source, sinks);

  // Receiver socket connections
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  for (const auto sink : sinks)
    {
      sink->Bind (local);
      sink->SetRecvCallback (MakeCallback (
        &Broadcast::ReceivePacket, &wiredBroadcast));
    }

  // Turn on global static routing so we can be routed across the network
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes
  // pointToPoint.EnablePcapAll ("kodo-wired-broadcast");

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (1.0),
    &Broadcast::SendPacket, &wiredBroadcast, source, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}