/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//    SRC
//     |<=== source network
//     A-----B
//      \   / \   all networks have cost 1, except
//       \ /  |   for the direct link from C to D, which
//        C  /    has cost 10
//        | /
//        |/
//        D
//        |<=== target network
//       DST
//
//
// A, B, C and D are RIPng routers.
// A and D are configured with static addresses.
// SRC and DST will exchange packets.
//
// After about 3 seconds, the topology is built, and Echo Reply will be received.
// After 40 seconds, the link between B and D will break, causing a route failure.
// After 44 seconds from the failure, the routers will recovery from the failure.
// Split Horizoning should affect the recovery time, but it is not. See the manual
// for an explanation of this effect.
//
// If "showPings" is enabled, the user will see:
// 1) if the ping has been acknowledged
// 2) if a Destination Unreachable has been received by the sender
// 3) nothing, when the Echo Request has been received by the destination but
//    the Echo Reply is unable to reach the sender.
// Examining the .pcap files with Wireshark can confirm this effect.


#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv6-routing-table-entry.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RipNgSimpleRouting");

void TearDownLink (Ptr<Node> nodeA, Ptr<Node> nodeB, uint32_t interfaceA, uint32_t interfaceB)
{
  nodeA->GetObject<Ipv6> ()->SetDown (interfaceA);
  nodeB->GetObject<Ipv6> ()->SetDown (interfaceB);
}

bool verbose = false;
bool printRoutingTables = false;
bool showPings = false;
std::string SplitHorizon ("PoisonReverse");

CommandLine cmd;

int main (int argc, char **argv)
{
  CommandLine cmd;
  cmd.AddValue ("verbose", "turn on log components", verbose);
  cmd.AddValue ("printRoutingTables", "Print routing tables at 30, 60 and 90 seconds", printRoutingTables);
  cmd.AddValue ("showPings", "Show Ping6 reception", showPings);
  cmd.AddValue ("splitHorizonStrategy", "Split Horizon strategy to use (NoSplitHorizon, SplitHorizon, PoisonReverse)", SplitHorizon);
  cmd.Parse (argc, argv);

  if (verbose)
    {
      LogComponentEnable ("RipNgSimpleRouting", LOG_LEVEL_ALL);
      LogComponentEnable ("RipNg", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv6L4Protocol", LOG_LEVEL_INFO);
      LogComponentEnable ("Ipv6Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("Icmpv6L4Protocol", LOG_LEVEL_ALL);
      LogComponentEnable ("NdiscCache", LOG_LEVEL_ALL);
      LogComponentEnable ("Ping6Application", LOG_LEVEL_ALL);
    }

  LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
  if (showPings)
    {
      LogComponentEnable ("Ping6Application", LOG_LEVEL_INFO);
    }

  if (SplitHorizon == "NoSplitHorizon")
    {
      Config::SetDefault ("ns3::RipNg::SplitHorizon", EnumValue (RipNg::NO_SPLIT_HORIZON));
    }
  else if (SplitHorizon == "SplitHorizon")
    {
      Config::SetDefault ("ns3::RipNg::SplitHorizon", EnumValue (RipNg::SPLIT_HORIZON));
    }
  else
    {
      Config::SetDefault ("ns3::RipNg::SplitHorizon", EnumValue (RipNg::POISON_REVERSE));
    }

  NS_LOG_UNCOND("Create nodes.");
  Ptr<Node> src = CreateObject<Node> ();
  Names::Add ("SrcNode", src);
  Ptr<Node> dst = CreateObject<Node> ();
  Names::Add ("DstNode", dst);
  Ptr<Node> a = CreateObject<Node> ();
  Names::Add ("RouterA", a);
  Ptr<Node> b = CreateObject<Node> ();
  Names::Add ("RouterB", b);
  Ptr<Node> c = CreateObject<Node> ();
  Names::Add ("RouterC", c);
  Ptr<Node> d = CreateObject<Node> ();
  Names::Add ("RouterD", d);
  NodeContainer nod_srca (src, a);
  NodeContainer nod_ab (a, b);
  NodeContainer nod_ac (a, c);
  NodeContainer nod_bc (b, c);
  NodeContainer nod_cd (c, d);
  NodeContainer nod_bd (b, d);
  NodeContainer nod_ddst (d, dst);
  NodeContainer routers (a, b, c, d);
  NodeContainer nod_srcdst (src, dst);


  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (5000000));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));
  NetDeviceContainer ndc_srca = csma.Install (nod_srca);
  NetDeviceContainer ndc_ab = csma.Install (nod_ab);
  NetDeviceContainer ndc_ac = csma.Install (nod_ac);
  NetDeviceContainer ndc_bc = csma.Install (nod_bc);
  NetDeviceContainer ndc_cd = csma.Install (nod_cd);
  NetDeviceContainer ndc_bd = csma.Install (nod_bd);
  NetDeviceContainer ndc_ddst = csma.Install (nod_ddst);

  NS_LOG_INFO ("Create IPv6 and routing");
  RipNgHelper ripNgRouting;

  // Rule of thumb:
  // Interfaces are added sequentially, starting from 0
  // However, interface 0 is always the loopback...
  ripNgRouting.ExcludeInterface (a, 1);
  ripNgRouting.ExcludeInterface (d, 3);

  ripNgRouting.SetInterfaceMetric (c, 3, 10);
  ripNgRouting.SetInterfaceMetric (d, 1, 10);

  Ipv6ListRoutingHelper listRH;
  listRH.Add (ripNgRouting, 0);

  InternetStackHelper internetv6;
  internetv6.SetIpv4StackInstall (false);
  internetv6.SetRoutingHelper (listRH);
  internetv6.Install (routers);

  InternetStackHelper internetv6Nodes;
  internetv6Nodes.SetIpv4StackInstall (false);
  internetv6Nodes.Install (nod_srcdst);

  // Assign addresses.
  // The source and destination networks have global addresses
  // The "core" network just needs link-local addresses for routing.
  // We assign global addresses to the routers as well to receive
  // ICMPv6 errors.
  NS_LOG_INFO ("Assign IPv6 Addresses.");
  Ipv6AddressHelper ipv6;

  ipv6.SetBase (Ipv6Address ("2001:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_srca = ipv6.Assign (ndc_srca);
  iic_srca.SetForwarding (1, true);
  iic_srca.SetDefaultRouteInAllNodes (1);

  ipv6.SetBase (Ipv6Address ("2001:0:1::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_ab = ipv6.Assign (ndc_ab);
  iic_ab.SetForwarding (0, true);
  iic_ab.SetForwarding (1, true);

  ipv6.SetBase (Ipv6Address ("2001:0:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_ac = ipv6.Assign (ndc_ac);
  iic_ac.SetForwarding (0, true);
  iic_ac.SetForwarding (1, true);

  ipv6.SetBase (Ipv6Address ("2001:0:3::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_bc = ipv6.Assign (ndc_bc);
  iic_bc.SetForwarding (0, true);
  iic_bc.SetForwarding (1, true);

  ipv6.SetBase (Ipv6Address ("2001:0:4::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_cd = ipv6.Assign (ndc_cd);
  iic_cd.SetForwarding (0, true);
  iic_cd.SetForwarding (1, true);

  ipv6.SetBase (Ipv6Address ("2001:0:5::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_bd = ipv6.Assign (ndc_bd);
  iic_bd.SetForwarding (0, true);
  iic_bd.SetForwarding (1, true);

  ipv6.SetBase (Ipv6Address ("2001:2::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer iic_ddst = ipv6.Assign (ndc_ddst);
  iic_ddst.SetForwarding (0, true);
  iic_ddst.SetDefaultRouteInAllNodes (0);

  if (printRoutingTables)
    {
      RipNgHelper routingHelper;

      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> (&std::cout);

      routingHelper.PrintRoutingTableAt (Seconds (30.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (30.0), d, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (60.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (60.0), d, routingStream);

      routingHelper.PrintRoutingTableAt (Seconds (90.0), a, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), b, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), c, routingStream);
      routingHelper.PrintRoutingTableAt (Seconds (90.0), d, routingStream);
    }

  NS_LOG_INFO ("Create Applications.");
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 100;
  Time interPacketInterval = Seconds (1.0);
  Ping6Helper ping6;

  ping6.SetLocal (iic_srca.GetAddress (0, 1));
  ping6.SetRemote (iic_ddst.GetAddress (1, 1));
  ping6.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  ping6.SetAttribute ("Interval", TimeValue (interPacketInterval));
  ping6.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer apps = ping6.Install (src);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (110.0));

  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("ripng-simple-routing.tr"));
  csma.EnablePcapAll ("ripng-simple-routing", true);

  Simulator::Schedule (Seconds (40), &TearDownLink, b, d, 3, 2);

  /* Now, do the actual simulation. */
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (120));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}

