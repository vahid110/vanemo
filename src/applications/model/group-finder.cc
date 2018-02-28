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
std::map<Mac48Address, Ptr<Node> > GroupFinder::m_pmipMacToNode;

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
{
	Report();
}

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
}

Ptr<Node>
GroupFinder::GetNodeByPmipMac(const Mac48Address &mac)
{
	std::map<Mac48Address, Ptr<Node> >::iterator it = m_pmipMacToNode.find(mac);
	if (it == m_pmipMacToNode.end())
		return 0;
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
    return  m_curGrpMacs;
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
		m_curGrpMacs.clear();
		break;
	default:
		break;
	};

	m_curMobilityState = to;//to do: Add more logic here, if required
}

void GroupFinder::GroupBCastReceived(Ptr<Packet> packet, WifiMacHeader const *hdr)
{
	NS_LOG_FUNCTION_NOARGS();
//	NS_LOG_LOGIC(hdr->GetAddr2());
	m_curGrpMacs.insert(hdr->GetAddr2());
}

void GroupFinder::Report()
{

    Simulator::Schedule(Time(Seconds(m_reportInterval)), &GroupFinder::Report, this);
    std::stringstream out("");
    for (std::set<Mac48Address>::iterator it(m_curGrpMacs.begin()); it != m_curGrpMacs.end(); it++)
    	out << "  " << *it << "\n";
    NS_LOG_LOGIC("Team Members for:" << (GetNode() ? GetNode()->GetId() : uint32_t(123456)) << " :\n" << out.str());
}


} // Namespace ns3
