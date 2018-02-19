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
#include "ns3/point-to-point-module.h"
#include "ns3/ipv6-static-routing-helper.h"


#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-static-source-routing.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/netanim-module.h"
#include "ns3/group-finder-helper.h"
#include "ns3/velocity-sensor-helper.h"
#include "ns3/ns2-mobility-helper.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("Pmipv6Wifi");

using namespace ns3;

Ipv6InterfaceContainer ASSIGN_SingleIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix)
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

Ipv6InterfaceContainer ASSIGN_Ipv6Addresses(NetDeviceContainer devices, Ipv6Address network, Ipv6Prefix prefix)
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

Ipv6InterfaceContainer ASSIGN_WithoutAddress(Ptr<NetDevice> device)
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

void LOG_Settings()
{
	//  LogLevel logAll = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
	//  LogLevel logLogic = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_LOGIC);
	//  LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);
	    LogLevel logDbg = static_cast<LogLevel>(LOG_LEVEL_DEBUG);
	    (void)logDbg;
	//  LogComponentEnable ("Udp6Server", logInfo);
	//  LogComponentEnable ("Pmipv6Agent", logAll);
	//  LogComponentEnable ("Pmipv6MagNotifier", logAll);
	//  LogComponentEnable ("Pmipv6Wifi", logDbg);
	//  LogComponentEnable ("Pmipv6MagNotifier", logDbg);
	//  LogComponentEnable ("Pmipv6Mag", logDbg);
	    LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	    LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
}

void INSTALL_ConstantMobility(NodeContainer &nc, Ptr<ListPositionAllocator> positionAlloc)
{
	MobilityHelper mobility;
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (nc);
}

namespace containers
{
	NodeContainer mnns;
	NodeContainer leader;
	NodeContainer followers;
	NodeContainer cn;
	NodeContainer lmaMagNodes;
	NodeContainer aps;

	//ref nodes
	NodeContainer lma;
	NodeContainer mags;
	NodeContainer lmaCnNodes;
	std::vector<NodeContainer> magApPairNodes;
	//MAC Address for MAGs
	std::vector<Mac48Address> magMacAddrs;

	NetDeviceContainer lmaMagDevs;
	NetDeviceContainer lmaCnDevs;
	std::vector<NetDeviceContainer> magApPairDevs;
	std::vector<NetDeviceContainer> apDevs;
	std::vector<NetDeviceContainer> magBrDevs;

	NetDeviceContainer mnnsExtDevs;
	NetDeviceContainer mnnsIntDevs;
	NetDeviceContainer leaderDev;
	NetDeviceContainer followerDevs;

	Ipv6InterfaceContainer backboneIfs;
	Ipv6InterfaceContainer outerIfs;

	std::vector<Ipv6InterfaceContainer> magIfs;

	Ipv6InterfaceContainer glIfs;
	Ipv6InterfaceContainer grpIfs;
}
using namespace containers;

void PRINIT_MNNs_DeviceInfor(const std::string &preface)
{
	NS_LOG_UNCOND("==================\nPrint " << preface << ":");
	for (size_t i = 0; i < mnns.GetN (); i++)
		if (mnns.Get(i)->GetObject<Ipv4> ())
			NS_LOG_UNCOND("MNN" << i << " Devices:" << mnns.Get(i)->GetNDevices() << " Interfaces:" << mnns.Get(i)->GetObject<Ipv4> ()->GetNInterfaces());
		else
			NS_LOG_UNCOND("MNN" << i << " Devices:" << mnns.Get(i)->GetNDevices());
	NS_LOG_UNCOND("==================");
}

#define nof_street_mags  6
#define nof_streets  2
const int backBoneCnt = nof_street_mags * nof_streets;
const double startTime = 0.0;
const double endTime   = 150.0;
Ipv6Address destAddress;
Ptr<Node> destNode;

void CREATE_Nodes()
{
	  mnns.Create(3);
	  leader.Add(mnns.Get(0));
	  lmaMagNodes.Create(backBoneCnt + 1);
	  aps.Create(backBoneCnt);
	  cn.Create(1);
	  NS_LOG_UNCOND("LEADER: " << mnns.Get(0)->GetId());
	  for (size_t i = 1; i < mnns.GetN(); i++)
		  followers.Add(mnns.Get(i));

	  lma.Add(lmaMagNodes.Get(0));

	  for (size_t i = 0; i < backBoneCnt; i++)
	  {
		  mags.Add(lmaMagNodes.Get(i + 1));
	  }

	  lmaCnNodes.Add(lma);
	  lmaCnNodes.Add(cn);

	  for (int i = 0; i < backBoneCnt; i++)
	  {
		  NodeContainer magApPair;
		  magApPair.Add(mags.Get(i));
		  magApPair.Add(aps.Get(i));
		  magApPairNodes.push_back(magApPair);
	  }
}

void INSTALL_InternetStack()
{
	  InternetStackHelper internet;
	  internet.Install (lmaMagNodes);
	  internet.Install (aps);
	  internet.Install (cn);
	  internet.Install (leader);
	  internet.Install (followers);
}

void ASSIGN_MAG_MAC_Addresses()
{
  for (int i = 0; i < backBoneCnt; i++)
  {
	  std::ostringstream out("");
	  char c = 'D' + i;
	  out << "00:00:AA:BB:CC:" << c << c;
	  Mac48Address magMacAddr(out.str().c_str());
	  NS_LOG_UNCOND(out.str());
	  magMacAddrs.push_back(magMacAddr);
	  out.str("");
  }
}

void INIT_Csma(CsmaHelper &csma, uint64_t dataRateBps = 50000000, int64_t delay = 100, uint64_t mtu = 1400)
{
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
}

Ptr<Pmipv6ProfileHelper> ENABLE_LMA_Profiling()
{
	//LMA Profiling
	Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper> ();
	for(unsigned int i = 0; i < mnnsExtDevs.GetN(); i++)
	{
	  NS_LOG_UNCOND("Profile add MNN:" << i << " :" << Mac48Address::ConvertFrom(mnnsExtDevs.Get(i)->GetAddress()));
	  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(mnnsExtDevs.Get(i)->GetAddress())), Identifier(Mac48Address::ConvertFrom(mnnsExtDevs.Get(i)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
	}
	return profile;
}

void INIT_MNN_Mobility()
{
	MobilityHelper mobility;
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install(mnns);

//	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//	positionAlloc->Add (Vector (-50.0, 50.0, 0.0)); //GL
//	mobility.SetPositionAllocator (positionAlloc);
//	mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
//	mobility.Install(leader);
//	positionAlloc = CreateObject<ListPositionAllocator> ();
//
//	for(unsigned int i = 0; i < followers.GetN(); i++)
//	{
//		  positionAlloc->Add (Vector (-20.0, (i*10) + 20.0, 0.0)); //MNNs
//	}
//	mobility.SetPositionAllocator (positionAlloc);
//	mobility.PushReferenceMobilityModel(leader.Get (0));
//	mobility.Install(followers);
//
//  //GL movement
//  Ptr<ConstantVelocityMobilityModel> cvm = leader.Get(0)->GetObject<ConstantVelocityMobilityModel>();
//  cvm->SetVelocity(Vector (10.0, 0, 0)); //move to left to right 10.0m/s
}

void INIT_VelocitySensor(double interval)
{
	//Server Application
	ApplicationContainer velocitySensor;

	VelocitySensorHelper vs(Seconds (interval));
	//do settings
	velocitySensor = vs.Install(NodeContainer(leader, followers));
	velocitySensor.Start (Seconds (startTime));
	velocitySensor.Stop (Seconds (endTime));
}

void INIT_GrpFinder()
{
	//Server Application
	ApplicationContainer grpFinder;

	GroupFinderHelper::SetEnable(true);
	GroupFinderHelper gf;
	//do settings
	gf.SetGroup(NetDeviceContainer(leaderDev, followerDevs));
//	gf.SetGroup(mnnsExtDevs);
	grpFinder = gf.Install(mnns);
	grpFinder.Start (Seconds (startTime));
	grpFinder.Stop (Seconds (endTime));
}

void INIT_UdpApp()
{
	  uint16_t port = 6000;
	  ApplicationContainer serverApps, clientApps;
	  UdpServerHelper server (port);
	  serverApps = server.Install (destNode);

	  //Clinet Application
	  NS_LOG_UNCOND("Installing UDP client on CN");
	  uint32_t packetSize = 1024;
	  uint32_t maxPacketCount = 6000;
	  Time interPacketInterval = MilliSeconds(1000);
	  UdpClientHelper udpClient(destAddress, port);
	  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
	  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
	  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
	  clientApps = udpClient.Install (cn.Get (0));

	  serverApps.Start (Seconds (startTime + 1.0));
	  clientApps.Start (Seconds (startTime + 1.5));
	  serverApps.Stop (Seconds (endTime));
	  clientApps.Stop (Seconds (endTime));
}

void INIT_Anim(AnimationInterface &anim)
{
	anim.SetMobilityPollInterval(Seconds(1));
	anim.UpdateNodeDescription(lmaMagNodes.Get(0), "LMA");
	anim.UpdateNodeDescription(cn.Get(0), "CN");
	anim.UpdateNodeDescription(leader.Get(0), "MNN");
	for (int i = 0; i < backBoneCnt; i++)
	{
	  std::ostringstream out("");
	  out << "AP" << i+1;
	  anim.UpdateNodeDescription(aps.Get(i), out.str().c_str());
	  out.str("");
	  out << "MAG" << i+1;
	  anim.UpdateNodeDescription(lmaMagNodes.Get(i+1), out.str().c_str());
	  out.str("");
	}

	for(unsigned int i = 0; i < followers.GetN(); i++)
	{
	  std::ostringstream out("");
	  out << "MN" << i+1;
	  anim.UpdateNodeDescription(followers.Get(i), out.str().c_str());
	  out.str("");
	}
}

void INIT_Pmip()
{
	Ptr<Pmipv6ProfileHelper> profile = ENABLE_LMA_Profiling();
//	ENABLE_LMA_Profiling();

	//LMA Helper
	NS_LOG_UNCOND("LMA Helper");
	Pmipv6LmaHelper lmahelper;
	lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:6:6::"), 48);
	lmahelper.SetProfileHelper(profile);
	lmahelper.Install(lma.Get(0));

	//MAG Helper
	NS_LOG_UNCOND("MAG Helper");
	Pmipv6MagHelper maghelper;
	maghelper.SetProfileHelper(profile);
	for (int i = 0; i < backBoneCnt; i++)
	{
	  maghelper.Install (mags.Get(i), magIfs[i].GetAddress(0, 0), aps.Get(i));
	}
}

// Prints actual position and velocity when a course change event occurs
static void
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  // Prints position and velocities
  *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL: x= " << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}

int main (int argc, char *argv[])
{
  (void) startTime; (void)endTime;

  CommandLine cmd;
  cmd.Parse (argc, argv);
  LOG_Settings();
  SeedManager::SetSeed (123456);

  CREATE_Nodes();

  PRINIT_MNNs_DeviceInfor("After node creation");
  INSTALL_InternetStack();
  PRINIT_MNNs_DeviceInfor("After Installing Internet stack");
  NS_LOG_UNCOND("MAC Addresses:");
  ASSIGN_MAG_MAC_Addresses();
  PRINIT_MNNs_DeviceInfor("After MAC Address");
  NS_LOG_UNCOND("Outer Network:");
  CsmaHelper csmaLmaCn;
  INIT_Csma(csmaLmaCn);
  PRINIT_MNNs_DeviceInfor("After INIT_Csma csmaLmaCn");
  Ipv6InterfaceContainer iifc;

  //Outer Dev CSMA and Addressing
  //Link between CN and LMA is 50Mbps and 0.1ms delay
  lmaCnDevs = csmaLmaCn.Install(lmaCnNodes);
  iifc = ASSIGN_SingleIpv6Address(lmaCnDevs.Get(0), Ipv6Address("3ffe:2::1"), 64);
  outerIfs.Add(iifc);
  iifc = ASSIGN_SingleIpv6Address(lmaCnDevs.Get(1), Ipv6Address("3ffe:2::2"), 64);
  outerIfs.Add(iifc);
  outerIfs.SetForwarding(0, true);
  outerIfs.SetDefaultRouteInAllNodes(0);
  PRINIT_MNNs_DeviceInfor("After lmaCnDevs");
  NS_LOG_UNCOND("LMA MAG Network:");
  CsmaHelper csmaLmaMag;
  INIT_Csma(csmaLmaMag);
  PRINIT_MNNs_DeviceInfor("After INIT_Csma INIT_Csma");
  //All Link is 50Mbps and 0.1ms delay
  //Backbone Addressing
  NS_LOG_UNCOND("Assign lmaMagNodes Addresses");
  lmaMagDevs = csmaLmaMag.Install(lmaMagNodes);
  for (int i = 0; i <= backBoneCnt; i++)
  {
	  std::ostringstream out("");
	  out << "3ffe:1::";
	  out << i+1;
	  iifc = ASSIGN_SingleIpv6Address(lmaMagDevs.Get(i), Ipv6Address(out.str().c_str()), 64);
	  backboneIfs.Add(iifc);
	  out.str("");
  }
  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);
  PRINIT_MNNs_DeviceInfor("After Assign lmaMagNodes Addresses");

  //Backbone Mobility
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 20.0, 0.0));   //LMA
  std::cout << "MAGs:\n";
  for (int i = 0; i < nof_street_mags; i++)
  {
	  double x = 50.0 + (i* 100);
	  for (int j = 0; j < nof_streets ; j++)
	  {
		  double y = 60.0 + (150 * j);
		  std::cout << x << "," << y << "  ";
		  positionAlloc->Add (Vector (x, y, 0.0)); //MAGi
	  }
	  std::cout << "\n";
  }
  INSTALL_ConstantMobility(lmaMagNodes, positionAlloc);

  //CN Mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (75.0, 20.0, 0.0));   //CN
  INSTALL_ConstantMobility(cn, positionAlloc);

  //AP mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  std::cout << "\nAPss:\n";
  for (int i = 0; i < nof_street_mags; i++)
  {
	  double x = 50.0 + (i* 100);
	  for (int j = 0; j < nof_streets; j++)
	  {
		  double y = 80.0 + (150 * j);
		  std::cout << x << "," << y << "  ";
		  positionAlloc->Add (Vector (x, y, 0.0)); //MAGAPi
	  }
	  std::cout << "\n";
  }
  std::cout << "\n";
  INSTALL_ConstantMobility(aps, positionAlloc);

  PRINIT_MNNs_DeviceInfor("Before ANY Wifi Installation");
  //Wifi
  NS_LOG_UNCOND("MAG-AP Addresses:");
  Ssid ssid = Ssid("MAG");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  WifiHelper wifi = WifiHelper::Default ();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (28));
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiMac.SetType ("ns3::ApWifiMac",
		           "Ssid", SsidValue (ssid),
		           "BeaconGeneration", BooleanValue (true),
		           "BeaconInterval", TimeValue (MicroSeconds (102400)));

  //MAG Wifi Bridging and addressing
  BridgeHelper bridge;
  for (int i = 0; i < backBoneCnt; i++)
  {
	  magApPairDevs.push_back(csmaLmaMag.Install(magApPairNodes[i]));
	  magApPairDevs[i].Get(0)->SetAddress(magMacAddrs[i]);
	  std::ostringstream out("");
	  out << "3ffe:1:" << i+1 << "::1";
	  magIfs.push_back(ASSIGN_SingleIpv6Address(magApPairDevs[i].Get(0), Ipv6Address(out.str().c_str()), 64));
	  out.str("");
	  apDevs.push_back(wifi.Install (wifiPhy, wifiMac, magApPairNodes[i].Get(1)));
	  magBrDevs.push_back(bridge.Install (aps.Get(i), NetDeviceContainer(apDevs[i], magApPairDevs[i].Get(1))));
	  iifc = ASSIGN_WithoutAddress(magApPairDevs[i].Get(1));
	  magIfs[i].Add(iifc);
	  magIfs[i].SetForwarding(0, true);
	  magIfs[i].SetDefaultRouteInAllNodes(0);
  }

  std::ostringstream magOut(""), apOut("");
  for (int i = 0; i < backBoneCnt; i++)
  {
	  magOut << "MAG" << i << " Addresses: " << magIfs[i].GetAddress(0,0) << " and " << magIfs[i].GetAddress(0,1) << "\n";
	  apOut << "AP" << i << " Mac Addresses: " << magApPairDevs[i].Get(1)->GetAddress() << "\n";
  }
  NS_LOG_UNCOND(magOut.str() << apOut.str());

  INIT_MNN_Mobility();

  PRINIT_MNNs_DeviceInfor("Before MNNs Wifi Installation");
  NS_LOG_UNCOND("Create EXTERNAL networks and assign MNN Addresses.");

  //GL Wifi
  wifiMac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));
  mnnsExtDevs = wifi.Install (wifiPhy, wifiMac, mnns);
  iifc = ASSIGN_Ipv6Addresses(mnnsExtDevs, Ipv6Address("3ffe:6:6:1::"), 64);
  iifc.SetForwarding (0, true);
  iifc.SetDefaultRouteInAllNodes (0);
  PRINIT_MNNs_DeviceInfor("After EXTERNAL MNNs StaWifi Installation");

  leaderDev.Add(mnnsExtDevs.Get(0));
  for (size_t i = 1; i < mnnsExtDevs.GetN(); i++)
	  followerDevs.Add(mnnsExtDevs.Get(i));

  NS_LOG_UNCOND("Create INTERNAL networks and assign MNN Addresses.");
  //GL Wifi
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiMac.SetType ("ns3::AdhocWifiMac");
  mnnsIntDevs = wifi.Install (wifiPhy, wifiMac, mnns);
  iifc = ASSIGN_Ipv6Addresses(mnnsIntDevs, Ipv6Address("3ffe:6:6:2::"), 64);
  iifc.SetForwarding (0, true);
  PRINIT_MNNs_DeviceInfor("After INTERNAL MNNs StaWifi Installation");

  //End addresses
  Ipv6Address glExternalAddress = mnnsExtDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();
  Ipv6Address glInternalAddress = mnnsExtDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();
  Ipv6Address mnn2ExternalAddress = mnnsExtDevs.Get(2)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();

  destAddress = glExternalAddress;
  destNode = leader.Get(0);
//  destAddress = mnn2ExternalAddress;
//  destNode = mnns.Get(2);

  NS_LOG_UNCOND("GL Address:" << glExternalAddress);
  NS_LOG_UNCOND("Mnn2 Address:" << mnn2ExternalAddress);
  NS_LOG_UNCOND("Dest Address:" << destAddress);
  
  INIT_Pmip();
  PRINIT_MNNs_DeviceInfor("After PMIP init");

  NS_LOG_UNCOND("Routes");
  //Routes From MAG
  Ptr<Ipv6> ipv6gl = leader.Get(0)->GetObject<Ipv6> ();
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> sr;
  int32_t next;
  next = ipv6gl->GetInterfaceForDevice(leader.Get(0)->GetDevice(1));
  NS_LOG_UNCOND("next:" << next);
  for (int i = 1; i <= backBoneCnt; i++)
  {
	  Ptr<Ipv6> ipv6mag =   lmaMagNodes.Get(i)->GetObject<Ipv6> ();
	  sr = ipv6RoutingHelper.GetStaticRouting (ipv6mag);
	  sr->AddHostRouteTo (destAddress, ipv6gl->GetAddress(next, 1).GetAddress(), 2);
	  NS_LOG_UNCOND("MAG:" << i << ": AddHostRouteTo: " << destAddress << " --> " << ipv6gl->GetAddress(next, 1).GetAddress() << " 2");
  }
  //Routes From STA (GL)
  sr = ipv6RoutingHelper.GetStaticRouting (ipv6gl);
  Ptr<Ipv6> ip6Mnn1 = mnns.Get(1)->GetObject<Ipv6> ();
  next = ip6Mnn1->GetInterfaceForDevice(mnns.Get(1)->GetDevice(2));
  NS_LOG_UNCOND("next:" << next);
  sr->AddHostRouteTo (destAddress, ip6Mnn1->GetAddress(next, 1).GetAddress(), 1);
  NS_LOG_UNCOND("MNN0(GL): AddHostRouteTo: " << destAddress << " --> " << ip6Mnn1->GetAddress(next, 1).GetAddress() << " 1");
  //Routes From relay node
  sr = ipv6RoutingHelper.GetStaticRouting (ip6Mnn1);
  Ptr<Ipv6> ip6Mnn2 = mnns.Get(2)->GetObject<Ipv6> ();
  next = ip6Mnn2->GetInterfaceForDevice(mnns.Get(2)->GetDevice(2));
  NS_LOG_UNCOND("next:" << next);
  sr->AddHostRouteTo (destAddress, ip6Mnn2->GetAddress(next, 1).GetAddress(), 2);
  NS_LOG_UNCOND("MNN1: AddHostRouteTo: " << destAddress << " --> " << ip6Mnn2->GetAddress(next, 1).GetAddress() << " 2");


  //Pcap
  AsciiTraceHelper ascii;

  csmaLmaMag.EnablePcap(std::string ("csma-lma-mag"), lmaMagDevs, false);
  csmaLmaMag.EnablePcap(std::string ("csma-lma-cn"), lmaCnDevs, false);

  for (int i = 0; i < backBoneCnt; i++)
  {
	  wifiPhy.EnablePcap ("wifi-ap", apDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-mag->ap", magApPairDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-ap", magApPairDevs[i].Get(1));
  }

  wifiPhy.EnablePcap ("wifi-ext-mnns", mnnsExtDevs);
  wifiPhy.EnablePcap ("wifi-int-mnns", mnnsIntDevs);

  NS_LOG_UNCOND("Installing SENSOR");

  INIT_VelocitySensor(5.0);

  NS_LOG_UNCOND("Installing GRP FINDER");
  INIT_GrpFinder();

  //Application
  NS_LOG_UNCOND("Installing UDP server on MN");
  INIT_UdpApp();

  NS_LOG_UNCOND("Animator Settings");
  AnimationInterface anim("PMIPv6_TRACE.xml");
//  anim.SetMaxPktsPerTraceFile(300000);
  INIT_Anim(anim);
  std::string cur_path =
		  "/home/user/Downloads/ns/rts/"
		  "ns-allinone-3.22.14-jul-2017/ns-allinone-3.22/ns-3.22/";
  std::string traceFile(cur_path + "mnn_trace.tcl");
  std::string logFile(cur_path + "mnn_trace.log");
  // Create Ns2MobilityHelper with the specified trace log file as parameter
  Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

  ns2.Install (); // configure movements for each node, while reading trace file

  // open log file for output
  std::ofstream os;
  os.open (logFile.c_str ());

  // Configure callback for logging
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                   MakeBoundCallback (&CourseChange, &os));


  PRINIT_MNNs_DeviceInfor("END");

  //Run
  NS_LOG_UNCOND("Run");
  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
