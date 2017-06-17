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

Ipv6InterfaceContainer AssignIpv6Address(NetDeviceContainer devices, Ipv6Address network = Ipv6Address("3ffe:1:4:1::"), Ipv6Prefix prefix = Ipv6Prefix (64))
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


int main (int argc, char *argv[])
{
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

  double startTime = 0.0;
  double endTime   = 50.0;
  (void) startTime; (void)endTime;

  int cnt = 4;

  NodeContainer sta;
  NodeContainer grp;
  NodeContainer cn;
  NodeContainer backbone;
  NodeContainer aps;

  //ref nodes
  NodeContainer lma;
  NodeContainer mags;
  NodeContainer outerNet;
  std::vector<NodeContainer> magNets;
  NodeContainer mag1Net;
  NodeContainer mag2Net;
  NodeContainer mag3Net;
  
  NetDeviceContainer backboneDevs;
  NetDeviceContainer outerDevs;
  std::vector<NetDeviceContainer> magDevs;
  std::vector<NetDeviceContainer> magApDevs;
  std::vector<NetDeviceContainer> magBrDevs;

  NetDeviceContainer staDevs;
  NetDeviceContainer grpDevs;
  
  Ipv6InterfaceContainer backboneIfs;
  Ipv6InterfaceContainer outerIfs;

  std::vector<Ipv6InterfaceContainer> magIfs;

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
  
  for (int i = 0; i < cnt; i++)
  {
	  mags.Add(backbone.Get(i + 1));
  }
  
  outerNet.Add(lma);
  outerNet.Add(cn);

  for (int i = 0; i < cnt; i++)
  {
	  NodeContainer magNet;
	  magNet.Add(mags.Get(i));
	  magNet.Add(aps.Get(i));
	  magNets.push_back(magNet);
  }

  CsmaHelper csma, csma1;
  
  //MAG's MAC Address (for unify default gateway of MN)
  Mac48Address magMacAddrUniq("00:00:AA:BB:CC:DD"); //unused
  //MAG's MAC Address (for unify default gateway of MN)
  std::vector<Mac48Address> magMacAddrs;
  NS_LOG_UNCOND("MAC Addresses");
  for (int i = 0; i < cnt; i++)
  {
	  std::ostringstream out("");
	  char c = 'D' + i;
	  out << "00:00:AA:BB:CC:" << c << c;
	  Mac48Address magMacAddr(out.str().c_str());
	  NS_LOG_UNCOND(out.str());
	  magMacAddrs.push_back(magMacAddr);
	  out.str("");
  }

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
  for (int i = 0; i <= cnt; i++)
  {
	  std::ostringstream out("");
	  out << "3ffe:1::";
	  out << i+1;
	  iifc = AssignIpv6Address(backboneDevs.Get(i), Ipv6Address(out.str().c_str()), 64);
	  backboneIfs.Add(iifc);
	  out.str("");
  }

  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);
  
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, -20.0, 0.0));   //LMA
  for (int i = 0; i < cnt; i++)
  {
	  positionAlloc->Add (Vector (-50.0 + (i* 100), 20.0, 0.0)); //MAGi
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (backbone);
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (75.0, -20.0, 0.0));   //CN
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (cn);
  
  positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < cnt; i++)
  {
	  positionAlloc->Add (Vector (-50.0 + (i* 100), 40.0, 0.0)); //MAGAPi
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  
  mobility.Install (aps);
  NS_LOG_UNCOND("MAG-AP Addresses");
  //Setting WLAN AP
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
  BridgeHelper bridge;
  (void)magMacAddrUniq;
  for (int i = 0; i < cnt; i++)
  {
	  magDevs.push_back(csma.Install(magNets[i]));
	  magDevs[i].Get(0)->SetAddress(magMacAddrs[i]);
	  std::ostringstream out("");
	  out << "3ffe:1:" << i+1 << "::1";
	  magIfs.push_back(AssignIpv6Address(magDevs[i].Get(0), Ipv6Address(out.str().c_str()), 64));
	  out.str("");
	  magApDevs.push_back(wifi.Install (wifiPhy, wifiMac, magNets[i].Get(1)));
	  magBrDevs.push_back(bridge.Install (aps.Get(i), NetDeviceContainer(magApDevs[i], magDevs[i].Get(1))));
	  iifc = AssignWithoutAddress(magDevs[i].Get(1));
	  magIfs[i].Add(iifc);
	  magIfs[i].SetForwarding(0, true);
	  magIfs[i].SetDefaultRouteInAllNodes(0);
  }

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

  grpDevs.Add( wifi.Install (wifiPhy, wifiMac, grp));
  NetDeviceContainer mnnDevs(staDevs, grpDevs);
  iifc = AssignIpv6Address(mnnDevs);
  NS_LOG_UNCOND("STA Address:" << mnnDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress());
  Ipv6Address staAddress = mnnDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();

  //attach PMIPv6 agents
  Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper> ();
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(grpDevs.Get(0)->GetAddress())), Identifier(Mac48Address::ConvertFrom(grpDevs.Get(0)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(grpDevs.Get(1)->GetAddress())), Identifier(Mac48Address::ConvertFrom(grpDevs.Get(1)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());

  Pmipv6LmaHelper lmahelper;
  lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:1:4::"), 48);
  lmahelper.SetProfileHelper(profile);
  lmahelper.Install(lma.Get(0));
  
  Pmipv6MagHelper maghelper;
  maghelper.SetProfileHelper(profile);
  for (int i = 0; i < cnt; i++)
  {
	  maghelper.Install (mags.Get(i), magIfs[i].GetAddress(0, 0), aps.Get(i));
  }
  
  AsciiTraceHelper ascii;
  csma.EnableAsciiAll (ascii.CreateFileStream ("pmip6-wifi.tr"));
  csma.EnablePcapAll (std::string ("pmip6-csma"), false);

  for (int i = 0; i < cnt; i++)
  {
	  wifiPhy.EnablePcap ("pmip6-wifi", magApDevs[i].Get(0));
  }
  wifiPhy.EnablePcap ("pmip6-wifi", staDevs.Get(0));

  NS_LOG_INFO ("Installing UDP server on MN");
  uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps;
  UdpServerHelper server (port);
  serverApps = server.Install (sta.Get(0));

  NS_LOG_INFO ("Installing UDP client on CN");
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 30;
  Time interPacketInterval = MilliSeconds(1000);
  UdpClientHelper udpClient(Ipv6Address(staAddress), port);
  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  clientApps = udpClient.Install (cn.Get (0));

  serverApps.Start (Seconds (startTime + 1.0));
  clientApps.Start (Seconds (startTime + 1.5));
  serverApps.Stop (Seconds (endTime));
  clientApps.Stop (Seconds (endTime));

  AnimationInterface anim("PMIPv6.xml");
  anim.SetMobilityPollInterval(Seconds(1));
  anim.UpdateNodeDescription(backbone.Get(0), "LMA");
  anim.UpdateNodeDescription(cn.Get(0), "CN");
  anim.UpdateNodeDescription(sta.Get(0), "MNN");
  anim.UpdateNodeDescription(grp.Get(0), "MNN1");
  anim.UpdateNodeDescription(grp.Get(1), "MNN2");
  for (int i = 0; i < cnt; i++)
  {
	  std::ostringstream out("");
	  out << "AP" << i+1;
	  anim.UpdateNodeDescription(aps.Get(i), out.str().c_str());
	  out.str("");
	  out << "MAG" << i+1;
	  anim.UpdateNodeDescription(backbone.Get(i+1), out.str().c_str());
	  out.str("");
  }

  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

