#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-static-routing-helper.h"//Added
#include "ns3/applications-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wsa");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      NS_LOG_UNCOND ("Received one packet!");
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, 
                             uint32_t pktCount, Time pktInterval )
{

  if (pktCount > 0)
    {
      std::cout << "Generating Traffic: Sent: " << socket->Send (Create<Packet> (pktSize)) << std::endl;
      Simulator::Schedule (pktInterval, &GenerateTraffic, 
                           socket, pktSize,pktCount-1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}


int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double rss = -80;
  uint32_t packetSize = 1000;
  uint32_t numPackets = 10;
  double interval = 1.0;
  bool verbose = false;

  CommandLine cmd;

  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);

  cmd.Parse (argc, argv);
  Time interPacketInterval = Seconds (interval);

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

  Ipv4AddressHelper ipv4;
  NS_LOG_UNCOND("Assign IP Addresses.");
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  NS_LOG_UNCOND(i.GetAddress(0));

  //Modified(Added)
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4> ipv4c2 = c.Get (2)->GetObject<Ipv4> ();
  Ptr<Ipv4StaticRouting> staticRoutingC2 = ipv4RoutingHelper.GetStaticRouting (ipv4c2);
  staticRoutingC2->AddHostRouteTo (Ipv4Address ("10.1.1.1"), Ipv4Address ("10.1.1.2"), 1);

//  //Modified(Added)
//  Ptr<Ipv4> ipv4c1 = c.Get (1)->GetObject<Ipv4> ();
//  Ptr<Ipv4StaticRouting> staticRoutingC1 = ipv4RoutingHelper.GetStaticRouting (ipv4c1);
//  staticRoutingC1->AddHostRouteTo (Ipv4Address ("10.1.1.1"), 1);

  {

	LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);
  // UDP Client SerVER app
  uint16_t port = 6000;
  ApplicationContainer serverApps, clientApps;
  UdpServerHelper server (port);
  serverApps = server.Install (c.Get(0));

  //Clinet Application
  NS_LOG_UNCOND("Installing UDP client on CN");
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 30;
  Time interPacketInterval = MilliSeconds(1000);
  UdpClientHelper udpClient(i.GetAddress(0), port);
  udpClient.SetAttribute ("Interval", TimeValue (interPacketInterval));
  udpClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  udpClient.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  clientApps = udpClient.Install (c.Get (2));

  serverApps.Start (Seconds (1.0));
  clientApps.Start (Seconds (1.5));
  serverApps.Stop (Seconds (10.0));
  clientApps.Stop (Seconds (10.0));
  }

//  //ONOFF APP
//  uint16_t port = 9;   // Discard port (RFC 863)
//  OnOffHelper onoff ("ns3::UdpSocketFactory",
//                     Address (InetSocketAddress (i.GetAddress(0), port)));
//  onoff.SetConstantRate (DataRate (6000));
//  ApplicationContainer apps = onoff.Install (c.Get(2));
//  apps.Start (Seconds (1.0));
//  apps.Stop (Seconds (10.0));
//
//  // Create a packet sink to receive these packets
//  PacketSinkHelper sink ("ns3::UdpSocketFactory",
//                         Address (InetSocketAddress (Ipv4Address::GetAny (), port)));
//  apps = sink.Install (c.Get(0));
//  apps.Start (Seconds (1.0));
//  apps.Stop (Seconds (10.0));



  //SOCKET BASE APP
//  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
//  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
//  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
//  recvSink->Bind (local);
//  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
//
////  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
//  Ptr<Socket> source = Socket::CreateSocket (c.Get (2), tid);//Modified
//
//  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
//  source->SetAllowBroadcast (true);
//  source->Connect (remote);
//
  wifiPhy.EnablePcap ("wifi-simple-adhoc", devices);
//
//  NS_LOG_UNCOND ("Testing " << numPackets  << " packets sent with receiver rss " << rss );
//
//  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
//                                  Seconds (1.0), &GenerateTraffic,
//                                  source, packetSize, numPackets, interPacketInterval);

  Simulator::Run ();
  Simulator::Destroy ();

//  //  //Modified(Added)
//  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (apps.Get (0));
//  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;

  return 0;
}

