/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Lukáš Danielovič <lukas.danielovic@gmail.com>
 */

// Network topology
// //
// //             n0  	  IPv4 net	n1
// //             |    _   _   _	|
// //   IPv6 net  ====|_|=|_|=|_|===	IPv6 net
// //             |	   r   r1  r2	|
// //			  n0a				n1a
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/ipv6-routing-table-entry.h"

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/config.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/ipv6-static-routing.h"
#include "ns3/ipv6-list-routing.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

#include "ns3/ipv4-end-point.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"

#include <string>

#include "ns3/ipv6-transition-address-generator.h"
#include "ns3/dualstack-container.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("6to4Example");

int main(int argc, char** argv) {
	bool verbose = false;

	CommandLine cmd;
	cmd.AddValue("verbose", "turn on log components", verbose);
	cmd.Parse(argc, argv);
    LogLevel logInfo = static_cast<LogLevel>(LOG_PREFIX_NODE | LOG_PREFIX_FUNC | LOG_LEVEL_INFO);
    LogLevel logAll = static_cast<LogLevel>(LOG_PREFIX_NODE | LOG_PREFIX_FUNC | LOG_LEVEL_ALL);
    LogLevel logLogic = static_cast<LogLevel>(LOG_PREFIX_NODE | LOG_LEVEL_LOGIC| LOG_LEVEL_INFO);
    LogLevel logFunc = static_cast<LogLevel>(LOG_PREFIX_NODE | LOG_PREFIX_FUNC | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO);
    (void) logLogic;
    (void) logFunc;
    (void) logAll;
    (void) logInfo;
//	LogComponentEnable("6to4Example", logAll);
//	LogComponentEnable("Ipv6TransitionAddressGenerator", logAll);
//	LogComponentEnable("Transition6Rd", logAll);
//	LogComponentEnable("Transition6In4", logAll);
//	LogComponentEnable("Ipv6EncapsulationHeader", logAll);
//	LogComponentEnable("Ipv6L3Protocol", logInfo);
//	LogComponentEnable("Ipv4L3Protocol", logInfo);
//	LogComponentEnable("DualStackContainer", logAll);
	LogComponentEnable("Ping6Application", logInfo);
//	LogComponentEnable("Icmpv6L4Protocol", logInfo);
	NS_LOG_INFO("Create nodes.");

	Ptr<Node> n0 = CreateObject<Node>();
	Names::Add("n0", n0);
	Ptr<Node> n0a = CreateObject<Node>();
	Names::Add("n0a", n0a);
	Ptr<Node> n1a = CreateObject<Node>();
	Names::Add("n1a", n1a);
	Ptr<Node> r = CreateObject<Node>();
	Names::Add("r", r);
	Ptr<Node> r2 = CreateObject<Node>();
	Ptr<Node> r1 = CreateObject<Node>();
	Ptr<Node> n1 = CreateObject<Node>();
	Names::Add("r1", r1);
	Names::Add("r2", r2);
	Names::Add("n1", n1);
	NodeContainer net1(n0, r, n0a);
	NodeContainer net2(n1, r2, n1a);
	NodeContainer inter1(r, r1);
	NodeContainer inter2(r1, r2);
	NodeContainer routers(r, r1, r2);
	NodeContainer nodes(n0, n1, n0a, n1a);

	Packet::EnablePrinting();

	NS_LOG_INFO("Create IPv6 Internet Stack");

	NS_LOG_INFO("Create channels.");
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

	NetDeviceContainer d1 = csma.Install(net1);
	NetDeviceContainer d2 = csma.Install(net2);
	///////////////////
//	NetDeviceContainer rout1 = csma.Install(inter1);
	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc =
			CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (50.0, 50.0, 0.0));
	positionAlloc->Add (Vector (50.0, 60.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (inter1);

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
	NetDeviceContainer rout1 = wifi.Install(wifiPhy,wifiMac, r);
	wifiMac.SetType ("ns3::StaWifiMac",
				   "Ssid", SsidValue (ssid),
				   "ActiveProbing", BooleanValue (false));
	rout1.Add(wifi.Install (wifiPhy, wifiMac, r1));
	/////////////////////////////
	NetDeviceContainer rout2 = csma.Install(inter2);
	positionAlloc =
			CreateObject<ListPositionAllocator> ();
	positionAlloc->Add (Vector (550.0, 60.0, 0.0));
	mobility.SetPositionAllocator (positionAlloc);
	mobility.Install (r2);


	InternetStackHelper internetv6routers;
	internetv6routers.Install(routers);
	internetv6routers.Install(nodes);
//	InternetStackHelper internetv6nodes;
//	internetv6nodes.Install(nodes);

	NS_LOG_INFO("Create networks and assign IPv6 Addresses.");

	NS_LOG_INFO("net1:");
	Ipv6TransitionAddressGenerator sixto4addrhelp;
	sixto4addrhelp.Set6To4Addr("10.10.10.6", "::1", false);
	DualStackContainer i1 = sixto4addrhelp.AssignIpv6Only(d1);
	i1.Print();
	i1.SetForwarding(1, true);
	i1.Set6to4Router(1, true);
	i1.SetDefaultRouteInAllNodes(1);

	NS_LOG_INFO("rout1:");
	//Sets IPv4 network with dedicated address for 6to4 router
	sixto4addrhelp.SetIpv4Base("10.10.10.0", "255.255.255.0", "0.0.0.1");
	DualStackContainer routers1 = sixto4addrhelp.Assign6to4Ipv4Only(rout1, 0,
			"10.10.10.6");
	routers1.SetForwarding(0, true);
	routers1.SetForwarding(1, true);
	routers1.Print();

	NS_LOG_INFO("rout2:");
	sixto4addrhelp.SetIpv4Base("10.20.20.0", "255.255.255.0", "0.0.0.1");
	DualStackContainer routers2 = sixto4addrhelp.Assign6to4Ipv4Only(rout2, 1,
			"10.20.20.20");
	routers2.SetForwarding(0, true);
	routers2.SetForwarding(1, true);
	routers2.Print();

	NS_LOG_INFO("net2:");
	//sixto4addrhelp.SetIpv6Base("1234::", "::1", false);
	sixto4addrhelp.Set6To4Addr("10.20.20.20", "::1", false);
	DualStackContainer i2 = sixto4addrhelp.AssignIpv6Only(d2);
	i2.SetForwarding(1, true);
	i2.Set6to4Router(1, true);
	i2.SetDefaultRouteInAllNodes(1);
	i2.Print();

	//sets router as 6to4 relay
	//i2.Set6to4Relay(1);

//	Ipv6StaticRoutingHelper routingHelper6;
//	Ptr<OutputStreamWrapper> routingStream6 = Create<OutputStreamWrapper>(
//			&std::cout);
//	routingHelper6.PrintRoutingTableAt(Seconds(30), r, routingStream6);
//	Ipv6StaticRoutingHelper routingHelper;
//	Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(
//			&std::cout);
//	routingHelper.PrintRoutingTableAt(Seconds(30), n0, routingStream);
//
	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	/* Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via r */
	uint32_t packetSize = 52;
	uint32_t maxPacketCount = 10;
	Time interPacketInterval = Seconds(1.0);

	Ping6Helper ping6;

	ping6.SetLocal(i2.GetIpv6Address(0, 1));
	ping6.SetRemote(i1.GetIpv6Address(0, 1));
	NS_LOG_UNCOND("PING : " << i2.GetIpv6Address(0, 1) << " " << i1.GetIpv6Address(0, 1));
	ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
	ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
	ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
	ApplicationContainer apps = ping6.Install(n1);
	apps.Start(Seconds(40.0));
	apps.Stop(Seconds(60.0));


//	apps = ping6.Install(n1);
//	apps.Start(Seconds(75.0));
//	apps.Stop(Seconds(110.0));

	AsciiTraceHelper ascii;
	csma.EnableAsciiAll(ascii.CreateFileStream("6to4-example.tr"));
	csma.EnablePcapAll(std::string("6to4-example"), true);

	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(120));
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");

}
