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

// This example shows how to use the Kodo library for recoding at an
// intermediate node in the network within a ns-3 simulation. Recoding is
// one of the characteristic features of network coding because it
// differentiates from end-to-end codes by allowing any intermediate node
// to recode a coded packet, even if its data set has not been completely
// decoded yet.

// In the script below an encoder transmits coded packets from a block of
// data to a node through an erasure channel with a given rate, which may or
// may not recode the data. This node transmits its packets to a decoder
// (thorugh another erasure channel) until it has received a complete
// generation. Here the packets are sent using the binary field, GF(2) with a
// generation of 3 packets and 1000 (application) bytes to the other node.
// Topology with IP addresses per net device is as follows:

//         +-----------+  e1  +-----------+  e2  +------------+
//         |  encoder  |+---->|  recoder  |+---->|  decoder_2 |
//         +-----------+      +-----------+      +------------+
//  IP:       10.1.1.1           10.1.1.2           10.1.1.4
//                               10.1.1.3

// In the previous figure: e1 is the packet error rate between encoder and
// recoder, namely errorRateEncoderRecoder. e2 is the packet error rate
// between recoder and decoder, namely errorRateRecoderDecoder. Each IP address
// represents a net device in the node.

// You can change any parameter, by running (for example with a different
// generation size):
// ./build/linux/encoder_recoder_decoder/encoder_recoder_decoder --generationSize=GENERATION_SIZE

// When you are done, you will notice four pcap trace files in your directory.
// You can review the files with Wireshark or tcpdump. If you have tcpdump
// installed, you can try (for example) this:
//
// tcpdump -r multihop-0-0.pcap -nn -tt


#include <ns3/core-module.h>
#include <ns3/point-to-point-star.h>
#include <ns3/network-module.h>
#include <ns3/config-store-module.h>
#include <ns3/internet-module.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <ctime>

#include "../encoder-recoders-decoder-rlnc.h"

using namespace ns3;

int main (int argc, char *argv[])
{
  uint32_t packetSize = 1000; // Application bytes per packet
  double interval = 1.0; // Time between events
  uint32_t generationSize = 3; // RLNC generation size
  double errorRateEncoderRecoder = 0.4; // Error rate for encoder-recoder link
  double errorRateRecoderDecoder = 0.2; // Error rate for recoder-decoder link
  bool recodingFlag = true; // Flag to control recoding
  uint32_t recoders = 3; // Number of recoders

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
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);

  // We group the nodes in different sets because
  // we want many net devices per node.For the broadcast
  // topology we create a subnet and for the recoders to
  // the decoders, we create a secondary one.


  // First we set the basic helper for a single link.
  PointToPointHelper ptp;

  // Encoder to recoders
  PointToPointStarHelper toRecoders (recoders, ptp);
  NodeContainer decoder;
  decoder.Create(1);

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
  // (N = recoders amount) and a net device per encoder
  // Second, we mirror this procedure to the recoder side.
  // Each net device in the encoder is in a **different** subnet.

  toRecoders.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0",
    "255.255.255.0"));

  // The IP set of the recoders to the decoder is calculated
  // in order to not collide with the one from the encoder
  // to the recoders.
  Ipv4AddressHelper fromRecoders ("10.2.1.0", "255.255.255.0");
  fromRecoders.Assign (recodersDecoderDev);

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
      recodersDecoderDev.Get (2*n)->SetAttribute ("ReceiveErrorModel",
        PointerValue (errorRecodersDecoder[n]));

      // Activate models
      errorEncoderRecoders[n]->Enable ();
      errorRecodersDecoder[n]->Enable ();
    }

  // Setting up application sockets for recoder and decoder
  uint16_t port = 80;
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  Ipv4Address decoderAddress = decoder.Get (0)->GetObject<Ipv4> ()->
    GetAddress (1,0).GetLocal ();

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

  // Simulation setup
  using field = fifi::binary8;
  using encoderTrace = kodo::disable_trace;
  using decoderTrace = kodo::enable_trace;

  using simulation = EncoderRecodersDecoderRlnc<
    field, encoderTrace, decoderTrace>;

  simulation multihop (recoders, generationSize, packetSize, recodersSockets,
    recodingFlag);

  // Recoders callbacks
  for (uint32_t n = 0; n < recoders; n++)
    {
      recodersSockets[n]-> SetRecvCallback (MakeCallback (
        &simulation::ReceivePacketRecoder, &multihop));
    }

  // Decoder
  Ptr<Socket> decoderSocket = Socket::CreateSocket (decoder.Get (0), tid);
  decoderSocket->Bind (local);
  decoderSocket->SetRecvCallback (MakeCallback (
    &simulation::ReceivePacketDecoder, &multihop));

  // Turn on global static routing so we can actually be routed across the hops
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  // Do pcap tracing on all point-to-point devices on all nodes. File naming
  // convention is: multihop-[NODE_NUMBER]-[DEVICE_NUMBER].pcap
  ptp.EnablePcapAll ("multihop");

  // Schedule processes
  // Encoder
  Simulator::ScheduleWithContext (encoderSocket->GetNode ()->GetId (),
    Seconds (1.0), &simulation::SendPacketEncoder, &multihop, encoderSocket,
    interPacketInterval);

  // Recoders
  for (auto recoderSocket : recodersSockets)
    {
      Simulator::ScheduleWithContext (recoderSocket->GetNode ()->GetId (),
        Seconds (1.5), &simulation::SendPacketRecoder, &multihop, recoderSocket,
        interPacketInterval);
    }

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}