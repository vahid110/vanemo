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
#include "ns3/group-finder.h"

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

void logSettings()
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

void installConstantMobility(NodeContainer &nc, Ptr<ListPositionAllocator> positionAlloc)
{
	MobilityHelper mobility;
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (nc);
}

namespace containers
{
	NodeContainer sta;
	NodeContainer grp;
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

	NetDeviceContainer staDevs;
	NetDeviceContainer grpDevs;

	Ipv6InterfaceContainer backboneIfs;
	Ipv6InterfaceContainer outerIfs;

	std::vector<Ipv6InterfaceContainer> magIfs;

	Ipv6InterfaceContainer staIfs;
	Ipv6InterfaceContainer grpIfs;
}
using namespace containers;

void printMnnsDeviceInfor(const std::string &preface)
{
	  NS_LOG_UNCOND(preface << ".\nSTA  Devices:" << sta.Get(0)->GetNDevices() << " Interfaces:" << sta.Get(0)->GetObject<Ipv4> ()->GetNInterfaces());
	  NS_LOG_UNCOND("MN1  Devices:" << grp.Get(0)->GetNDevices() << " Interfaces:" << grp.Get(0)->GetObject<Ipv4> ()->GetNInterfaces());
	  NS_LOG_UNCOND("MN2  Devices:" << grp.Get(1)->GetNDevices() << " Interfaces:" << grp.Get(1)->GetObject<Ipv4> ()->GetNInterfaces());
}

const int cnt = 4;
void createNodes()
{
	  lmaMagNodes.Create(cnt + 1);
	  aps.Create(cnt);
	  cn.Create(1);
	  sta.Create(1);
	  grp.Create(2);

	  lma.Add(lmaMagNodes.Get(0));

	  for (int i = 0; i < cnt; i++)
	  {
		  mags.Add(lmaMagNodes.Get(i + 1));
	  }

	  lmaCnNodes.Add(lma);
	  lmaCnNodes.Add(cn);

	  for (int i = 0; i < cnt; i++)
	  {
		  NodeContainer magApPair;
		  magApPair.Add(mags.Get(i));
		  magApPair.Add(aps.Get(i));
		  magApPairNodes.push_back(magApPair);
	  }
}

void installInternetStack()
{
	  InternetStackHelper internet;
	  internet.Install (lmaMagNodes);
	  internet.Install (aps);
	  internet.Install (cn);
	  internet.Install (sta);
	  internet.Install (grp);
}

void assignMagMacAddresses()
{
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
}

void initCsma(CsmaHelper &csma, uint64_t dataRateBps = 50000000, int64_t delay = 100, uint64_t mtu = 1400)
{
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
}

int main (int argc, char *argv[])
{

  double startTime = 0.0;
  double endTime   = 30.0;
  (void) startTime; (void)endTime;

  CommandLine cmd;
  cmd.Parse (argc, argv);
  logSettings();
  SeedManager::SetSeed (123456);

  createNodes();
  installInternetStack();
  NS_LOG_UNCOND("MAC Addresses:");
  assignMagMacAddresses();

  NS_LOG_UNCOND("Outer Network:");
  CsmaHelper csmaLmaCn;
  initCsma(csmaLmaCn);

  Ipv6InterfaceContainer iifc;

  //Outer Dev CSMA and Addressing
  //Link between CN and LMA is 50Mbps and 0.1ms delay
  lmaCnDevs = csmaLmaCn.Install(lmaCnNodes);
  iifc = AssignIpv6Address(lmaCnDevs.Get(0), Ipv6Address("3ffe:2::1"), 64);
  outerIfs.Add(iifc);
  iifc = AssignIpv6Address(lmaCnDevs.Get(1), Ipv6Address("3ffe:2::2"), 64);
  outerIfs.Add(iifc);
  outerIfs.SetForwarding(0, true);
  outerIfs.SetDefaultRouteInAllNodes(0);

  NS_LOG_UNCOND("LMA MAG Network:");
  CsmaHelper csmaLmaMag;
  initCsma(csmaLmaMag);
  //All Link is 50Mbps and 0.1ms delay
  //Backbone Addressing
  NS_LOG_UNCOND("Assign lmaMagNodes Addresses");
  lmaMagDevs = csmaLmaMag.Install(lmaMagNodes);
  for (int i = 0; i <= cnt; i++)
  {
	  std::ostringstream out("");
	  out << "3ffe:1::";
	  out << i+1;
	  iifc = AssignIpv6Address(lmaMagDevs.Get(i), Ipv6Address(out.str().c_str()), 64);
	  backboneIfs.Add(iifc);
	  out.str("");
  }
  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);

  //Backbone Mobility
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, -20.0, 0.0));   //LMA
  for (int i = 0; i < cnt; i++)
  {
	  positionAlloc->Add (Vector (-50.0 + (i* 100), 20.0, 0.0)); //MAGi
  }
  installConstantMobility(lmaMagNodes, positionAlloc);

  //CN Mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (75.0, -20.0, 0.0));   //CN
  installConstantMobility(cn, positionAlloc);

  //AP mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < cnt; i++)
  {
	  positionAlloc->Add (Vector (-50.0 + (i* 100), 40.0, 0.0)); //MAGAPi
  }
  installConstantMobility(aps, positionAlloc);

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
  for (int i = 0; i < cnt; i++)
  {
	  magApPairDevs.push_back(csmaLmaMag.Install(magApPairNodes[i]));
	  magApPairDevs[i].Get(0)->SetAddress(magMacAddrs[i]);
	  std::ostringstream out("");
	  out << "3ffe:1:" << i+1 << "::1";
	  magIfs.push_back(AssignIpv6Address(magApPairDevs[i].Get(0), Ipv6Address(out.str().c_str()), 64));
	  out.str("");
	  apDevs.push_back(wifi.Install (wifiPhy, wifiMac, magApPairNodes[i].Get(1)));
	  magBrDevs.push_back(bridge.Install (aps.Get(i), NetDeviceContainer(apDevs[i], magApPairDevs[i].Get(1))));
	  iifc = AssignWithoutAddress(magApPairDevs[i].Get(1));
	  magIfs[i].Add(iifc);
	  magIfs[i].SetForwarding(0, true);
	  magIfs[i].SetDefaultRouteInAllNodes(0);
  }
  for (int i = 0; i < cnt; i++)
  {
	  NS_LOG_UNCOND("MAG" << i << " Addresses: " << magIfs[i].GetAddress(0,0) << " and " << magIfs[i].GetAddress(0,1));
  }
  for (int i = 0; i < cnt; i++)
  {
	  NS_LOG_UNCOND("AP" << i << " Mac Addresses: " << magApPairDevs[i].Get(1)->GetAddress());
  }

  //STA Mobility
  MobilityHelper mobility;
  NS_LOG_UNCOND ("Create networks and assign MNN Addresses.");
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (-50.0, 50.0, 0.0)); //STA
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");  
  mobility.Install(sta);

  //STA movement
  Ptr<ConstantVelocityMobilityModel> cvm = sta.Get(0)->GetObject<ConstantVelocityMobilityModel>();
  cvm->SetVelocity(Vector (10.0, 0, 0)); //move to left to right 10.0m/s
  //STA Wifi
  wifiMac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));
  //printMnnsDeviceInfor("WIS");

  staDevs.Add( wifi.Install (wifiPhy, wifiMac, sta));
  //printMnnsDeviceInfor("WIS_");
  NS_LOG_UNCOND("STA Mac Addresses: " << staDevs.Get(0)->GetAddress());

  //MNNs Mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();

  for(unsigned int i = 0; i < grp.GetN(); i++)
  {
	  positionAlloc->Add (Vector (-20.0, (i*10) + 20.0, 0.0)); //MNNs
  }
  mobility.SetPositionAllocator (positionAlloc);
  mobility.PushReferenceMobilityModel(sta.Get (0));
  mobility.Install(grp);
  //printMnnsDeviceInfor("WIG");

  //Mobile Addressing
  grpDevs.Add( wifi.Install (wifiPhy, wifiMac, grp));
  //printMnnsDeviceInfor("WIG");
  NetDeviceContainer mnnDevs(staDevs, grpDevs);
  iifc = AssignIpv6Address(mnnDevs);
  //MNN routing
  iifc.SetForwarding (0, true);
  iifc.SetDefaultRouteInAllNodes (0);
  //printMnnsDeviceInfor("ASG");

  //End addresses
  Ipv6Address staAddress = mnnDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();
  Ipv6Address mnn2Address = mnnDevs.Get(2)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();

  NS_LOG_UNCOND("STA Address:" << staAddress);
  NS_LOG_UNCOND("Mnn2 Address:" << mnn2Address);
//  Ipv6Address &destAddress = staAddress;
//  Ptr<Node> destNode = sta.Get(0);
  Ipv6Address &destAddress = mnn2Address;
  Ptr<Node> destNode = grp.Get(1);

  //LMA Profiling
  Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper> ();
  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), Identifier(Mac48Address::ConvertFrom(staDevs.Get(0)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  for(unsigned int i = 0; i < grpDevs.GetN(); i++)
  {
	  profile->AddProfile(Identifier(Mac48Address::ConvertFrom(grpDevs.Get(i)->GetAddress())), Identifier(Mac48Address::ConvertFrom(grpDevs.Get(i)->GetAddress())), backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  }

  //LMA Helper
  NS_LOG_UNCOND("LMA Helper");
  Pmipv6LmaHelper lmahelper;
  lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:1:4::"), 48);
  lmahelper.SetProfileHelper(profile);
  lmahelper.Install(lma.Get(0));
  
  //MAG Helper
  NS_LOG_UNCOND("MAG Helper");
  Pmipv6MagHelper maghelper;
  maghelper.SetProfileHelper(profile);
  for (int i = 0; i < cnt; i++)
  {
	  maghelper.Install (mags.Get(i), magIfs[i].GetAddress(0, 0), aps.Get(i));
  }
  
  //P2P
  NS_LOG_UNCOND("P2P");
  printMnnsDeviceInfor("P2P Before");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NodeContainer p2pNodes(sta, grp.Get(1));
  NetDeviceContainer p2pDevs = p2p.Install(p2pNodes);
  printMnnsDeviceInfor("P2P Install");
  Ptr<Ipv6> ipv6sta = sta.Get(0)->GetObject<Ipv6> ();
  int32_t staP2pIfx = ipv6sta->AddInterface(p2pDevs.Get(0));
  (void)staP2pIfx;

  Ptr<Ipv6> ipv6mn2 = grp.Get(1)->GetObject<Ipv6> ();
  int32_t mn2P2pIfx = ipv6mn2->AddInterface(p2pDevs.Get(1));
  (void)mn2P2pIfx;
  printMnnsDeviceInfor("P2P Interface");
  AssignIpv6Address(p2pDevs, Ipv6Address("3ffe:4:4:4::"), 64);
  printMnnsDeviceInfor("P2P Assign");
  NS_LOG_UNCOND("P2P Addresses:");
  int32_t t = ipv6sta->GetInterfaceForDevice(sta.Get(0)->GetDevice(2));
  int32_t t1 = ipv6mn2->GetInterfaceForDevice(grp.Get(1)->GetDevice(2));
  NS_LOG_UNCOND(ipv6sta->GetAddress(t, 1).GetAddress() << " --> " << ipv6mn2->GetAddress(t1, 1).GetAddress());

  // Routes
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6StaticRouting> sr;
  int32_t next;
  Ptr<Ipv6> ipv6mag1 = lmaMagNodes.Get(1)->GetObject<Ipv6> ();
  sr = ipv6RoutingHelper.GetStaticRouting (ipv6mag1);
  next = ipv6sta->GetInterfaceForDevice(sta.Get(0)->GetDevice(1));
  sr->AddHostRouteTo (destAddress, ipv6sta->GetAddress(next, 1).GetAddress(), 2);
  NS_LOG_UNCOND("MAG: AddHostRouteTo: " << destAddress << " --> " << ipv6sta->GetAddress(next, 1).GetAddress() << " 2");

  sr = ipv6RoutingHelper.GetStaticRouting (ipv6sta);
  next = ipv6mn2->GetInterfaceForDevice(grp.Get(1)->GetDevice(2));
  sr->AddHostRouteTo (destAddress, ipv6mn2->GetAddress(next, 1).GetAddress(), 2);
  NS_LOG_UNCOND("STA: AddHostRouteTo: " << destAddress << " --> " << ipv6mn2->GetAddress(next, 1).GetAddress() << " 2");

  p2p.EnablePcapAll ("p2p");

  //Pcap
  AsciiTraceHelper ascii;
//  csmaLmaMag.EnableAsciiAll (ascii.CreateFileStream ("pmip6-wifi.tr"));
//  csmaLmaMag.EnablePcapAll (std::string ("pmip6-csmaLmaMag"), false);
  csmaLmaMag.EnablePcap(std::string ("csma-lma-mag"), lmaMagDevs, false);
  csmaLmaMag.EnablePcap(std::string ("csma-lma-cn"), lmaCnDevs, false);



  for (int i = 0; i < cnt; i++)
  {
	  wifiPhy.EnablePcap ("wifi-ap", apDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-mag->ap", magApPairDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-ap", magApPairDevs[i].Get(1));
  }
  wifiPhy.EnablePcap ("wifi-sta", staDevs.Get(0));
  wifiPhy.EnablePcap ("wifi-mnn", mnnDevs.Get(2));

  //Server Application

  NS_LOG_UNCOND("Installing UDP server on MN");
  uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps, grpFinder;

  GroupFinderHelper::SetEnable(true);
  GroupFinderHelper gf;
  //do settings
  gf.SetGroup(grpDevs);
  grpFinder = gf.Install(sta.Get(0));

  UdpServerHelper server (port);
  serverApps = server.Install (destNode);

  //Clinet Application
  NS_LOG_UNCOND("Installing UDP client on CN");
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 30;
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
  grpFinder.Start (Seconds (startTime));
  grpFinder.Stop (Seconds (endTime));
  //Anim
  AnimationInterface anim("PMIPv6.xml");
  anim.SetMobilityPollInterval(Seconds(1));
  anim.UpdateNodeDescription(lmaMagNodes.Get(0), "LMA");
  anim.UpdateNodeDescription(cn.Get(0), "CN");
  anim.UpdateNodeDescription(sta.Get(0), "MNN");
  for (int i = 0; i < cnt; i++)
  {
	  std::ostringstream out("");
	  out << "AP" << i+1;
	  anim.UpdateNodeDescription(aps.Get(i), out.str().c_str());
	  out.str("");
	  out << "MAG" << i+1;
	  anim.UpdateNodeDescription(lmaMagNodes.Get(i+1), out.str().c_str());
	  out.str("");
  }

  for(unsigned int i = 0; i < grp.GetN(); i++)
  {
	  std::ostringstream out("");
	  out << "MN" << i+1;
	  anim.UpdateNodeDescription(grp.Get(i), out.str().c_str());
	  out.str("");
  }

  printMnnsDeviceInfor("END");

  //Run
  NS_LOG_UNCOND("Run");
  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
