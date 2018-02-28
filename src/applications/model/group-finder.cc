#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/wifi-mac-header.h"
#include "group-finder.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GroupFinder");

NS_OBJECT_ENSURE_REGISTERED (GroupFinder);

bool GroupFinder::m_enable = true;
std::map<Mac48Address, Ptr<Node> > GroupFinder::m_meshMacToNode;
std::map<Mac48Address, Ptr<Node> > GroupFinder::m_pmipMacToNode;
std::map<Ptr<Node>, Mac48Address > GroupFinder::m_pmipNodeToMac;

TypeId
GroupFinder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroupFinder")
    .SetParent<Application> ()
    .AddConstructor<GroupFinder> ();
  return tid;
}

GroupFinder::GroupFinder ()
	: m_devices()
	, m_bind_mag()
	, m_is_grp_leader(false)
	, m_curMobilityState(VelocitySensor::VS_UNKNOWN)
	, m_reportInterval(1)
{}

void
GroupFinder::SetEnable(bool value)
{
    m_enable = value;
}

bool
GroupFinder::IsEnabled()
{
    return m_enable;
}

void
GroupFinder::AddPmipMac(const Mac48Address &mac, Ptr<Node> node)
{
    m_pmipMacToNode[mac] = node;
    m_pmipNodeToMac[node] = mac;
}

Ptr<Node>
GroupFinder::GetNodeByPmipMac(const Mac48Address &mac)
{
	std::map<Mac48Address, Ptr<Node> >::iterator it = m_pmipMacToNode.find(mac);
	if (it == m_pmipMacToNode.end())
	{
		NS_LOG_WARN("Could not find a node among " << m_pmipMacToNode.size() << " nodes.");
		for(it = m_pmipMacToNode.begin(); it != m_pmipMacToNode.end(); it++)
			NS_LOG_WARN("Node:::: " << it->second->GetId() << ":" << it->first);
		return 0;
	}
    return it->second;
}


Ptr<GroupFinder>
GroupFinder::GetGroupFinderApplication(Ptr<Node> node)
{
	Ptr<GroupFinder> app;
	for (uint32_t i = 0; i < node->GetNApplications(); i++)
	{
		app = node->GetApplication(i)->GetObject<GroupFinder>();
	    if (app)
		    break;
	}
	return app;
}

const std::set<Mac48Address>&
GroupFinder::GetGroup() const
{
    return  m_curPmipMacs;
}

void GroupFinder::SetBindMag(const Ipv6Address& val)
{
	m_bind_mag = val;
}

Ipv6Address GroupFinder::GetBindMag() const
{
	return m_bind_mag;
}

void GroupFinder::SetGrpLeader(bool val)
{
	m_is_grp_leader = val;
}

bool GroupFinder::GetGrpLeader() const
{
	return m_is_grp_leader;
}

GroupFinder::~GroupFinder()
{}

void
GroupFinder::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void GroupFinder::MobilityStateUpdated(VelocitySensor::MobilityState from,
								  VelocitySensor::MobilityState to)
{
	NS_LOG_LOGIC("MobilityStateUpdated: " <<
			     VelocitySensor::MobilityStateStr(from) << " -> " <<
				 VelocitySensor::MobilityStateStr(to));
	switch(to)
	{
	case VelocitySensor::VS_ONMOVE:
		m_curMeshMacs.clear();//todo consider removing
		m_curPmipMacs.clear();
		break;
	default:
		break;
	};

	m_curMobilityState = to;//to do: Add more logic here, if required
}

void GroupFinder::GroupBCastReceived(Ptr<Packet> packet, WifiMacHeader const *hdr)
{
	NS_LOG_FUNCTION_NOARGS();
	(void) packet;
	m_curMeshMacs.insert(hdr->GetAddr2());//todo, consider discarding
	std::map<Mac48Address, Ptr<Node> >::iterator itNode = m_meshMacToNode.find(hdr->GetAddr2());
	if (itNode != m_meshMacToNode.end())
	{
		std::map<Ptr<Node>, Mac48Address >::iterator itMac = m_pmipNodeToMac.find(itNode->second);
		if (itMac != m_pmipNodeToMac.end() )
			m_curPmipMacs.insert(itMac->second);
	}
}

void GroupFinder::Report()
{
	m_updateEvent = Simulator::Schedule(Time(Seconds(m_reportInterval)), &GroupFinder::Report, this);
    std::stringstream out("");
    for (std::set<Mac48Address>::iterator it(m_curMeshMacs.begin()); it != m_curMeshMacs.end(); it++)
    	out << "  " << *it << "\n";
    NS_LOG_LOGIC("Team Members for:" << (GetNode() ? GetNode()->GetId() : uint32_t(123456)) << " :\n" << out.str());
    if (GetNode())
        NS_LOG_LOGIC("POSITION FOR NODE(" << GetNode()->GetId() << ")" << GetNode()->GetObject<MobilityModel> ()->GetPosition ());
}

void GroupFinder::StartApplication (void)
{
	m_updateEvent = Simulator::Schedule(Time(Seconds(m_reportInterval)), &GroupFinder::Report, this);
}

void GroupFinder::StopApplication (void)
{
	m_updateEvent.Cancel();
}


} // Namespace ns3
