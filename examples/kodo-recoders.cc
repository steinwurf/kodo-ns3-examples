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

// This example shows how to use the Kodo library for recoding at many
// intermediate nodes in a network within a ns-3 Recoders topology.
// Recoding is one of the characteristic features of network coding because it
// differentiates from end-to-end codes by allowing any intermediate node
// to recode a coded packet, even if its data set has not been completely
// decoded yet.
//
// In the source code below an encoder transmits coded packets from a block of
// data to a set of nodes with the same erasure channel
// (errorRateEncoderRecoders) The recoders which may or may not recode the data
// according to a boolean value (recodingFlag). Then, recoders transmit its
// packets to a decoder through another erasure channel
// (errorRateRecodersDecoder). Coded packets are sent from the recoders until
// the decoder has received the complete generation. By default, packets are
// sent using the binary8 field, GF(2^8) with a generation of 3 packets and
// 1000 (application) bytes per packet. Default error rates are 40% (0.4) for
// the encoder-recoders hop and 20% (0.2) for the recoders-decoder hop. By
// default, we set the number of recoders to 2.
//
// In general, topology with IP addresses per net device is as follows:
//! [0]
//                +-----------------------------------------------+
//                |             Encoder (Node 0)                  |
//                |                                               |
//                | Net Device 1   Net Device 2  ..  Net Device N |
//                | IP: 10.1.1.1   IP: 10.1.2.1  ..  IP: 10.1.N.1 |
//                |                                               |
//                |     +---+         +---+             +---+     |
//                |     |   |         |   |             |   |     |
//                +-----+-+-+---------+-+-+-------------+-+-+-----+
//                        |             |                 |
//     eE-R  +------------+        eE-R |           eE-R  +-------+
//           |                          |                         |
//+--------+-v-+-------+     +--------+-v-+-------+    +--------+-v-+-------+
//|        |   |       |     |        |   |       |    |        |   |       |
//|        +-+-+       |     |        +-+-+       |    |        +-+-+       |
//|    Net Device 1    |     |    Net Device 1    | .. |    Net Device 1    |
//|    IP: 10.1.1.2    |     |    IP: 10.1.2.2    | .. |    IP: 10.1.N.N    |
//|                    |     |                    |    |                    |
//| Recoder 1 (Node 1) |     | Recoder 2 (Node 2) | .. | Recoder N (Node N) |
//|                    |     |                    |    |                    |
//|    Net Device 2    |     |    Net Device 2    | .. |    Net Device 2    |
//|    IP: 10.2.1.1    |     |    IP: 10.2.1.3    | .. |    IP: 10.2.1.2N+1 |
//|                    |     |                    |    |                    |
//|        +---+       |     |        +---+       |    |        +---+       |
//|        |   |       |     |        |   |       |    |        |   |       |
//+--------+-+-+-------+     +--------+-+-+-------+    +--------+-+-+-------+
//           |                          |                         |
//           +-----------+  eR-D        |  eR-D           +-------+  eR-D
//                       |              |                 |
//               +-----+-v-+----------+-v-+-------------+-v-+----+
//               |     |   |          |   |             |   |    |
//               |     +---+          +---+             +---+    |
//               |                                               |
//               |             Decoder (Node N+1)                |
//               |                                               |
//               | Net Device 1   Net Device 2  ..  Net Device N |
//               | IP: 10.2.1.2   IP: 10.2.1.4  ..  IP: 10.2.1.2N|
//               +-----------------------------------------------+
//
//                           N: Number of recoders
//                           eE-R: errorRateEncoderRecoders
//                           eR-D: errorRateRecodersDecoder
//! [1]
// By using the previous topology and IP addressing, we ensure that packets
// are properly broadcasted to the recoders and each combination is sent from
// the respective recoder to the decoder.
//
// You can modify any default parameter, by running (for example with a
// different number of recoders):
//
// python waf --run kodo-recoders --command-template="%s --recoders=MY_RECODER_COUNT"
//! [2]

#include <iostream>
#include <vector>
#include <string>

#include <ns3/core-module.h>
#include <ns3/point-to-point-star.h>
#include <ns3/network-module.h>
#include <ns3/config-store-module.h>
#include <ns3/internet-module.h>

#include "kodo-recoders.h"
//! [3]
using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 3; // RLNC generation size
  double errorRateEncoderRecoder = 0.4; // Error rate for encoder-recoder link
  double errorRateRecoderDecoder = 0.2; // Error rate for recoder-decoder link
  bool recodingFlag = true; // Flag to control recoding
  uint32_t recoders = 2; // Number of recoders
  std::string field = "binary"; // Finite field used
  double transmitProbability = 0.5; // Transmit probability for the recoders

  // Create a map for the field values
  std::map<std::string, fifi::api::field> fieldMap;
  fieldMap["binary"] = fifi::api::field::binary;
  fieldMap["binary4"] = fifi::api::field::binary4;
  fieldMap["binary8"] = fifi::api::field::binary8;

  Time interPacketInterval = Seconds (interval);

  CommandLine cmd;

  cmd.AddValue ("packetSize", "Size of application packet sent", packetSize);
  cmd.AddValue ("interval", "Interval (seconds) between packets", interval);
  cmd.AddValue ("generationSize", "Set the generation size to use",
                generationSize);
  cmd.AddValue ("errorRateEncoderRecoder",
                "Packet erasure rate for the encoder-recoder link",
                errorRateEncoderRecoder);
  cmd.AddValue ("errorRateRecoderDecoder",
                "Packet erasure rate for the recoder-decoder link",
                errorRateRecoderDecoder);
  cmd.AddValue ("recodingFlag", "Enable packet recoding", recodingFlag);
  cmd.AddValue ("recoders", "Amount of recoders", recoders);
  cmd.AddValue ("field", "Finite field used", field);
  cmd.AddValue ("transmitProbability", "Transmit probability from recoder",
                transmitProbability);

  cmd.Parse (argc, argv);

  // Use the binary8 field in case of errors
  if (fieldMap.find (field) == fieldMap.end ())
    {
      field = "binary8";
    }

  Time::SetResolution (Time::NS);

  //! [4]
  // We group the nodes in different sets because
  // we want many net devices per node. For the broadcast
  // topology we create a subnet and for the recoders to
  // the decoders, we create a secondary one. This in order to
  // properly set the net devices and socket connections later


  // First we set the basic helper for a single link.
  PointToPointHelper ptp;

  // Encoder to recoders
  PointToPointStarHelper toRecoders (recoders, ptp);
  NodeContainer decoder;
  decoder.Create (1);

  // Recoders to decoder
  NetDeviceContainer recodersDecoderDev;

  for (uint32_t n = 0; n < recoders; n++)
   {
      recodersDecoderDev.Add (ptp.Install (
        NodeContainer (toRecoders.GetSpokeNode (n), decoder.Get (0))) );
   }

  // Internet stack for the broadcast topology and decoder
  InternetStackHelper internet;
  toRecoders.InstallStack (internet);
  internet.Install (decoder);

  // Here, we first create a total of N net devices in the encoder
  // (N = recoders amount) and a net device per recoder
  // Second, we mirror this procedure in the second hop.
  // Each net device in the recoder is in a different subnet.

  toRecoders.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0",
    "255.255.255.0"));

  // The IP set of the recoders to the decoder is calculated
  // in order to not collide with the one from the encoder
  // to the recoders.
  Ipv4AddressHelper fromRecoders ("10.2.1.0", "255.255.255.0");
  fromRecoders.Assign (recodersDecoderDev);
  //! [5]
  // Set error model for the net devices
  Config::SetDefault ("ns3::RateErrorModel::ErrorUnit",
                      StringValue ("ERROR_UNIT_PACKET"));

  // Create error rate models per branch
  std::vector<Ptr<RateErrorModel>> errorEncoderRecoders (recoders);
  std::vector<Ptr<RateErrorModel>> errorRecodersDecoder (recoders);

  for (uint32_t n = 0; n < recoders; n++)
    {
      // Encoder to recoders branches
      errorEncoderRecoders[n] = CreateObject<RateErrorModel> ();
      errorEncoderRecoders[n]->SetAttribute ("ErrorRate", DoubleValue (
        errorRateEncoderRecoder));
      toRecoders.GetSpokeNode (n)->GetDevice (0)->SetAttribute (
        "ReceiveErrorModel", PointerValue (errorEncoderRecoders[n]));

      // Recoders to decoder branches
      errorRecodersDecoder[n] = CreateObject<RateErrorModel> ();
      errorRecodersDecoder[n]->SetAttribute ("ErrorRate", DoubleValue (
        errorRateRecoderDecoder));
      recodersDecoderDev.Get (2*n + 1)->SetAttribute ("ReceiveErrorModel",
        PointerValue (errorRecodersDecoder[n]));

      // Activate models
      errorEncoderRecoders[n]->Enable ();
      errorRecodersDecoder[n]->Enable ();
    }

  // Setting up application sockets for recoder and decoder
  uint16_t port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  Ipv4Address decoderAddress = decoder.Get (0)->GetObject<Ipv4> ()->
    GetAddress (1, 0).GetLocal ();

  // Socket connection addresses
  InetSocketAddress decoderSocketAddress = InetSocketAddress (
    decoderAddress, port);

  // Socket bind address
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), port);

  // Encoder connections to recoders
  Ptr<Socket> encoderSocket = Socket::CreateSocket (toRecoders.GetHub (), tid);
  encoderSocket->SetAllowBroadcast (true);
  InetSocketAddress recodersSocketAddress = InetSocketAddress (
    Ipv4Address ("255.255.255.255"), port);
  encoderSocket->Connect (recodersSocketAddress);

  // Recoders connections to decoder
  std::vector<Ptr<Socket>> recodersSockets;

  for (uint32_t n = 0; n < recoders; n++)
    {
      recodersSockets.push_back (Socket::CreateSocket (
        toRecoders.GetSpokeNode (n), tid));
      recodersSockets[n]->Bind (local);
      recodersSockets[n]->Connect (decoderSocketAddress);
    }

  Recoders multihop (fieldMap[field], recoders, generationSize, packetSize,
      recodersSockets, recodingFlag, transmitProbability);

  // Recoders callbacks
  for (uint32_t n = 0; n < recoders; n++)
    {
      recodersSockets[n]-> SetRecvCallback (MakeCallback (
        &Recoders::ReceivePacketRecoder, &multihop));
    }

  // Decoder
  Ptr<Socket> decoderSocket = Socket::CreateSocket (decoder.Get (0), tid);
  decoderSocket->Bind (local);
  decoderSocket->SetRecvCallback (MakeCallback (
    &Recoders::ReceivePacketDecoder, &multihop));

  // Turn on global static routing so we can actually be routed across the hops
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes. File naming
  // convention is: kodo-recoders-[NODE_NUMBER]-[DEVICE_NUMBER].pcap
  // ptp.EnablePcapAll ("kodo-recoders");

  // Schedule processes
  // Encoder
  Simulator::ScheduleWithContext (encoderSocket->GetNode ()->GetId (),
    Seconds (1.0), &Recoders::SendPacketEncoder,
    &multihop, encoderSocket, interPacketInterval);

  //! [6]
  // Recoders
  for (auto recoderSocket : recodersSockets)
    {
      Simulator::ScheduleWithContext (recoderSocket->GetNode ()->GetId (),
        Seconds (1.5), &Recoders::SendPacketRecoder,
        &multihop, recoderSocket, interPacketInterval);
    }
  //! [7]
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
