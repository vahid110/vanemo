/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 FIIT STU
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
// //             n0   r  r1    n1
// //             |    _   _  _	|
// //             ====|_|=|_|==
// //             	   router
// //
// //
#include <fstream>
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-helper.h"

#include "ns3/ipv6-transition-address-generator.h"
#include "ns3/dualstack-container.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DualstackHelperExample");

int main(int argc, char** argv) {
	bool verbose = false;

	CommandLine cmd;
	cmd.AddValue("verbose", "turn on log components", verbose);
	cmd.Parse(argc, argv);
	LogComponentEnable("DualstackHelperExample", LOG_LEVEL_ALL);

	if (verbose)
	{
		LogComponentEnable("Ipv4L3Protocol", LOG_LEVEL_ALL);
		LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
		LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
		LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
		LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
		LogComponentEnable("Icmpv6Header", LOG_LEVEL_ALL);
		LogComponentEnable("Ping6Application", LOG_LEVEL_INFO);
		LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
		LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);
	}
	LogComponentEnable("Ping6Application", LOG_LEVEL_INFO);
	LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
	LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

	NS_LOG_INFO("Create nodes.");

	Ptr<Node> n0 = CreateObject<Node>();
	Names::Add("n0",n0);


	Ptr<Node> r = CreateObject<Node>();
	Names::Add("r",r);

	Ptr<Node> r1 = CreateObject<Node>();
	Names::Add("r1",r1);

	NodeContainer ipv6Only(n0, r);
	NodeContainer dualstackContainer(r, r1);

	NS_LOG_INFO("Create IPv6 Internet Stack");

	InternetStackHelper internetv6;
	internetv6.SetIpv4StackInstall(false);

	internetv6.Install(n0);

	InternetStackHelper internetDual;
	internetDual.Install(dualstackContainer);

	NS_LOG_INFO("Create channels.");
	CsmaHelper csma;
	csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
	csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
	NetDeviceContainer c6 = csma.Install(ipv6Only);
	NetDeviceContainer dc = csma.Install(dualstackContainer);

	NS_LOG_INFO("Create networks and assign IPv6 Addresses.");
	NS_LOG_UNCOND("\ni6:");
	Ipv6TransitionAddressGenerator addrhelp;
	//set IPv6 base address with EUI-48
	addrhelp.SetIpv6Base("3443::", "::1",true);
	DualStackContainer i6 = addrhelp.AssignIpv6Only(c6);
	i6.SetDefaultRouteInAllNodes(1);
	i6.SetForwarding(1, true);
	i6.Print();

	NS_LOG_UNCOND("\ni46:");
	//assign IPv4 and IPv6 address
	addrhelp.SetIpv4Base("40.1.2.0", "255.255.255.0", "0.0.0.1");
	addrhelp.SetIpv6Base("1234::", "::1", false);
	DualStackContainer i46 = addrhelp.AssignDualStack(dc);
	i46.SetDefaultRouteInAllNodes(0);
	i46.SetForwarding(0, true);
	i46.Print();

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

	Ipv6StaticRoutingHelper routingHelper6;
	Ptr<OutputStreamWrapper> routingStream6 =
			Create<OutputStreamWrapper> (&std::cout);
	routingHelper6.PrintRoutingTableAt (Seconds (30), r, routingStream6);

	Ipv6StaticRoutingHelper routingHelper;
	Ptr<OutputStreamWrapper> routingStream =
			Create<OutputStreamWrapper> (&std::cout);
	routingHelper.PrintRoutingTableAt (Seconds (30), r1, routingStream);

	UdpEchoServerHelper echoServer(10);
	ApplicationContainer serverApps = echoServer.Install (r1);
	serverApps.Start (Seconds (29.0));
	serverApps.Stop (Seconds (110.0));

	UdpEchoClientHelper echoClient (i46.GetIpv6Address(1,1), 10);
	echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
	echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
	echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

	ApplicationContainer clientApps = echoClient.Install (n0);
	clientApps.Start (Seconds (30.0));
	clientApps.Stop (Seconds (110.0));



//	/* Create a Ping6 application to send ICMPv6 echo request from n0 to n1 via r */
//	uint32_t packetSize = 100;
//	uint32_t maxPacketCount = 10;
//	Time interPacketInterval = Seconds(1.0);
//
//	Ping6Helper ping6;
//	NS_LOG_UNCOND("\nPING " << i6.GetIpv6Address(0,1) << " -> " << i46.GetIpv6Address(1,1));
//
//	ping6.SetLocal (i6.GetIpv6Address(0,1));
//	ping6.SetRemote (i46.GetIpv6Address(1,1));
//
//	ping6.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
//	ping6.SetAttribute("Interval", TimeValue(interPacketInterval));
//	ping6.SetAttribute("PacketSize", UintegerValue(packetSize));
//	ApplicationContainer apps = ping6.Install(ipv6Only.Get(0));
//
//	apps.Start(Seconds(40.0));
//	apps.Stop(Seconds(110.0));


	Packet::EnablePrinting();

	AsciiTraceHelper ascii;
	csma.EnableAsciiAll (ascii.CreateFileStream ("dualstack-helper-example.tr"));
	csma.EnablePcapAll (std::string ("dualstack-helper-example"), true);

	NS_LOG_INFO("Run Simulation.");
	Simulator::Stop(Seconds(120));
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Done.");

}
