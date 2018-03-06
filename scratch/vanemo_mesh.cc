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
#include "ns3/mesh-helper.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("Pmipv6Wifi");

using namespace ns3;


class MeshTest
{
public:
  /// Init test
  MeshTest ();
  /// Configure test from command line arguments
  void Configure (int argc, char ** argv);
  /// Run test
  void Init (NodeContainer &nc);
private:
  int       m_xSize;
  int       m_ySize;
  double    m_step;
  double    m_randomStart;
  double    m_totalTime;
  double    m_packetInterval;
  uint16_t  m_packetSize;
  uint32_t  m_nIfaces;
  bool      m_chan;
  bool      m_pcap;
  std::string m_stack;
  std::string m_root;
  /// List of network nodes
  NodeContainer *m_nodes;
  /// List of all mesh point devices
  NetDeviceContainer meshDevices;
  //Addresses of interfaces:
  Ipv4InterfaceContainer interfaces;
  // MeshHelper. Report is not static methods
  MeshHelper mesh;
private:
  /// Create nodes and setup their mobility
  void CreateNodes ();
  /// Install Mesh to target group.
  void InstallMesh ();
  void InstallMobility ();
  /// Install internet m_stack on nodes
  void InstallInternetStack ();
  /// Install applications
  void InstallApplication ();
  /// Print mesh devices diagnostics
  void Report ();
};
MeshTest::MeshTest () :
  m_xSize (3),
  m_ySize (3),
  m_step (100.0),
  m_randomStart (0.1),
  m_totalTime (100.0),
  m_packetInterval (0.1),
  m_packetSize (1024),
  m_nIfaces (1),
  m_chan (true),
  m_pcap (true),
  m_stack ("ns3::Dot11sStack"),
  m_root ("ff:ff:ff:ff:ff:ff")
{
}
void
MeshTest::Configure (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("x-size", "Number of nodes in a row grid. [6]", m_xSize);
  cmd.AddValue ("y-size", "Number of rows in a grid. [6]", m_ySize);
  cmd.AddValue ("step",   "Size of edge in our grid, meters. [100 m]", m_step);
  /*
   * As soon as starting node means that it sends a beacon,
   * simultaneous start is not good.
   */
  cmd.AddValue ("start",  "Maximum random start delay, seconds. [0.1 s]", m_randomStart);
  cmd.AddValue ("time",  "Simulation time, seconds [100 s]", m_totalTime);
  cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping, seconds [0.001 s]", m_packetInterval);
  cmd.AddValue ("packet-size",  "Size of packets in UDP ping", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point. [1]", m_nIfaces);
  cmd.AddValue ("channels",   "Use different frequency channels for different interfaces. [0]", m_chan);
  cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces. [0]", m_pcap);
  cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
  cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);

  cmd.Parse (argc, argv);
  NS_LOG_DEBUG ("Grid:" << m_xSize << "*" << m_ySize);
  NS_LOG_DEBUG ("Simulation time: " << m_totalTime << " s");
}
void
MeshTest::CreateNodes ()
{
  /*
   * Create m_ySize*m_xSize stations to form a grid topology
   */
  m_nodes->Create (m_ySize*m_xSize);
}

void
MeshTest::InstallMesh ()
{
  // Configure YansWifiChannel
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  /*
   * Create mesh helper and set stack installer to it
   * Stack installer creates all needed protocols and install them to
   * mesh point device
   */
  mesh = MeshHelper::Default ();
  if (!Mac48Address (m_root.c_str ()).IsBroadcast ())
  {
	  mesh.SetStackInstaller (m_stack, "Root", Mac48AddressValue (Mac48Address (m_root.c_str ())));
	  NS_LOG_UNCOND("000000000000000000 THIS MESH IS A BROADCAST 00000000000000000");
  }
  else
  {
      //If root is not set, we do not use "Root" attribute, because it
      //is specified only for 11s
      mesh.SetStackInstaller (m_stack);
  }
  if (m_chan)
  {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
  }
  else
  {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
  }
  mesh.SetMacType ("RandomStart", TimeValue (Seconds (m_randomStart)));
  // Set number of interfaces - default is single-interface mesh point
  mesh.SetNumberOfInterfaces (m_nIfaces);
  // Install protocols and return container if MeshPointDevices
  meshDevices = mesh.Install (wifiPhy, *m_nodes);
  if (m_pcap)
    wifiPhy.EnablePcap (std::string ("mp-"), meshDevices);
}

void MeshTest::InstallMobility ()//todo: remove if unused
{
  // Setup mobility - static grid topology

  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (m_step),
                                 "DeltaY", DoubleValue (m_step),
                                 "GridWidth", UintegerValue (m_xSize),
                                 "LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (*m_nodes);
}

void MeshTest::InstallInternetStack ()
{
  InternetStackHelper internetStack;
  internetStack.Install (*m_nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces = address.Assign (meshDevices);
}
void
MeshTest::InstallApplication ()
{
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (m_nodes->Get (0));
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));
  UdpEchoClientHelper echoClient (interfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(m_totalTime*(1/m_packetInterval))));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  ApplicationContainer clientApps = echoClient.Install (m_nodes->Get (m_nodes->GetN() - 1));
  clientApps.Start (Seconds (0.0));
  clientApps.Stop (Seconds (m_totalTime));
}
void MeshTest::Init (NodeContainer &nc)
{
//  CreateNodes ();
	m_nodes = &nc;
  InstallMesh ();
//  InstallInternetStack ();
//  InstallApplication ();
//  Simulator::Schedule (Seconds (m_totalTime), &MeshTest::Report, this);
//  Simulator::Stop (Seconds (m_totalTime));
//  Simulator::Run ();
//  Simulator::Destroy ();
}

void MeshTest::Report ()
{
  unsigned n (0);
  for (NetDeviceContainer::Iterator i = meshDevices.Begin (); i != meshDevices.End (); ++i, ++n)
    {
      std::ostringstream os;
      os << "mp-report-" << n << ".xml";
      std::cerr << "Printing mesh point device #" << n << " diagnostics to " << os.str () << "\n";
      std::ofstream of;
      of.open (os.str ().c_str ());
      if (!of.is_open ())
        {
          std::cerr << "Error: Can't open file " << os.str () << "\n";
          return;
        }
      mesh.Report (*i, of);
      of.close ();
    }
}

Ipv6InterfaceContainer AssignSingleIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix)
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

Ipv6InterfaceContainer AssignIpv6Addresses(NetDeviceContainer devices, Ipv6Address network, Ipv6Prefix prefix)
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
	std::vector<NodeContainer> magApNodes;
	//MAC Address for MAGs
	std::vector<Mac48Address> magMacAddrs;

	NetDeviceContainer lmaMagCsmaDevs;
	NetDeviceContainer lmaCnDevs;
	std::vector<NetDeviceContainer> magApCsmaDevs;
	std::vector<NetDeviceContainer> apWifiDevs;
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
	MeshTest mesh;
}
using namespace containers;

void doMesh()
{
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogLevel logFunc = static_cast<LogLevel>(LOG_LEVEL_INFO|LOG_LEVEL_FUNCTION);
    LogComponentEnable ("UdpEchoServerApplication", logFunc);
    mesh.Init (mnns);
}

void printMnnsDeviceInfor(const std::string &preface)
{
	NS_LOG_UNCOND("==================\nPrint " << preface << ":");
	for (size_t i = 0; i < mnns.GetN (); i++)
		if (mnns.Get(i)->GetObject<Ipv4> ())
			NS_LOG_UNCOND("MNN" << i << " Devices:" << mnns.Get(i)->GetNDevices() << " Interfaces:" << mnns.Get(i)->GetObject<Ipv4> ()->GetNInterfaces());
		else
			NS_LOG_UNCOND("MNN" << i << " Devices:" << mnns.Get(i)->GetNDevices());
	NS_LOG_UNCOND("==================");
}

const int backBoneCnt = 4;
const double startTime = 0.0;
const double endTime   = 30.0;
Ipv6Address destAddress;
Ptr<Node> destNode;

void createNodes()
{
	  lmaMagNodes.Create(backBoneCnt + 1);
	  aps.Create(backBoneCnt);
	  cn.Create(1);
	  mnns.Create(3);
	  leader.Add(mnns.Get(0));
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
		  magApNodes.push_back(magApPair);
	  }
}

void installInternetStack()
{
	  InternetStackHelper internet;
	  internet.Install (lmaMagNodes);
	  internet.Install (aps);
	  internet.Install (cn);
	  internet.Install (leader);
	  internet.Install (followers);
}

void assignMagMacAddresses()
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

void initCsma(CsmaHelper &csma, uint64_t dataRateBps = 50000000, int64_t delay = 100, uint64_t mtu = 1400)
{
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
}

Ptr<Pmipv6ProfileHelper> enableLMAProfiling()
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

void initMnnMobility()
{
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (-50.0, 50.0, 0.0)); //GL
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
	mobility.Install(leader);
	positionAlloc = CreateObject<ListPositionAllocator> ();

	for(unsigned int i = 0; i < followers.GetN(); i++)
	{
		  positionAlloc->Add (Vector (-20.0, (i*10) + 20.0, 0.0)); //MNNs
	}
	mobility.SetPositionAllocator (positionAlloc);
	mobility.PushReferenceMobilityModel(leader.Get (0));
	mobility.Install(followers);
}

void initVelocitySensor(double interval)
{
	//Server Application
	ApplicationContainer velocitySensor;

	VelocitySensorHelper vs(Seconds (interval));
	//do settings
	velocitySensor = vs.Install(NodeContainer(leader, followers));
	velocitySensor.Start (Seconds (startTime));
	velocitySensor.Stop (Seconds (endTime));
}

void initGrpFinder(NetDeviceContainer &devs)
{
	//Server Application
	ApplicationContainer grpFinder;

	GroupFinderHelper::SetEnable(true);
	GroupFinderHelper gf;
	//do settings
	grpFinder = gf.Install(mnns, devs);
	grpFinder.Start (Seconds (startTime));
	grpFinder.Stop (Seconds (endTime));
}

void initUdpApp()
{
	  uint16_t port = 6000;
	  ApplicationContainer serverApps, clientApps;
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
}

void initAnim(AnimationInterface &anim)
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

void initPmip()
{
	Ptr<Pmipv6ProfileHelper> profile = enableLMAProfiling();
//	enableLMAProfiling();

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

int main (int argc, char *argv[])
{
  (void) startTime; (void)endTime;

  CommandLine cmd;
  cmd.Parse (argc, argv);
  logSettings();
  SeedManager::SetSeed (123456);

  createNodes();

  printMnnsDeviceInfor("After node creation");
  installInternetStack();
  printMnnsDeviceInfor("After Installing Internet stack");
  NS_LOG_UNCOND("MAC Addresses:");
  assignMagMacAddresses();
  printMnnsDeviceInfor("After MAC Address");
  NS_LOG_UNCOND("Outer Network:");
  CsmaHelper csmaLmaCn;
  initCsma(csmaLmaCn);
  printMnnsDeviceInfor("After initCsma csmaLmaCn");
  Ipv6InterfaceContainer iifc;

  //Outer Dev CSMA and Addressing
  //Link between CN and LMA is 50Mbps and 0.1ms delay
  lmaCnDevs = csmaLmaCn.Install(lmaCnNodes);
  iifc = AssignSingleIpv6Address(lmaCnDevs.Get(0), Ipv6Address("3ffe:2::1"), 64);
  outerIfs.Add(iifc);
  iifc = AssignSingleIpv6Address(lmaCnDevs.Get(1), Ipv6Address("3ffe:2::2"), 64);
  outerIfs.Add(iifc);
  outerIfs.SetForwarding(0, true);
  outerIfs.SetDefaultRouteInAllNodes(0);
  printMnnsDeviceInfor("After lmaCnDevs");
  NS_LOG_UNCOND("LMA MAG Network:");
  CsmaHelper csmaLmaMag;
  initCsma(csmaLmaMag);
  printMnnsDeviceInfor("After initCsma initCsma");
  //All Link is 50Mbps and 0.1ms delay
  //Backbone Addressing
  NS_LOG_UNCOND("Assign lmaMagNodes Addresses");
  lmaMagCsmaDevs = csmaLmaMag.Install(lmaMagNodes);
  for (int i = 0; i <= backBoneCnt; i++)
  {
	  std::ostringstream out("");
	  out << "3ffe:1::";
	  out << i+1;
	  iifc = AssignSingleIpv6Address(lmaMagCsmaDevs.Get(i), Ipv6Address(out.str().c_str()), 64);
	  backboneIfs.Add(iifc);
	  out.str("");
  }
  backboneIfs.SetForwarding(0, true);
  backboneIfs.SetDefaultRouteInAllNodes(0);
  printMnnsDeviceInfor("After Assign lmaMagNodes Addresses");

  //Backbone Mobility
  Ptr<ListPositionAllocator> positionAlloc;
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, -20.0, 0.0));   //LMA
  for (int i = 0; i < backBoneCnt; i++)
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
  for (unsigned i = 0; i < backBoneCnt; i++)
  {
	  positionAlloc->Add (Vector (-50.0 + (i* 100), 40.0, 0.0)); //MAGAPi
  }
  installConstantMobility(aps, positionAlloc);

  printMnnsDeviceInfor("Before ANY Wifi Installation");
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
	  magApCsmaDevs.push_back(csmaLmaMag.Install(magApNodes[i]));
	  magApCsmaDevs[i].Get(0)->SetAddress(magMacAddrs[i]);
	  std::ostringstream out("");
	  out << "3ffe:1:" << i+1 << "::1";
	  magIfs.push_back(AssignSingleIpv6Address(magApCsmaDevs[i].Get(0), Ipv6Address(out.str().c_str()), 64));
	  out.str("");
	  apWifiDevs.push_back(wifi.Install (wifiPhy, wifiMac, magApNodes[i].Get(1)));
	  magBrDevs.push_back(bridge.Install (aps.Get(i), NetDeviceContainer(apWifiDevs[i], magApCsmaDevs[i].Get(1))));
	  iifc = AssignWithoutAddress(magApCsmaDevs[i].Get(1));
	  magIfs[i].Add(iifc);
	  magIfs[i].SetForwarding(0, true);
	  magIfs[i].SetDefaultRouteInAllNodes(0);
  }

  std::ostringstream magOut(""), apOut("");
  for (int i = 0; i < backBoneCnt; i++)
  {
	  magOut << "MAG" << i << " Addresses: " << magIfs[i].GetAddress(0,0) << " and " << magIfs[i].GetAddress(0,1) << "\n";
	  apOut << "AP" << i << " Mac Addresses: " << magApCsmaDevs[i].Get(1)->GetAddress() << "\n";
  }
  NS_LOG_UNCOND(magOut.str() << apOut.str());

  initMnnMobility();

  printMnnsDeviceInfor("Before MNNs Wifi Installation");
  NS_LOG_UNCOND("Create EXTERNAL networks and assign MNN Addresses.");

  //GL movement
  Ptr<ConstantVelocityMobilityModel> cvm = leader.Get(0)->GetObject<ConstantVelocityMobilityModel>();
  cvm->SetVelocity(Vector (10.0, 0, 0)); //move to left to right 10.0m/s
  //GL Wifi
  wifiMac.SetType ("ns3::StaWifiMac",
	               "Ssid", SsidValue (ssid),
	               "ActiveProbing", BooleanValue (false));
  mnnsExtDevs = wifi.Install (wifiPhy, wifiMac, mnns);
  iifc = AssignIpv6Addresses(mnnsExtDevs, Ipv6Address("3ffe:6:6:1::"), 64);
  iifc.SetForwarding (0, true);
  iifc.SetDefaultRouteInAllNodes (0);
  printMnnsDeviceInfor("After EXTERNAL MNNs StaWifi Installation");

  leaderDev.Add(mnnsExtDevs.Get(0));
  for (size_t i = 1; i < mnnsExtDevs.GetN(); i++)
	  followerDevs.Add(mnnsExtDevs.Get(i));

  NS_LOG_UNCOND("Create INTERNAL networks and assign MNN Addresses.");
  //GL Wifi
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiMac.SetType ("ns3::AdhocWifiMac");
  mnnsIntDevs = wifi.Install (wifiPhy, wifiMac, mnns);
  iifc = AssignIpv6Addresses(mnnsIntDevs, Ipv6Address("3ffe:6:6:2::"), 64);
  iifc.SetForwarding (0, true);
  printMnnsDeviceInfor("After INTERNAL MNNs StaWifi Installation");

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
  
  initPmip();
  printMnnsDeviceInfor("After PMIP init");

  NS_LOG_UNCOND("Mesh Setup");
  doMesh();

  NS_LOG_UNCOND("Routes Setup");

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

  csmaLmaMag.EnablePcap(std::string ("csma-lma-mag"), lmaMagCsmaDevs, false);
  csmaLmaMag.EnablePcap(std::string ("csma-lma-cn"), lmaCnDevs, false);

  for (int i = 0; i < backBoneCnt; i++)
  {
	  wifiPhy.EnablePcap ("wifi-ap", apWifiDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-mag->ap", magApCsmaDevs[i].Get(0));
	  csmaLmaMag.EnablePcap("csma-ap", magApCsmaDevs[i].Get(1));
  }

  wifiPhy.EnablePcap ("wifi-ext-mnns", mnnsExtDevs);
  wifiPhy.EnablePcap ("wifi-int-mnns", mnnsIntDevs);

  NS_LOG_UNCOND("Installing SENSOR");

  initVelocitySensor(1.0);

  NS_LOG_UNCOND("Installing GRP FINDER");
  initGrpFinder(mnnsExtDevs);

  //Application
  NS_LOG_UNCOND("Installing UDP server on MN");
  initUdpApp();

  NS_LOG_UNCOND("Animator Settings");
  AnimationInterface anim("PMIPv6.xml");
  initAnim(anim);

  printMnnsDeviceInfor("END");

  //Run
  NS_LOG_UNCOND("Run");
  Simulator::Stop (Seconds (endTime));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
