/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/pmipv6-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/bridge-module.h"
#include "ns3/propagation-loss-model.h"

#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-static-source-routing.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/netanim-module.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("Pmipv6Wifi");

using namespace ns3;

Ipv6InterfaceContainer AssignIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix)
{
  Ipv6InterfaceContainer retval;

  Ptr<Node> node = device->GetNode ();
  NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;

  ifIndex = ipv6->GetInterfaceForDevice (device);

  if (ifIndex == -1)
    {
      ifIndex = ipv6->AddInterface (device);
    }
  NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                 "Interface index not found");

  Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress (addr, prefix);
  ipv6->SetMetric (ifIndex, 1);
  ipv6->SetUp (ifIndex);
  ipv6->AddAddress (ifIndex, ipv6Addr);
  NS_LOG_UNCOND (ipv6Addr);

  retval.Add (ipv6, ifIndex);

  return retval;
}

Ipv6InterfaceContainer AssignIpv6Address(NetDeviceContainer devices, Ipv6Address network = Ipv6Address(/*"2001:1::"*/"3ffe:1:4:1::"), Ipv6Prefix prefix = Ipv6Prefix (64))
{
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (network, prefix);
  Ipv6InterfaceContainer i = ipv6.Assign (devices);

  for (Ipv6InterfaceContainer::Iterator it = i.Begin(); it != i.End(); it++)
  {
	  NS_LOG_UNCOND (it->first->GetAddress(it->second, 1));
  }
  return i;
}

Ipv6InterfaceContainer AssignWithoutAddress(Ptr<NetDevice> device)
{
  Ipv6InterfaceContainer retval;

  Ptr<Node> node = device->GetNode ();
  NS_ASSERT_MSG (node, "Ipv6AddressHelper::Allocate (): Bad node");

  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
  NS_ASSERT_MSG (ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;

  ifIndex = ipv6->GetInterfaceForDevice (device);
  if (ifIndex == -1)
    {
      ifIndex = ipv6->AddInterface (device);
    }
  NS_ASSERT_MSG (ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                 "Interface index not found");

  ipv6->SetMetric (ifIndex, 1);
  ipv6->SetUp (ifIndex);

  retval.Add (ipv6, ifIndex);

  return retval;
}


//static void udpRx (std::string context, Ptr<const Packet> packet, const Address &address) {
//  SeqTsHeader seqTs;
//  packet->Copy ()->RemoveHeader (seqTs);
//  NS_LOG_UNCOND (context << "  " << seqTs.GetTs () << " ---> " << Simulator::Now() << ": " << seqTs.GetSeq());
//}

int main (int argc, char *argv[])
{
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

  double startTime = 0.0;
  double endTime   = 40.0;
  (void) startTime; (void)endTime;

  NodeContainer sta;
  NodeContainer grp;
  NodeContainer cn;
  NodeContainer backbone;
  NodeContainer aps;

  //ref nodes
  NodeContainer lma;
  NodeContainer mags;
  NodeContainer outerNet;
  NodeContainer mag1Net;
  NodeContainer mag2Net;
  NodeContainer mag3Net;
  
  NetDeviceContainer backboneDevs;
  NetDeviceContainer outerDevs;
  NetDeviceContainer mag1Devs;
  NetDeviceContainer mag2Devs;
  NetDeviceContainer mag3Devs;
  NetDeviceContainer mag1ApDev;
  NetDeviceContainer mag2ApDev;
  NetDeviceContainer mag3ApDev;
  NetDeviceContainer mag1BrDev;
  NetDeviceContainer mag2BrDev;
  NetDeviceContainer mag3BrDev;
  NetDeviceContainer staDevs;
  NetDeviceContainer grpDevs;
  
  Ipv6InterfaceContainer backboneIfs;
  Ipv6InterfaceContainer outerIfs;
  Ipv6InterfaceContainer mag1Ifs;
  Ipv6InterfaceContainer mag2Ifs;
  Ipv6InterfaceContainer mag3Ifs;
  Ipv6InterfaceContainer staIfs;
  Ipv6InterfaceContainer grpIfs;
  
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  SeedManager::SetSeed (123456);
//  LogLevel logAll = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
//  LogLevel logLogic = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_LOGIC);
//  LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);

//  LogComponentEnable ("Udp6Server", logInfo);
//  LogComponentEnable ("Pmipv6Agent", logAll);
//  LogComponentEnable ("Pmipv6MagNotifier", logAll);
 
  int cnt = 3;
  backbone.Create(cnt + 1);
  aps.Create(cnt);
  cn.Create(1);
  sta.Create(1);
  grp.Create(2);

  InternetStackHelper internet;
  internet.Install (backbone);
  internet.Install (aps);
  internet.Install (cn);
  internet.Install (sta);
  internet.Install (grp);

  lma.Add(backbone.Get(0));
  
  mags.Add(backbone.Get(1));
  mags.Add(backbone.Get(2));
  mags.Add(backbone.Get(3));
  
  outerNet.Add(lma);
  outerNet.Add(cn);

  mag1Net.Add(mags.Get(0));
  mag1Net.Add(aps.Get(0));

  mag2Net.Add(mags.Get(1));
  mag2Net.Add(aps.Get(1));

  mag3Net.Add(mags.Get(2));
  mag3Net.Add(aps.Get(2));

  CsmaHelper csma, csma1;
  
  //MAG's MAC Address (for unify default gateway of MN)
  Mac48Address magMacAddr("00:00:AA:BB:CC:DD");
  Mac48Address magMacAddr1("00:00:AA:BB:CC:EE");
  Mac48Address magMacAddr2("00:00:AA:BB:CC:FF");

  Ipv6InterfaceContainer iifc;
  
  //Link between CN and LMA is 50Mbps and 0.1ms delay
  csma1.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma1.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma1.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  
  outerDevs = csma1.Install(outerNet);
  iifc = AssignIpv6Address(outerDevs.Get(0), Ipv6Address("3ffe:2::1"), 64);
  outerIfs.Add(iifc);
  iifc = AssignIpv6Address(outerDevs.Get(1), Ipv6Address("3ffe:2::2"), 64);
  outerIfs.Add(iifc);
  outerIfs.SetForwarding(0, true);
  outerIfs.SetDefaultRouteInAllNodes(0);

  //All Link is 50Mbps and 0.1ms delay
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
  NS_LOG_UNCOND("Assign backbone Addresses");
  backboneDevs = csma.Install(backbone);
  iifc = AssignIpv6Address(backboneDevs.Get(0), Ipv6Address("3ffe:1::1"), 64);
  backboneIfs.Add(iifc);
  iifc = AssignIpv6Address(backboneDevs.Get(1), Ipv6Address("3ffe:1::2"), 64);
  backboneIfs.Add(iifc);
  iifc = AssignIpv6Address(backboneDevs.Get(2), Ipv6Address("3ffe:1::3"), 64);
  backboneIfs.Add(iifc);
  iifc = AssignIpv6Address(backboneDevs.Get(3), Ipv6Address("3ffe:1::4"), 64);
  backboneIfs.Add(iifc);
  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);
  
  BridgeHelper bridge;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (0.0, -20.0, 0.0));   //LMA
  positionAlloc->Add (Vector (-50.0, 20.0, 0.0)); //MAG1
  positionAlloc->Add (Vector (50.0, 20.0, 0.0));  //MAG2
  positionAlloc->Add (Vector (150.0, 20.0, 0.0));  //MAG3
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobility.Install (backbone);
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (75.0, -20.0, 0.0));   //CN
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobility.Install (cn);
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (-50.0, 40.0, 0.0)); //MAG1AP
  positionAlloc->Add (Vector (50.0, 40.0, 0.0));  //MAG2AP
  positionAlloc->Add (Vector (150.0, 40.0, 0.0));  //MAG3AP
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobility.Install (aps);
  NS_LOG_UNCOND("MAG-AP Addresses");
  //Setting MAG1 and WLAN AP
  Ssid ssid = Ssid("MAG");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  WifiHelper wifi = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (30));
  wifiPhy.SetChannel (wifiChannel.Create ());
   
  wifiMac.SetType ("ns3::ApWifiMac",
		           "Ssid", SsidValue (ssid),
		           "BeaconGeneration", BooleanValue (true),
		           "BeaconInterval", TimeValue (MicroSeconds (102400)));

  mag1Devs = csma.Install(mag1Net);
  mag1Devs.Get(0)->SetAddress(magMacAddr);
  mag1Ifs = AssignIpv6Address(mag1Devs.Get(0), Ipv6Address("3ffe:1:1::1"), 64);
  mag1ApDev = wifi.Install (wifiPhy, wifiMac, mag1Net.Get(1));
  mag1BrDev = bridge.Install (aps.Get(0), NetDeviceContainer(mag1ApDev, mag1Devs.Get(1)));
  iifc = AssignWithoutAddress(mag1Devs.Get(1));
  mag1Ifs.Add(iifc);
  mag1Ifs.SetForwarding(0, true);
  mag1Ifs.SetDefaultRouteInAllNodes(0);
  
  //Setting MAG2
  mag2Devs = csma.Install(mag2Net);
  mag2Devs.Get(0)->SetAddress(magMacAddr1);
  mag2Ifs = AssignIpv6Address(mag2Devs.Get(0), Ipv6Address("3ffe:1:2::1"), 64);
  mag2ApDev = wifi.Install (wifiPhy, wifiMac, mag2Net.Get(1));
  mag2BrDev = bridge.Install (aps.Get(1), NetDeviceContainer(mag2ApDev, mag2Devs.Get(1)));
  iifc = AssignWithoutAddress(mag2Devs.Get(1));
  mag2Ifs.Add(iifc);
  mag2Ifs.SetForwarding(0, true);
  mag2Ifs.SetDefaultRouteInAllNodes(0);
  
  //Setting MAG3
  mag3Devs = csma.Install(mag3Net);
  mag3Devs.Get(0)->SetAddress(magMacAddr2);
  mag3Ifs = AssignIpv6Address(mag3Devs.Get(0), Ipv6Address("3ffe:1:3::1"), 64);
  mag3ApDev = wifi.Install (wifiPhy, wifiMac, mag3Net.Get(1));
  mag3BrDev = bridge.Install (aps.Get(2), NetDeviceContainer(mag3ApDev, mag3Devs.Get(1)));
  iifc = AssignWithoutAddress(mag3Devs.Get(1));
  mag3Ifs.Add(iifc);
  mag3Ifs.SetForwarding(0, true);
  mag3Ifs.SetDefaultRouteInAllNodes(0);

  NS_LOG_UNCOND ("Create networks and assign MNN Addresses.");
  //setting station
  positionAlloc = CreateObject<ListPositionAllocator> ();
  
  positionAlloc->Add (Vector (-50.0, 50.0, 0.0)); //STA
  
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");  
  mobility.Install(sta);
  
  Ptr<ConstantVelocityMobilityModel> cvm = sta.Get(0)->GetObject<ConstantVelocityMobilityModel>();
  cvm->SetVelocity(Vector (10.0, 0, 0)); //move to left to right 10.0m/s
  //WLAN interface
  wifiMac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));

  staDevs.Add( wifi.Install (wifiPhy, wifiMac, sta));


  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-20.0, 20.0, 0.0)); //STA
  positionAlloc->Add (Vector (-20.0, 30.0, 0.0)); //STA
  mobility.SetPositionAllocator (positionAlloc);
  mobility.PushReferenceMobilityModel(sta.Get (0));
  mobility.Install(grp);

  //WLAN interface
  wifiMac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));
  grpDevs.Add( wifi.Install (wifiPhy, wifiMac, grp));

  NetDeviceContainer mnnDevs(staDevs, grpDevs);
  iifc = AssignIpv6Address(mnnDevs);
  NS_LOG_UNCOND("STA Address:" << mnnDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress());
  Ipv6Address staAddress = mnnDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();


  //attach PMIPv6 agents
  Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper> ();

  //adding profile for each station  
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(grpDevs.Get(0)->GetAddress())), Identifier(Mac48Address::ConvertFrom(grpDevs.Get(0)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(grpDevs.Get(1)->GetAddress())), Identifier(Mac48Address::ConvertFrom(grpDevs.Get(1)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());

  Pmipv6LmaHelper lmahelper;
  lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:1:4::"), 48);
  lmahelper.SetProfileHelper(profile);
  
  lmahelper.Install(lma.Get(0));
  
  Pmipv6MagHelper maghelper;

  maghelper.SetProfileHelper(profile);  
  
  maghelper.Install (mags.Get(0), mag1Ifs.GetAddress(0, 0), aps.Get(0));
  maghelper.Install (mags.Get(1), mag2Ifs.GetAddress(0, 0), aps.Get(1));
  maghelper.Install (mags.Get(2), mag3Ifs.GetAddress(0, 0), aps.Get(2));

  
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("pmip6-wifi.tr"));
  csma.EnablePcapAll (std::string ("pmip6-csma"), false);
  
  wifiPhy.EnablePcap ("pmip6-wifi", mag1ApDev.Get(0));
  wifiPhy.EnablePcap ("pmip6-wifi", mag2ApDev.Get(0));
  wifiPhy.EnablePcap ("pmip6-wifi", mag3ApDev.Get(0));
  wifiPhy.EnablePcap ("pmip6-wifi", staDevs.Get(0));

//  NS_LOG_INFO ("Installing UDP server on MN");
//  uint16_t port = 6000;
//  ApplicationContainer serverApps, clientApps;
//  UdpServerHelper server (port);
//  serverApps = server.Install (sta.Get(0));
//
//  NS_LOG_INFO ("Installing UDP client on CN");
//  uint32_t packetSize = 1024;
//  uint32_t maxPacketCount = 20;
//  Time interPacketInterval = MilliSeconds(1000);
//  UdpClientHelper udpClient(Ipv6Address(/*"3ffe:1:4:1:200:ff:fe00:e"*/staAddress), port);
//  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
//  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
//  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
//  clientApps = udpClient.Install (cn.Get (0));
//
//  serverApps.Start (Seconds (startTime + 1.0));
//  clientApps.Start (Seconds (startTime + 1.5));
//  serverApps.Stop (Seconds (endTime));
//  clientApps.Stop (Seconds (endTime));

  AnimationInterface anim("PMIPv6.xml");
  anim.SetMobilityPollInterval(Seconds(1));
  anim.UpdateNodeDescription(backbone.Get(0), "LMA");
  anim.UpdateNodeDescription(cn.Get(0), "CN");
  anim.UpdateNodeDescription(sta.Get(0), "MNN");
  anim.UpdateNodeDescription(grp.Get(0), "MNN1");
  anim.UpdateNodeDescription(grp.Get(1), "MNN2");
  anim.UpdateNodeDescription(aps.Get(0), "AP1");
  anim.UpdateNodeDescription(aps.Get(1), "AP2");
  anim.UpdateNodeDescription(aps.Get(2), "AP3");
  anim.UpdateNodeDescription(backbone.Get(1), "MAG1");
  anim.UpdateNodeDescription(backbone.Get(2), "MAG2");
  anim.UpdateNodeDescription(backbone.Get(3), "MAG3");

  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

