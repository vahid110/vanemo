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

#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-static-source-routing.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/netanim-module.h"
#include "position.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

NS_LOG_COMPONENT_DEFINE("Pmipv6Wifi");

using namespace ns3;
using namespace std;

Ipv6InterfaceContainer
AssignIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix)
{
  Ipv6InterfaceContainer retval;
  Ptr<Node> node = device->GetNode();
  NS_ASSERT_MSG(node, "Ipv6AddressHelper::Allocate (): Bad node");
  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
  NS_ASSERT_MSG(ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;
  ifIndex = ipv6->GetInterfaceForDevice(device);
  if (ifIndex == -1)
  {
    ifIndex = ipv6->AddInterface(device);
  }
  NS_ASSERT_MSG(ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                              "Interface index not found");
  Ipv6InterfaceAddress ipv6Addr = Ipv6InterfaceAddress(addr, prefix);
  ipv6->SetMetric(ifIndex, 1);
  ipv6->SetUp(ifIndex);
  ipv6->AddAddress(ifIndex, ipv6Addr);
  retval.Add(ipv6, ifIndex);

  return retval;
}

Ipv6InterfaceContainer
AssignWithoutAddress(Ptr<NetDevice> device)
{
  Ipv6InterfaceContainer retval;
  Ptr<Node> node = device->GetNode();
  NS_ASSERT_MSG(node, "Ipv6AddressHelper::Allocate (): Bad node");
  Ptr<Ipv6> ipv6 = node->GetObject<Ipv6>();
  NS_ASSERT_MSG(ipv6, "Ipv6AddressHelper::Allocate (): Bad ipv6");
  int32_t ifIndex = 0;
  ifIndex = ipv6->GetInterfaceForDevice(device);
  if (ifIndex == -1)
  {
    ifIndex = ipv6->AddInterface(device);
  }
  NS_ASSERT_MSG(ifIndex >= 0, "Ipv6AddressHelper::Allocate (): "
                              "Interface index not found");
  ipv6->SetMetric(ifIndex, 1);
  ipv6->SetUp(ifIndex);
  retval.Add(ipv6, ifIndex);
  return retval;
}

static void
udpRx(std::string context, Ptr<const Packet> packet, const Address &address)
{
  SeqTsHeader seqTs;
  packet->Copy()->RemoveHeader(seqTs);
  NS_LOG_UNCOND(seqTs.GetTs() << "->" << Simulator::Now() << ": " << seqTs.GetSeq());
}

int main(int argc, char *argv[])
{
  uint16_t m_backbone = 8;
  uint8_t towers = m_backbone - 1;
  uint8_t mobile = 1;
  uint16_t port = 6000;
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 200;
  uint16_t simtime = 60;
  Time interPacketInterval = MilliSeconds(1000);

  NodeContainer sta,
      cn,
      backbone,
      aps,
      vehicles;

  //ref nodes
  NodeContainer lma,
      mags,
      outerNet,
      *magNet;

  NetDeviceContainer backboneDevs,
      outerDevs,
      *magDevs,
      *magApDev,
      *magBrDev,
      staDevs,
      vehiclesStaDev,
      vehiclesApDev;

  Ipv6InterfaceContainer backboneIfs,
      outerIfs,
      *magIfs,
      staIfs,
      iifc;

  ApplicationContainer *serverApps,
      *clientApps;

  CommandLine cmd;
  cmd.Parse(argc, argv);

  SeedManager::SetSeed(123456);

  //  LogLevel logAll = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
  //  LogLevel logLogic = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_LOGIC);
  //  LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);

  //  LogComponentEnable ("Udp6Server", logInfo);
  //  LogComponentEnable ("Pmipv6Agent", logAll);
  //  LogComponentEnable ("Pmipv6MagNotifier", logAll);

  backbone.Create(m_backbone);
  aps.Create(towers);
  cn.Create(1);
  sta.Create(mobile);
  vehicles.Create(5);

  magNet = new NodeContainer[towers];
  magDevs = new NetDeviceContainer[towers];
  magApDev = new NetDeviceContainer[towers];
  magBrDev = new NetDeviceContainer[towers];
  magIfs = new Ipv6InterfaceContainer[towers];

  InternetStackHelper internet;
  internet.Install(backbone);
  internet.Install(aps);
  internet.Install(cn);
  internet.Install(sta);
  internet.Install(vehicles);

  // Setting Up the connection for Sub vehicles
  YansWifiChannelHelper channelForVehicles = YansWifiChannelHelper::Default();
  YansWifiPhyHelper phyForvehicles = YansWifiPhyHelper::Default();
  phyForvehicles.SetChannel(channelForVehicles.Create());
  WifiHelper wifiForVehicles = WifiHelper::Default();
  wifiForVehicles.SetRemoteStationManager("ns3::AarfWifiManager");
  NqosWifiMacHelper macForVehicles = NqosWifiMacHelper::Default();
  Ssid ssid1 = Ssid("Vehicles");
  macForVehicles.SetType("ns3::StaWifiMac",
                         "Ssid", SsidValue(ssid1),
                         "ActiveProbing", BooleanValue(false));
  vehiclesStaDev = wifiForVehicles.Install(phyForvehicles, macForVehicles, vehicles);

  macForVehicles.SetType("ns3::ApWifiMac",
                         "Ssid", SsidValue(ssid1));
  vehiclesApDev = wifiForVehicles.Install(phyForvehicles, macForVehicles, sta);

  Ipv4AddressHelper address;
  address.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer vehiclesInterfaces;
  vehiclesInterfaces =
      address.Assign(vehiclesStaDev);
  address.Assign(vehiclesApDev);
  // Over Connection

  lma.Add(backbone.Get(0));
  for (int i = 1; i < m_backbone; ++i)
  {
    mags.Add(backbone.Get(i));
  }

  outerNet.Add(lma);
  outerNet.Add(cn);

  for (int i = 0; i < towers; ++i)
  {
    magNet[i].Add(mags.Get(i));
    magNet[i].Add(aps.Get(i));
  }

  CsmaHelper csma, csma1;

  //MAG's MAC Address (for unify default gateway of MN)
  Mac48Address magMacAddr("00:00:AA:BB:CC:DD");

  //Link between CN and LMA is 50Mbps and 0.1ms delay
  csma1.SetChannelAttribute("DataRate", DataRateValue(DataRate(50000000)));
  csma1.SetChannelAttribute("Delay", TimeValue(MicroSeconds(100)));
  csma1.SetDeviceAttribute("Mtu", UintegerValue(1400));

  outerDevs = csma1.Install(outerNet);
  iifc = AssignIpv6Address(outerDevs.Get(0), Ipv6Address("3ffe:2::1"), 64);
  outerIfs.Add(iifc);
  iifc = AssignIpv6Address(outerDevs.Get(1), Ipv6Address("3ffe:2::2"), 64);
  outerIfs.Add(iifc);
  outerIfs.SetForwarding(0, true);
  outerIfs.SetDefaultRouteInAllNodes(0);

  //All Link is 50Mbps and 0.1ms delay
  csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(50000000)));
  csma.SetChannelAttribute("Delay", TimeValue(MicroSeconds(100)));
  csma.SetDeviceAttribute("Mtu", UintegerValue(1400));

  backboneDevs = csma.Install(backbone);

  for (uint16_t i = 0; i < m_backbone; i++)
  {
    string ipaddress;
    stringstream ss;
    ss << i;
    string str = ss.str();
    ipaddress = "3ffe:1::" + str;
    iifc = AssignIpv6Address(backboneDevs.Get(i), Ipv6Address(ipaddress.c_str()), 64);
    backboneIfs.Add(iifc);
  }
  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);

  BridgeHelper bridge;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc;

  positionAlloc = CreateObject<ListPositionAllocator>();
  //LMA
  positionAlloc->Add(Vector(200.0, 00.0, 0.0));
  // Position for MAG
  PositionForMAG(positionAlloc);
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(backbone);

  positionAlloc = CreateObject<ListPositionAllocator>();
  // position for CN
  positionAlloc->Add(Vector(0.0, 0.0, 0.0));
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(cn);

  // PositionFor MAGAP
  positionAlloc = CreateObject<ListPositionAllocator>();
  PositionForMAGAP(positionAlloc);
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(aps);

  // Setting Wlan ap
  Ssid ssid = Ssid("MAG");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();
  wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  WifiHelper wifi = WifiHelper::Default();
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
  wifiPhy.SetChannel(wifiChannel.Create());

  wifiMac.SetType("ns3::ApWifiMac",
                  "Ssid", SsidValue(ssid),
                  "BeaconGeneration", BooleanValue(true),
                  "BeaconInterval", TimeValue(MicroSeconds(102400)));

  //Setting MAG1
  for (int i = 0; i < towers; i++)
  {
    magDevs[i] = csma.Install(magNet[i]);
    magDevs[i].Get(0)->SetAddress(magMacAddr);
    string head = "3ffe:1:",
           tail = "::1",
           out;
    stringstream ss;
    ss << i + 1;
    string str = ss.str();
    out = head + str + tail;
    magIfs[i] = AssignIpv6Address(magDevs[i].Get(0), Ipv6Address(out.c_str()), 64);

    magApDev[i] = wifi.Install(wifiPhy, wifiMac, magNet[i].Get(1));
    magBrDev[i] = bridge.Install(aps.Get(i), NetDeviceContainer(magApDev[i], magDevs[i].Get(1)));
    iifc = AssignWithoutAddress(magDevs[i].Get(1));
    magIfs[i].Add(iifc);
    magIfs[i].SetForwarding(0, true);
    magIfs[i].SetDefaultRouteInAllNodes(0);
  }

  //setting station
  positionAlloc = CreateObject<ListPositionAllocator>();

  for (uint i = 0; i < sta.GetN(); ++i)
  {
    positionAlloc->Add(Vector(i * 25, 60.0, 0.0));
  } //STA
  for (uint i = 0; i < vehicles.GetN(); ++i)
  {
    positionAlloc->Add(Vector(i * 25, 80.0, 0.0));
  } //vehicles
  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
  mobility.Install(sta);
  mobility.Install(vehicles);

  for (uint i = 0; i < sta.GetN(); ++i)
  {
    Ptr<ConstantVelocityMobilityModel> cvm = sta.Get(i)->GetObject<ConstantVelocityMobilityModel>();
    cvm->SetVelocity(Vector(10.0, 0, 0)); //move to left to right 10.0m/s
  }

  for (uint i = 0; i < vehicles.GetN(); ++i)
  {
    Ptr<ConstantVelocityMobilityModel> cvm1 = vehicles.Get(i)->GetObject<ConstantVelocityMobilityModel>();
    cvm1->SetVelocity(Vector(10.0, 0, 0)); //move to left to right 10.0m/s
  }

  //WLAN interface
  wifiMac.SetType("ns3::StaWifiMac",
                  "Ssid", SsidValue(ssid),
                  "ActiveProbing", BooleanValue(false));
  staDevs.Add(wifi.Install(wifiPhy, wifiMac, sta));
  Ptr<Pmipv6ProfileHelper> profile = Create<Pmipv6ProfileHelper>();
  for (int i = 0; i < mobile; i++)
  {
    iifc = AssignWithoutAddress(staDevs.Get(i));
    staIfs.Add(iifc);
  }

  for (uint i = 0; i < staDevs.GetN(); i++)
  {
    //attach PMIPv6 agents
    //adding profile for each station
    string head = "pmip",
           tail = "@example.com",
           out;
    stringstream ss;
    ss << i;
    string str = ss.str();
    out = head + str + tail;
    profile->AddProfile(Identifier(out.c_str()),
                        Identifier(Mac48Address::ConvertFrom(staDevs.Get(i)->GetAddress())),
                        backboneIfs.GetAddress(0, 1), std::list<Ipv6Address>());
  }

  Pmipv6LmaHelper lmahelper;
  lmahelper.SetPrefixPoolBase(Ipv6Address("3ffe:1:4::"), 48);
  lmahelper.SetProfileHelper(profile);

  lmahelper.Install(lma.Get(0));

  Pmipv6MagHelper maghelper;

  maghelper.SetProfileHelper(profile);

  for (int i = 0; i < towers; i++)
  {
    maghelper.Install(mags.Get(i), magIfs[i].GetAddress(0, 0), aps.Get(i));
  }

  // AsciiTraceHelper ascii;
  // csma.EnableAsciiAll (ascii.CreateFileStream ("pmip6-wifi.tr"));
  // csma.EnablePcapAll (std::string ("pmip6-wifi"), false);

  // wifiPhy.EnablePcap ("pmip6-wifi", mag1ApDev.Get(0));
  // wifiPhy.EnablePcap ("pmip6-wifi", mag2ApDev.Get(0));
  // wifiPhy.EnablePcap ("pmip6-wifi", staDevs.Get(0));

  serverApps = new ApplicationContainer[mobile];
  clientApps = new ApplicationContainer[mobile];
  for (int i = 0; i < mobile; i++)
  {
    NS_LOG_INFO("Installing UDP server on MN");
    PacketSinkHelper sink("ns3::UdpSocketFactory",
                          Inet6SocketAddress(Ipv6Address::GetAny(), port));
    serverApps[i] = sink.Install(sta.Get(i));

    NS_LOG_INFO("Installing UDP client on CN");

    UdpClientHelper udpClient(Ipv6Address("3ffe:1:4:1:200:ff:fe00:c"), port);
    udpClient.SetAttribute("Interval", TimeValue(interPacketInterval));
    udpClient.SetAttribute("PacketSize", UintegerValue(packetSize));
    udpClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    clientApps[i] = udpClient.Install(cn.Get(0));
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx", MakeCallback(&udpRx));

    serverApps[i].Start(Seconds(1.0));
    clientApps[i].Start(Seconds(1.5));
    serverApps[i].Stop(Seconds(simtime));
    clientApps[i].Stop(Seconds(simtime));
  }

  AnimationInterface anim("PMIPv6.xml");

  Simulator::Stop(Seconds(simtime));
  Simulator::Run();
  Simulator::Destroy();

  delete[] magNet;
  delete[] magDevs;
  delete[] magApDev;
  delete[] magBrDev;
  delete[] magIfs;
  delete[] serverApps;
  delete[] clientApps;
  return 0;
}
