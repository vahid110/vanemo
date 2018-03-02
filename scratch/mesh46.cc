/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008,2009 IITP RAS
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
 * Author: Kirill Andreev <andreev@iitp.ru>
 *
 *
 * By default this script creates m_xSize * m_ySize square grid topology with
 * IEEE802.11s stack installed at each node with peering management
 * and HWMP protocol.
 * The side of the square cell is defined by m_step parameter.
 * When topology is created, UDP ping is installed to opposite corners
 * by diagonals. packet size of the UDP ping and interval between two
 * successive packets is configurable.
 * 
 *  m_xSize * step
 *  |<--------->|
 *   step
 *  |<--->|
 *  * --- * --- * <---Ping sink  _
 *  | \   |   / |                ^
 *  |   \ | /   |                |
 *  * --- * --- * m_ySize * step |
 *  |   / | \   |                |
 *  | /   |   \ |                |
 *  * --- * --- *                _
 *  ^ Ping source
 *
 *  See also MeshTest::Configure to read more about configurable
 *  parameters.
 */


#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mesh-module.h"
#include "ns3/mobility-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/csma-module.h"


#include <iostream>
#include <sstream>
#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestMeshScript");

/**
 * \param device a pointer to the net device which is calling this callback
 * \param packet the packet received
 * \param protocol the 16 bit protocol number associated with this packet.
 *        This protocol number is expected to be the same protocol number
 *        given to the Send method by the user on the sender side.
 * \param sender the address of the sender
 * \returns true if the callback could handle the packet successfully, false
 *          otherwise.
 */

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


/**
 * \ingroup mesh
 * \brief MeshTest class
 */
class MeshTest
{
public:
  /// Init test
  MeshTest ();
  /**
   * Configure test from command line arguments
   *
   * \param argc command line argument count
   * \param argv command line arguments
   */
  void Configure (int argc, char ** argv);
  /**
   * Run test
   * \returns the test status
   */
  int Run ();
private:
  int       m_xSize; ///< X size
  int       m_ySize; ///< Y size
  double    m_step; ///< step
  double    m_randomStart; ///< random start
  double    m_totalTime; ///< total time
  double    m_packetInterval; ///< packet interval
  uint16_t  m_packetSize; ///< packet size
  uint32_t  m_nIfaces; ///< number interfaces
  bool      m_chan; ///< channel
  bool      m_pcap; ///< PCAP
  std::string m_stack; ///< stack
  std::string m_root; ///< root
  /// List of network nodes
  NodeContainer nodes;
  NodeContainer externals;
  NodeContainer bridge;
  Ptr<Node> cn, mag, src, dst;
  Ipv6Address destAddress;
  /// List of all mesh point devices
  NetDeviceContainer meshDevices;
  /// Addresses of interfaces:
  Ipv4InterfaceContainer interfaces;
  /// MeshHelper. Report is not static methods
  MeshHelper mesh;
private:
  /// Create nodes and setup their mobility
  void CreateNodes ();
  /// Install internet m_stack on nodes
  void InstallInternetStack ();
  /// Install applications
  void InstallApplication ();
  /// Print mesh devices diagnostics
  void InstallExternal();
  void Report ();

  bool CsmaDeviceRecv(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t m, const Address & from)
  {
  	NS_LOG_UNCOND(p->GetUid() << " PACKET received by CSMA NODE:" << device->GetNode()->GetId());
  	return true;
  }

  bool WifiDeviceRecv(Ptr<NetDevice> device, Ptr<const Packet> p, uint16_t m, const Address & from)
  {
  	NS_LOG_UNCOND(p->GetUid() << " PACKET received by Wifi NODE:" << device->GetNode()->GetId());
  	return true;
  }
  void EchoTx(Ptr< const Packet > p)
  {
      NS_LOG_UNCOND(p->GetUid() << " PACKET Echo Sent.");
  }
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
  m_pcap (false),
  m_stack ("ns3::Dot11sStack"),
  m_root ("ff:ff:ff:ff:ff:ff")
{
}
void
MeshTest::Configure (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.AddValue ("x-size", "Number of nodes in a row grid", m_xSize);
  cmd.AddValue ("y-size", "Number of rows in a grid", m_ySize);
  cmd.AddValue ("step",   "Size of edge in our grid (meters)", m_step);
  // Avoid starting all mesh nodes at the same time (beacons may collide)
  cmd.AddValue ("start",  "Maximum random start delay for beacon jitter (sec)", m_randomStart);
  cmd.AddValue ("time",  "Simulation time (sec)", m_totalTime);
  cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping (sec)", m_packetInterval);
  cmd.AddValue ("packet-size",  "Size of packets in UDP ping (bytes)", m_packetSize);
  cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point", m_nIfaces);
  cmd.AddValue ("channels",   "Use different frequency channels for different interfaces", m_chan);
  cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces", m_pcap);
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
  nodes.Create (m_ySize*m_xSize);
  externals.Create(2);
  cn = externals.Get(0);
  mag = externals.Get(1);
  dst = nodes.Get(nodes.GetN() - 1);
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
  meshDevices = mesh.Install (wifiPhy, nodes);
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
  mobility.Install (nodes);
  if (m_pcap)
    wifiPhy.EnablePcapAll (std::string ("mp-"));
}
void MeshTest::InstallInternetStack ()
{
  InternetStackHelper internetStack;
  internetStack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  interfaces = address.Assign (meshDevices);
}

Ipv6Address GetNodeAddress(Ptr<Node> node, uint32_t device_id, uint32_t addr_index)
{
	Ptr<Ipv6> ipv6 = node->GetObject<Ipv6> ();
	int32_t next = ipv6->GetInterfaceForDevice(node->GetDevice(device_id));
	return ipv6->GetAddress(next, addr_index).GetAddress();
}
void MeshTest::InstallExternal()
{
	InternetStackHelper internet;
	internet.Install(externals);

	//CSMA
	CsmaHelper csma;
    csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
    csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
    csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));
    NetDeviceContainer externalsDevs = csma.Install(externals);
    NS_LOG_UNCOND("[MESH-TEST]  CN<->MAG address:");
    Ipv6InterfaceContainer iifc = AssignIpv6Addresses(externalsDevs, Ipv6Address("3ffe:6:6:6::"), 64);
    iifc.SetForwarding (0, true);
    iifc.SetDefaultRouteInAllNodes (0);
    externalsDevs.Get(0)->SetReceiveCallback (MakeCallback (&MeshTest::CsmaDeviceRecv, this));
    externalsDevs.Get(1)->SetReceiveCallback (MakeCallback (&MeshTest::CsmaDeviceRecv, this));
    bridge.Add(mag);
    bridge.Add (dst);
    //Wifi
    Ssid ssid = Ssid("MAG");
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
    WifiHelper wifi = WifiHelper::Default ();
    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (28));
    wifiPhy.SetChannel (wifiChannel.Create ());
    //WiFi MAG
    wifiMac.SetType ("ns3::ApWifiMac",
  		           "Ssid", SsidValue (ssid),
  		           "BeaconGeneration", BooleanValue (true),
  		           "BeaconInterval", TimeValue (MicroSeconds (102400)));
    NetDeviceContainer WifiDevs = wifi.Install (wifiPhy, wifiMac, mag);
    //WiFi DST
    wifiMac.SetType ("ns3::StaWifiMac",
  	               "Ssid", SsidValue (ssid),
  	               "ActiveProbing", BooleanValue (false));
    WifiDevs.Add(wifi.Install (wifiPhy, wifiMac, dst));
    WifiDevs.Get(0)->SetReceiveCallback (MakeCallback (&MeshTest::WifiDeviceRecv, this));
    WifiDevs.Get(1)->SetReceiveCallback (MakeCallback (&MeshTest::WifiDeviceRecv, this));
    // Wifi Bridge Address
    iifc = AssignIpv6Addresses(WifiDevs, Ipv6Address("3ffe:5:5:5::"), 64);
    iifc.SetForwarding (0, true);
    iifc.SetDefaultRouteInAllNodes (0);
    //MAG Mobility
	MobilityHelper mobility;
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
	                                 "MinX", DoubleValue (-10.0),
	                                 "MinY", DoubleValue (-10.0),
	                                 "DeltaX", DoubleValue (2),
	                                 "DeltaY", DoubleValue (2),
	                                 "GridWidth", UintegerValue (2),
	                                 "LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (externals);

    destAddress = GetNodeAddress(dst, dst->GetNDevices() - 1, 1);

    //Routing, CSMA
    NS_LOG_UNCOND("[MESH-TEST]  CN(" << cn->GetId() << ") address:" << GetNodeAddress(cn, cn->GetNDevices() - 1, 1));
    NS_LOG_UNCOND("[MESH-TEST]  MAG(" << mag->GetId() << ") address:" << GetNodeAddress(mag, mag->GetNDevices() - 1, 1));
    NS_LOG_UNCOND("[MESH-TEST]  DST(" << dst->GetId() << ") address:" << destAddress);
    Ipv6StaticRoutingHelper ipv6RoutingHelper;
    Ptr<Ipv6StaticRouting> sr_cn;
    sr_cn = ipv6RoutingHelper.GetStaticRouting (cn->GetObject<Ipv6> ());
    NS_LOG_UNCOND("[MESH-TEST]  CN Ipv6StaticRouting:" << GetPointer(sr_cn));
    NS_LOG_UNCOND("[MESH-TEST]  CN Adding HostRouteTo(" << destAddress << "," << GetNodeAddress(mag, 1, 1) << ",1)" << cn->GetNDevices());
	sr_cn->AddHostRouteTo (destAddress, GetNodeAddress(mag, 1, 1), 1);
	//Routing, Wifi
    Ptr<Ipv6StaticRouting> sr_mag;
    sr_mag = ipv6RoutingHelper.GetStaticRouting (mag->GetObject<Ipv6> ());
    NS_LOG_UNCOND("[MESH-TEST]  MAG Ipv6StaticRouting:" << GetPointer(sr_mag));
    NS_LOG_UNCOND("[MESH-TEST]  MAG Adding HostRouteTo(" << GetNodeAddress(dst, dst->GetNDevices() - 1, 1) << ", 2) " << mag->GetNDevices());
	sr_mag->AddHostRouteTo (destAddress,  GetNodeAddress(dst, dst->GetNDevices() - 1, 1), 2);
}

void MeshTest::InstallApplication ()
{
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (dst);
  serverApps.Start (Seconds (0.0));
  serverApps.Stop (Seconds (m_totalTime));

  UdpEchoClientHelper echoClient (destAddress, 9);
//  echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(m_totalTime*(1/m_packetInterval))));
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));

  echoClient.SetAttribute ("Interval", TimeValue (Seconds (m_packetInterval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (m_packetSize));
  ApplicationContainer clientApps = echoClient.Install (cn);
  clientApps.Start (Seconds (0.2));
  clientApps.Stop (Seconds (m_totalTime));
}

int MeshTest::Run ()
{
	NS_LOG_UNCOND("[MESH-TEST]  Run");
  CreateNodes ();
	NS_LOG_UNCOND("[MESH-TEST]  CreateNodes");
  InstallInternetStack ();
	NS_LOG_UNCOND("[MESH-TEST]  InstallInternetStack");
  InstallExternal();
	NS_LOG_UNCOND("[MESH-TEST]  InstallExternal");
  InstallApplication ();
	NS_LOG_UNCOND("[MESH-TEST]  InstallApplication");
//	Config::Connect("/NodeList/9/ApplicationList/*/$ns3::UdpEchoClient/Tx", MakeCallback(&MeshTest::EchoTx, this));
//  Simulator::Schedule (Seconds (m_totalTime), &MeshTest::Report, this);
  Simulator::Stop (Seconds (m_totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
void
MeshTest::Report ()
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
int
main (int argc, char *argv[])
{
  
    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
//    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_FUNCTION);
//    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_FUNCTION);
//    LogComponentEnable ("Ipv6StaticRouting", LOG_LEVEL_ALL);
    LogComponentEnable ("Ipv6L3Protocol", LOG_LEVEL_ALL);
  MeshTest t; 
  t.Configure (argc, argv);
  return t.Run ();
}
