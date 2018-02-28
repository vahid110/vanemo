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
#include "ns3/mesh-module.h"
#include "ns3/mesh-helper.h"

#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-static-source-routing.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/netanim-module.h"
#include "ns3/group-finder-helper.h"
#include "ns3/velocity-sensor-helper.h"
#include "ns3/mesh-helper.h"
#include "ns3/ns2-mobility-helper.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>

NS_LOG_COMPONENT_DEFINE ("Pmipv6Wifi");

using namespace ns3;
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

    NetDeviceContainer mnnsNormalDevs;
    NetDeviceContainer leaderDev;
    NetDeviceContainer followerDevs;

    Ipv6InterfaceContainer backboneIfs;
    Ipv6InterfaceContainer outerIfs;

    std::vector<Ipv6InterfaceContainer> magIfs;

    Ipv6InterfaceContainer glIfs;
    Ipv6InterfaceContainer grpIfs;
    std::ofstream mobilityLogStream;
}
using namespace containers;

Ipv6InterfaceContainer ASSIGN_SingleIpv6Address(Ptr<NetDevice> device, Ipv6Address addr, Ipv6Prefix prefix);
Ipv6InterfaceContainer ASSIGN_Ipv6Addresses(NetDeviceContainer devices, Ipv6Address network, Ipv6Prefix prefix);
Ipv6InterfaceContainer ASSIGN_WithoutAddress(Ptr<NetDevice> device);
void LOG_Settings();
void INSTALL_ConstantMobility(NodeContainer &nc, Ptr<ListPositionAllocator> positionAlloc);
void PRINIT_MNNs_DeviceInfor(const std::string &preface);
void INIT_MobilityTracing(std::ofstream &log);
void INIT_GrpFinder(NetDeviceContainer &devs);
void INIT_UdpApp();

void EchoTx1(Ptr< const Packet > p);
void EchoRx1(Ptr< const Packet > p);
void MacRx1(Ptr<Packet> packet, WifiMacHeader const *hdr);
static void CourseChange (std::string path, Ptr<const MobilityModel> mobility);

struct VanemoConfig
{
    VanemoConfig() :
        m_totalTime (0),
        m_packetInterval (0),
        m_packetSize (0),
        m_randomStart (0),
        m_nIfaces (0),
        m_chan (false),
        m_pcap (false),
        m_stack (""),
        m_root ("")
    {}

    VanemoConfig(int argc, char *argv[]) :
        m_totalTime (100.0),
        m_packetInterval (0.1),
        m_packetSize (1024),
        m_randomStart (1.0),
        m_nIfaces (1),
        m_chan (true),
        m_pcap (false),
        m_stack ("ns3::Dot11sStack"),
        m_root ("ff:ff:ff:ff:ff:ff")
    {
        CommandLine cmd;
        cmd.AddValue ("time",  "Simulation time (sec)", m_totalTime);
        cmd.AddValue ("packet-interval",  "Interval between packets in UDP ping (sec)", m_packetInterval);
        cmd.AddValue ("packet-size",  "Size of packets in UDP ping (bytes)", m_packetSize);
        cmd.AddValue ("start",  "Maximum random start delay for beacon jitter (sec)", m_randomStart);
        cmd.AddValue ("interfaces", "Number of radio interfaces used by each mesh point", m_nIfaces);
        cmd.AddValue ("channels",   "Use different frequency channels for different interfaces", m_chan);
        cmd.AddValue ("pcap",   "Enable PCAP traces on interfaces", m_pcap);
        cmd.AddValue ("stack",  "Type of protocol stack. ns3::Dot11sStack by default", m_stack);
        cmd.AddValue ("root", "Mac address of root mesh point in HWMP", m_root);
        cmd.Parse (argc, argv);
    }

    double    m_totalTime;
    //App settings
    double    m_packetInterval;
    uint16_t  m_packetSize;
    //Mesh Settings
    double    m_randomStart;
    uint32_t  m_nIfaces;
    bool      m_chan;
    bool      m_pcap;
    std::string m_stack;
    std::string m_root;
};

VanemoConfig script_cfg;

struct EchoApp
{
    EchoApp()
    {}

    void Setup (NodeContainer &nodes, Ipv4InterfaceContainer &interfaces)
    {
        NS_ASSERT_MSG(nodes.GetN() > 1,
                      "No nodes to install Echo Application on.");
        NS_ASSERT_MSG(nodes.GetN() == interfaces.GetN(),
                      "Node and ip interface counts do not match: " <<
                      nodes.GetN() << "/" << interfaces.GetN());
        uint32_t serverNode = 0, clientNode = nodes.GetN() - 1;
        UdpEchoServerHelper echoServer (9);
        ApplicationContainer serverApps = echoServer.Install (nodes.Get (serverNode));
        serverApps.Start (Seconds (1.5));
        serverApps.Stop (Seconds (script_cfg.m_totalTime));
        UdpEchoClientHelper echoClient (interfaces.GetAddress (serverNode), 9);
        echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(script_cfg.m_totalTime*(1/script_cfg.m_packetInterval))));
        echoClient.SetAttribute ("Interval", TimeValue (Seconds (script_cfg.m_packetInterval)));
        echoClient.SetAttribute ("PacketSize", UintegerValue (script_cfg.m_packetSize));
        ApplicationContainer clientApps = echoClient.Install (nodes.Get (clientNode));
        clientApps.Start (Seconds (2.0));
        clientApps.Stop (Seconds (script_cfg.m_totalTime));
        Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx", MakeCallback(&EchoTx1));
        Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::UdpEchoServer/Rx", MakeCallback(&EchoRx1));
    }
};

class MeshSetup
{
public:
MeshSetup(NodeContainer &nodes) :
  m_meshNodes (nodes)
{}


Ipv4InterfaceContainer& GetIpInterfaces()
{
    return m_ipInterfaces;
}

void Setup ()
{
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    mesh = MeshHelper::Default ();
    if (!Mac48Address (script_cfg.m_root.c_str ()).IsBroadcast ())
    {
      mesh.SetStackInstaller (script_cfg.m_stack, "Root", Mac48AddressValue (Mac48Address (script_cfg.m_root.c_str ())));
    }
    else
    {
      mesh.SetStackInstaller (script_cfg.m_stack);
    }
    if (script_cfg.m_chan)
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::SPREAD_CHANNELS);
    }
    else
    {
      mesh.SetSpreadInterfaceChannels (MeshHelper::ZERO_CHANNEL);
    }
    mesh.SetMacType ("RandomStart", TimeValue (Seconds (script_cfg.m_randomStart)));
    mesh.SetNumberOfInterfaces (script_cfg.m_nIfaces);
    meshDevices = mesh.Install (wifiPhy, m_meshNodes);

    for (uint32_t i = 0; i < meshDevices.GetN(); ++i)
    {
      Ptr<MeshPointDevice> device = DynamicCast<MeshPointDevice>(meshDevices.Get(i));
      NS_ASSERT (device != 0);

      std::vector<Ptr<NetDevice> > innerDevices = device->GetInterfaces ();
      for (std::vector<Ptr<NetDevice> >::iterator i = innerDevices.begin (); i != innerDevices.end (); i++)
        {
          Ptr<WifiNetDevice> wifiNetDev = (*i)->GetObject<WifiNetDevice> ();
          if (wifiNetDev == 0)
            {
              continue;
            }
          Ptr<MeshWifiInterfaceMac> mac = wifiNetDev->GetMac ()->GetObject<MeshWifiInterfaceMac> ();
          if (mac == 0)
            {
              continue;
            }

          Ptr<GroupFinder> gfApp = GroupFinder::GetGroupFinderApplication(device->GetNode());
          if (gfApp)
              mac->SetRecvCb(MakeCallback(&GroupFinder::GroupBCastReceived, gfApp));
        }

    }
    //IP Addressing
    IpAddressing ();
    //pcap
    if (script_cfg.m_pcap)
        wifiPhy.EnablePcapAll (std::string ("mp-"));
}

private:
void IpAddressing ()
{
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  m_ipInterfaces = address.Assign (meshDevices);
  for (uint32_t i = 0; i < m_ipInterfaces.GetN(); i++)
      NS_LOG_UNCOND("aDDED " << m_ipInterfaces.GetAddress (i) << " " << meshDevices.Get(i)->GetAddress());
}

void Report ()
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
private:
  NodeContainer &m_meshNodes;
  Ipv4InterfaceContainer m_ipInterfaces;
  MeshHelper mesh;
  NetDeviceContainer meshDevices;

};// MeshSetUp

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
//        LogLevel logDbg = static_cast<LogLevel>(LOG_LEVEL_DEBUG);
    //  LogComponentEnable ("Udp6Server", logInfo);
    //  LogComponentEnable ("Pmipv6Agent", logAll);
    //  LogComponentEnable ("Pmipv6MagNotifier", logAll);
    //  LogComponentEnable ("Pmipv6Wifi", logDbg);
    //  LogComponentEnable ("Pmipv6MagNotifier", logDbg);
    //  LogComponentEnable ("Pmipv6Mag", logDbg);
        LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
        LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

        //  LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);
        LogLevel logLogicFunctionInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_LOGIC | LOG_LEVEL_INFO | LOG_LEVEL_FUNCTION);
        LogLevel logFunctionInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO | LOG_LEVEL_FUNCTION);
        LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_INFO);
        (void) logLogicFunctionInfo;
        (void) logFunctionInfo;
        (void) logInfo;
    //    LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    //    LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    //    LogComponentEnable ("Ipv4Interface", LOG_LEVEL_LOGIC);
    //    LogComponentEnable ("Ipv6Interface", LOG_LEVEL_LOGIC);

//        LogComponentEnable ("UdpEchoClientApplication", logInfo);
//        LogComponentEnable ("UdpEchoServerApplication", logInfo);
    //    LogComponentEnable ("Ipv4Interface", logLogicFunctionInfo);
    //    LogComponentEnable ("Ipv6Interface", logLogicFunctionInfo);
    //    LogComponentEnable ("Icmpv6L4Protocol", logLogicFunctionInfo);
    //    LogComponentEnable ("Ipv6L3Protocol", logLogicFunctionInfo);
    //    LogComponentEnable ("Ipv6StaticRouting", logLogicFunctionInfo);
    //    LogComponentEnable ("NdiscCache", logLogicFunctionInfo);
//        LogComponentEnable ("HwmpProtocol", logLogicFunctionInfo);
//        LogComponentEnable ("HwmpRtable", logLogicFunctionInfo);
//        LogComponentEnable ("HwmpProtocolMac", logLogicFunctionInfo);
    //    LogComponentEnable ("MeshWifiInterfaceMac", logLogicFunctionInfo);
    //    LogComponentEnable ("MeshL2RoutingProtocol", logLogicFunctionInfo);
    //    LogComponentEnable ("MeshPointDevice", logLogicFunctionInfo);
//        LogComponentEnable ("Pmipv6Wifi", logLogicFunctionInfo);
        LogLevel loglogicNode = static_cast<LogLevel>(LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_LOGIC);
    //    LogComponentEnable ("TestMeshScript", loglogicNode);
        LogComponentEnable ("GroupFinder", loglogicNode);
}

void INSTALL_ConstantMobility(NodeContainer &nc, Ptr<ListPositionAllocator> positionAlloc)
{
    MobilityHelper mobility;
    mobility.SetPositionAllocator (positionAlloc);
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install (nc);
}

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

void EchoTx1(Ptr< const Packet > p)
{
    NS_LOG_UNCOND(p->GetUid() << " PACKET Echo Sent.");
}

void EchoRx1(Ptr< const Packet > p)
{
    NS_LOG_UNCOND(p->GetUid() << " PACKET Echo Received.");
}

void MacRx1(Ptr<Packet> packet, WifiMacHeader const *hdr)
{
//    NS_LOG_LOGIC(hdr->GetAddr2());
}


#define nof_street_mags  6
#define nof_streets  2
const int backBoneCnt = nof_street_mags * nof_streets;
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
    for(unsigned int i = 0; i < mnnsNormalDevs.GetN(); i++)
    {
      NS_LOG_UNCOND("Profile add MNN:" << i << " :" << Mac48Address::ConvertFrom(mnnsNormalDevs.Get(i)->GetAddress()));
      profile->AddProfile(Identifier(Mac48Address::ConvertFrom(
                                                       mnnsNormalDevs.Get(i)->GetAddress())),
                          Identifier(Mac48Address::ConvertFrom(
                                             mnnsNormalDevs.Get(i)->GetAddress())),
                          backboneIfs.GetAddress(0, 1),
                          std::list<Ipv6Address>());
    }
    return profile;
}

void INIT_MNN_Mobility()
{
    MobilityHelper mobility;
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobility.Install(mnns);
}

void INIT_VelocitySensor(
        double interval,
        VelocitySensor::UpdateMode updateMethod =
            VelocitySensor::MSU_ALL)
{
    //Server Application
    ApplicationContainer velocitySensor;
    VelocitySensorHelper vs(Seconds (interval), updateMethod);
    //do settings
    velocitySensor = vs.Install(NodeContainer(leader, followers));
    velocitySensor.Start (Seconds (0.0));
    velocitySensor.Stop (Seconds (script_cfg.m_totalTime));
}

void INIT_GrpFinder(NetDeviceContainer &devs)
{
    NS_ASSERT_MSG(mnns.GetN() == devs.GetN(),
                  "INIT_GrpFinder, node-dev cnount mismatch:" <<
                  mnns.GetN() << "/" << devs.GetN());
    //Server Application
    ApplicationContainer grpFinder;

    GroupFinderHelper::SetEnable(true);
    GroupFinderHelper gf;
    //do settings
    grpFinder = gf.Install(mnns, devs);
    grpFinder.Start (Seconds (0.0));
    grpFinder.Stop (Seconds (script_cfg.m_totalTime));
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

      serverApps.Start (Seconds (1.0));
      clientApps.Start (Seconds (1.5));
      serverApps.Stop (Seconds (script_cfg.m_totalTime));
      clientApps.Stop (Seconds (script_cfg.m_totalTime));
}

void INIT_Anim(AnimationInterface &anim)
{
    NS_LOG_UNCOND("Animator Settings");
    //  anim.SetMaxPktsPerTraceFile(300000);
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

void INIT_MobilityTracing(std::ofstream &log)
{
      std::string cur_path =
              "/home/user/Downloads/ns/rts/"
              "ns-allinone-3.22.14-jul-2017/ns-allinone-3.22/ns-3.22/";
      std::string traceFile(cur_path + "mnn3_trace.tcl");
      std::string logFile(cur_path + "mnn_trace.log");
      Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);
      ns2.Install ();
      log.open (logFile.c_str ());
      Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange",
                       MakeCallback (&CourseChange/*, &mobilityLogStream*/));
}

void INIT_Pmip()
{
    Ptr<Pmipv6ProfileHelper> profile = ENABLE_LMA_Profiling();
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

void CourseChange (std::string path, Ptr<const MobilityModel> mobility)
{
  Vector pos = mobility->GetPosition (); // Get position
  Vector vel = mobility->GetVelocity (); // Get velocity

  // Prints position and velocities
  mobilityLogStream << Simulator::Now () /*<< " " << path*/ << " POS: x=" << pos.x << ", y=" << pos.y
      << ", z=" << pos.z << "; VEL: x= " << vel.x << ", y=" << vel.y
      << ", z=" << vel.z << std::endl;
}

void PMIP_Setup ()
{
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
  for (int i = 0; i < nof_street_mags; i++)
  {
      double x = 50.0 + (i* 100);
      for (int j = 0; j < nof_streets ; j++)
      {
          double y = 60.0 + (150 * j);
          positionAlloc->Add (Vector (x, y, 0.0)); //MAGi
      }
  }
  INSTALL_ConstantMobility(lmaMagNodes, positionAlloc);

  //CN Mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (75.0, 20.0, 0.0));   //CN
  INSTALL_ConstantMobility(cn, positionAlloc);

  //AP mobility
  positionAlloc = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < nof_street_mags; i++)
  {
      double x = 50.0 + (i* 100);
      for (int j = 0; j < nof_streets; j++)
      {
          double y = 80.0 + (150 * j);
          positionAlloc->Add (Vector (x, y, 0.0)); //MAGAPi
      }
  }
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
  mnnsNormalDevs = wifi.Install (wifiPhy, wifiMac, mnns);
  iifc = ASSIGN_Ipv6Addresses(mnnsNormalDevs, Ipv6Address("3ffe:6:6:1::"), 64);
  iifc.SetForwarding (0, true);
  iifc.SetDefaultRouteInAllNodes (0);
  PRINIT_MNNs_DeviceInfor("After EXTERNAL MNNs StaWifi Installation");

  leaderDev.Add(mnnsNormalDevs.Get(0));
  for (size_t i = 1; i < mnnsNormalDevs.GetN(); i++)
      followerDevs.Add(mnnsNormalDevs.Get(i));


  //End addresses
  Ipv6Address glExternalAddress = mnnsNormalDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();
  Ipv6Address glInternalAddress = mnnsNormalDevs.Get(0)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();
  Ipv6Address mnn2ExternalAddress = mnnsNormalDevs.Get(2)->GetNode ()->GetObject<Ipv6> ()->GetAddress(1, 1).GetAddress();

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
//  //Routes From STA (GL)
//  sr = ipv6RoutingHelper.GetStaticRouting (ipv6gl);
//  Ptr<Ipv6> ip6Mnn1 = mnns.Get(1)->GetObject<Ipv6> ();
//  next = ip6Mnn1->GetInterfaceForDevice(mnns.Get(1)->GetDevice(2));
//  NS_LOG_UNCOND("next:" << next);
//  sr->AddHostRouteTo (destAddress, ip6Mnn1->GetAddress(next, 1).GetAddress(), 1);
//  NS_LOG_UNCOND("MNN0(GL): AddHostRouteTo: " << destAddress << " --> " << ip6Mnn1->GetAddress(next, 1).GetAddress() << " 1");
//  //Routes From relay node
//  sr = ipv6RoutingHelper.GetStaticRouting (ip6Mnn1);
//  Ptr<Ipv6> ip6Mnn2 = mnns.Get(2)->GetObject<Ipv6> ();
//  next = ip6Mnn2->GetInterfaceForDevice(mnns.Get(2)->GetDevice(2));
//  NS_LOG_UNCOND("next:" << next);
//  sr->AddHostRouteTo (destAddress, ip6Mnn2->GetAddress(next, 1).GetAddress(), 2);
//  NS_LOG_UNCOND("MNN1: AddHostRouteTo: " << destAddress << " --> " << ip6Mnn2->GetAddress(next, 1).GetAddress() << " 2");


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

  wifiPhy.EnablePcap ("wifi-ext-mnns", mnnsNormalDevs);

  NS_LOG_UNCOND("Installing SENSOR");

  INIT_VelocitySensor(1.0);

  NS_LOG_UNCOND("Installing GRP FINDER");
  INIT_GrpFinder(mnnsNormalDevs);

  //Application
  NS_LOG_UNCOND("Installing UDP server on MN");
  INIT_UdpApp();


  PRINIT_MNNs_DeviceInfor("END");
}

void run(const VanemoConfig &cfg)
{
    NS_LOG_UNCOND("Do");
    Simulator::Stop (Seconds (cfg.m_totalTime));
    Simulator::Run ();
    Simulator::Destroy ();
    if (mobilityLogStream.is_open())
    	mobilityLogStream.close();
}

int
main (int argc, char *argv[])
{
    script_cfg = VanemoConfig(argc,argv);
    LOG_Settings();
    SeedManager::SetSeed (123456);
    CREATE_Nodes();
    PMIP_Setup();
    MeshSetup meshSetup(mnns);
    meshSetup.Setup ();
    EchoApp echoApp;
    echoApp.Setup(mnns, meshSetup.GetIpInterfaces());
    INIT_MobilityTracing(mobilityLogStream);
    AnimationInterface anim("PMIPv6_TRACE.xml");
    INIT_Anim(anim);
    run(script_cfg);
}
