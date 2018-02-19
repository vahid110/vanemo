#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wsa");

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  bool verbose = false;

  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", 
                      StringValue (phyMode));

  NodeContainer c;
  c.Create (3);

  WifiHelper wifi;
  if (verbose)
  {
      wifi.EnableLogComponents ();
  }
  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set ("RxGain", DoubleValue (0) ); 
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO); 

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::RangePropagationLossModel", "MaxRange", DoubleValue (100));
  wifiPhy.SetChannel (wifiChannel.Create ());

  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, c);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  //Modified:
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (70.0, 0.0, 0.0));
  positionAlloc->Add (Vector (140.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv6AddressHelper ipv6;
  NS_LOG_UNCOND("Assign IP Addresses.");
  ipv6.SetBase (Ipv6Address("3ffe:6:6:1::"), 64);
  Ipv6InterfaceContainer i = ipv6.Assign (devices);
  i.SetForwarding (0, true);
  i.SetDefaultRouteInAllNodes (0);
  NS_LOG_UNCOND(i.GetAddress(0, 1));
  NS_LOG_UNCOND(i.GetAddress(1, 1));
  NS_LOG_UNCOND(i.GetAddress(2, 1));

 //Static Routing
  Ipv6StaticRoutingHelper ipv6RoutingHelper;
  Ptr<Ipv6> ipv6c2 = c.Get (2)->GetObject<Ipv6> ();
  Ptr<Ipv6StaticRouting> staticRoutingC2 = ipv6RoutingHelper.GetStaticRouting (ipv6c2);
  staticRoutingC2->AddHostRouteTo (i.GetAddress(0, 1), i.GetAddress(1, 1), 1);
  NS_LOG_UNCOND("AddHostRouteTo(" << i.GetAddress(0, 1) << ", " << i.GetAddress(1, 1) << ", 1)");

//  Ptr<Ipv6> ipv6c1 = c.Get (1)->GetObject<Ipv6> ();
//  Ptr<Ipv6StaticRouting> staticRoutingC1 = ipv6RoutingHelper.GetStaticRouting (ipv6c1);
//  staticRoutingC1->AddHostRouteTo (i.GetAddress(0, 1), 1);
//  NS_LOG_UNCOND("AddHostRouteTo(" << i.GetAddress(0, 1) << ", 1)");

  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  // UDP Client Server Applications
  uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps;
  UdpServerHelper server (port);
  serverApps = server.Install (c.Get(0));

  //Clinet Application
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 30;
  Time interPacketInterval = MilliSeconds(1000);
  UdpClientHelper udpClient(i.GetAddress(0,1), port);
  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  clientApps = udpClient.Install (c.Get (2));

  serverApps.Start (Seconds (1.0));
  clientApps.Start (Seconds (1.5));
  serverApps.Stop (Seconds (10.0));
  clientApps.Stop (Seconds (10.0));


  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);

  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}

