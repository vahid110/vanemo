/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/athstats-helper.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4.h"
#include "ns3/internet-module.h"

#include <iostream>

using namespace ns3;

static bool g_verbose = true;

void
DevTxTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << " TX p: " << *p << std::endl;
    }
}
void
DevRxTrace (std::string context, Ptr<const Packet> p)
{
  if (g_verbose)
    {
      std::cout << " RX p: " << *p << std::endl;
    }
}
void
PhyRxOkTrace (std::string context, Ptr<const Packet> packet, double snr, WifiMode mode, enum WifiPreamble preamble)
{
  if (g_verbose)
    {
      std::cout << "PHYRXOK mode=" << mode << " snr=" << snr << " " << *packet << std::endl;
    }
}
void
PhyRxErrorTrace (std::string context, Ptr<const Packet> packet, double snr)
{
  if (g_verbose)
    {
      std::cout << "PHYRXERROR snr=" << snr << " " << *packet << std::endl;
    }
}
void
PhyTxTrace (std::string context, Ptr<const Packet> packet, WifiMode mode, WifiPreamble preamble, uint8_t txPower)
{
  if (g_verbose)
    {
      std::cout << "PHYTX mode=" << mode << " " << *packet << std::endl;
    }
}
void
PhyStateTrace (std::string context, Time start, Time duration, enum WifiPhy::State state)
{
  if (g_verbose)
    {
      std::cout << " state=" << state << " start=" << start << " duration=" << duration << std::endl;
    }
}

static void
SetPosition (Ptr<Node> node, Vector position)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  mobility->SetPosition (position);
}

static Vector
GetPosition (Ptr<Node> node)
{
  Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
  return mobility->GetPosition ();
}

static void 
AdvancePosition (Ptr<Node> node) 
{
  Vector pos = GetPosition (node);
  pos.x += 5.0;
  if (pos.x >= 210.0) 
    {
      return;
    }
  SetPosition (node, pos);

  if (g_verbose)
    {
      //std::cout << "x="<<pos.x << std::endl;
    }
  Simulator::Schedule (Seconds (1.0), &AdvancePosition, node);
}

int main (int argc, char *argv[])
{
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.AddValue ("verbose", "Print trace information if true", g_verbose);

  cmd.Parse (argc, argv);

//  Packet::EnablePrinting ();

  // enable rts cts all the time.
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("0"));
  // disable fragmentation
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));

  WifiHelper wifi = WifiHelper::Default ();
  MobilityHelper mobility;
  NodeContainer sta;
  NodeContainer mn;
  NetDeviceContainer staDevs, mnDevs;

  sta.Create (1);
  mn.Create (1);

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  Ssid ssid = Ssid ("wifi-default");
  wifi.SetRemoteStationManager ("ns3::ArfWifiManager");
  // setup stas.

  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "ActiveProbing", BooleanValue (false));
  staDevs = wifi.Install (wifiPhy, wifiMac, sta);
//  // setup ap.
  wifiMac.SetType ("ns3::AdhocWifiMac");
  staDevs.Add(wifi.Install (wifiPhy, wifiMac, sta));

  mnDevs = wifi.Install (wifiPhy, wifiMac, mn);


  InternetStackHelper internet;
//  internet.SetRoutingHelper (olsr); // has effect on the next Install ()
  internet.Install (NodeContainer(sta,mn));

  Ipv4AddressHelper ipAddrs;
  ipAddrs.SetBase ("192.168.0.0", "255.255.255.0");
  ipAddrs.Assign (NetDeviceContainer(mnDevs, staDevs));

  // mobility.
  mobility.Install (sta);
  mobility.Install (mn);
  uint16_t port = 6000;

  Ptr<Node> srcNode = sta.Get(0);
  Ipv4Address srcAddress = srcNode->GetObject<Ipv4> ()->GetAddress(1, 0).GetLocal ();
  Ptr<Node> destNode = mn.Get(0);
  Ipv4Address destAddress = destNode->GetObject<Ipv4> ()->GetAddress(1, 0).GetLocal ();
  (void) srcAddress;

  SetPosition(srcNode, Vector(10,10,10));
  SetPosition(destNode, Vector(10,10,10));

  NS_LOG_UNCOND("Source Address:" << srcAddress << "  Dest Address:" << destAddress);

  UdpServerHelper server (port);
  ApplicationContainer serverApps = server.Install (destNode);

  //Clinet Application
  NS_LOG_UNCOND("Installing UDP client on CN");
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 30;
  Time interPacketInterval = MilliSeconds(1000);
  UdpClientHelper udpClient(destAddress, port);
  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  ApplicationContainer clientApps = udpClient.Install (srcNode);

  serverApps.Start (Seconds (1.0));
  clientApps.Start (Seconds (1.5));
  serverApps.Stop (Seconds (50.0));
  clientApps.Stop (Seconds (50.0));

  Simulator::Stop (Seconds (50.0));

//  Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacTx", MakeCallback (&DevTxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Mac/MacRx", MakeCallback (&DevRxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxOk", MakeCallback (&PhyRxOkTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/RxError", MakeCallback (&PhyRxErrorTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/Tx", MakeCallback (&PhyTxTrace));
//  Config::Connect ("/NodeList/*/DeviceList/*/Phy/State/State", MakeCallback (&PhyStateTrace));

  AthstatsHelper athstats;
  athstats.EnableAthstats ("athstats-sta", sta);
  athstats.EnableAthstats ("athstats-ap", mn);

  Simulator::Run ();

  Simulator::Destroy ();

  return 0;
}
