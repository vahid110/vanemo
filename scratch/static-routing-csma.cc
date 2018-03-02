/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 *
 */

// Test program for this 3-router scenario, using static routing
//
// (a.a.a.a/32)A<--x.x.x.0/30-->B<--y.y.y.0/30-->C(c.c.c.c/32)

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/mobility-module.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("StaticRoutingSlash32Test");

void EchoTx1(Ptr< const Packet > p)
{
    NS_LOG_LOGIC(" Echo Send PACKET : " << p->GetUid());
}

void EchoRx1(Ptr< const Packet > p)
{
	NS_LOG_LOGIC(" Echo Receive PACKET : " << p->GetUid());
}

struct EchoApp
{
    EchoApp()
    {}

    void Setup (NodeContainer &nodes)
    {
        UdpEchoServerHelper echoServer (9);
        ApplicationContainer serverApps = echoServer.Install (nodes.Get (0));
        serverApps.Start (Seconds (1));
        serverApps.Stop (Seconds (4));
        UdpEchoClientHelper echoClient (Ipv4Address ("10.1.1.6"), 9);
        echoClient.SetAttribute ("MaxPackets", UintegerValue ((uint32_t)(5)));
        echoClient.SetAttribute ("Interval", TimeValue (Seconds (1)));
        echoClient.SetAttribute ("PacketSize", UintegerValue (1024));
        ApplicationContainer clientApps = echoClient.Install (nodes.Get (1));
        clientApps.Start (Seconds (2.0));
        clientApps.Stop (Seconds (4));
        Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::UdpEchoClient/Tx", MakeCallback(&EchoTx1));
        Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::UdpEchoServer/Rx", MakeCallback(&EchoRx1));
    }
};

void printNodeDevices(Ptr<Node> n, std::string name)
{
	if (n->GetNDevices())
		NS_LOG_LOGIC(name << " Devices:");
	else
		NS_LOG_LOGIC(name << " Devices: empty");
	for (uint32_t i=0; i < n->GetNDevices(); i++)
		NS_LOG_UNCOND(n->GetDevice(i));
}

int 
main (int argc, char *argv[])
{

  // Allow the user to override any of the defaults and the above
  // DefaultValue::Bind ()s at run-time, via command-line arguments
  CommandLine cmd;
  cmd.Parse (argc, argv);
//  LogComponentEnable ("Ipv4StaticRouting", LOG_LEVEL_FUNCTION);
  LogLevel loglogicNode = static_cast<LogLevel>(LOG_PREFIX_NODE | LOG_LEVEL_LOGIC | LOG_LEVEL_FUNCTION);
  LogComponentEnable ("StaticRoutingSlash32Test",loglogicNode);
//  LogComponentEnable ("Ipv4ListRouting", loglogicNode);
//  LogComponentEnable ("Ipv4L3Protocol", loglogicNode);
//  LogComponentEnable ("Node", loglogicNode);
  Ptr<Node> nA = CreateObject<Node> ();
  Ptr<Node> nB = CreateObject<Node> ();
  Ptr<Node> nC = CreateObject<Node> ();
//  NS_LOG_LOGIC("Initial A :" << nA->GetNDevices() << " B:" << nB->GetNDevices() << " C:" << nC->GetNDevices());
  NodeContainer c = NodeContainer (nA, nB, nC);

  InternetStackHelper internet;
  internet.Install (c);
  NS_LOG_LOGIC("After Internet A :" <<
		  nA->GetNDevices() << " B:" <<
		  nB->GetNDevices() << " C:" <<
		  nC->GetNDevices());

  // Point-to-point links
  NodeContainer nAnB = NodeContainer (nA, nB);
  NodeContainer nBnC = NodeContainer (nB, nC);

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate(50000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(100)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1400));

  NetDeviceContainer dAdB = csma.Install (nAnB);

  NetDeviceContainer dBdC = csma.Install (nBnC);
  Ptr<NetDevice> deviceC = dBdC.Get(1);

  // Later, we add IP addresses.
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer iAiB = ipv4.Assign (dAdB);

  ipv4.SetBase ("10.1.1.4", "255.255.255.252");
  Ipv4InterfaceContainer iBiC = ipv4.Assign (dBdC);

  Ptr<Ipv4> ipv4A = nA->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4B = nB->GetObject<Ipv4> ();
  Ptr<Ipv4> ipv4C = nC->GetObject<Ipv4> ();

//  int32_t ifIndexA = ipv4A->GetInterfaceForDevice(nA->GetDevice(1));
//  int32_t ifIndexB1 = ipv4B->GetInterfaceForDevice(nB->GetDevice(1));
//  int32_t ifIndexB2 = ipv4B->GetInterfaceForDevice(nB->GetDevice(2));
//  int32_t ifIndexC = ipv4C->GetInterfaceForDevice (deviceC);
//  NS_LOG_LOGIC("Devices A :" << nA->GetNDevices() << " B:" << nB->GetNDevices() << " C:" << nC->GetNDevices());
//  NS_LOG_UNCOND("Ifce index: " << ifIndexA << "," << ifIndexB1 << "," << ifIndexB2 << "," << ifIndexC);
//  ipv4A->SetMetric (ifIndexA, 1);
//  ipv4A->SetUp (ifIndexA);
//  ipv4A->SetForwarding(ifIndexA, true);
//
//  ipv4B->SetMetric (ifIndexB1, 1);
//  ipv4B->SetUp (ifIndexB1);
//  ipv4B->SetForwarding(ifIndexB1, true);
//
//  ipv4B->SetMetric (ifIndexB2, 1);
//  ipv4B->SetUp (ifIndexB2);
//  ipv4B->SetForwarding(ifIndexB2, true);
//
//  ipv4C->SetMetric (ifIndexC, 1);
//  ipv4C->SetUp (ifIndexC);
//  ipv4C->SetForwarding(ifIndexC, true);
//
//  MobilityHelper mobility;
//  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
//  positionAlloc->Add (Vector (0.0, 10.0, 0.0));
//  positionAlloc->Add (Vector (0.0, 20.0, 0.0));
//  positionAlloc->Add (Vector (0.0, 30.0, 0.0));
//
//  mobility.SetPositionAllocator (positionAlloc);
//  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
//  mobility.Install (c);
//
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  // Create static routes from A to C
  Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4A);
  // The ifIndex for this outbound route is 1; the first p2p link added
  staticRoutingA->AddHostRouteTo (Ipv4Address ("10.1.1.6"), Ipv4Address ("10.1.1.2"), 1);
  Ptr<Ipv4StaticRouting> staticRoutingB = ipv4RoutingHelper.GetStaticRouting (ipv4B);
  // The ifIndex we want on node B is 2; 0 corresponds to loopback, and 1 to the first point to point link
  staticRoutingB->AddHostRouteTo (Ipv4Address ("10.1.1.6"), 2);

  NodeContainer nCnA(nC, nA);
  EchoApp echo;
  echo.Setup(nCnA);
  NS_LOG_UNCOND("======= START =======");


  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
