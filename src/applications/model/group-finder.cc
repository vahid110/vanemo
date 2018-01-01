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
#include "group-finder.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("GroupFinder");

NS_OBJECT_ENSURE_REGISTERED (GroupFinder);

bool GroupFinder::m_enable = true;
std::map<Mac48Address, Ptr<Node> > GroupFinder::m_mac_to_node;

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
GroupFinder::AddMacNodeMap(const Mac48Address &mac, Ptr<Node> node)
{
    m_mac_to_node[mac] = node;
}

Ptr<Node>
GroupFinder::GetNodebyMac(const Mac48Address &mac)
{
    return m_mac_to_node[mac];
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

void
GroupFinder::SetGroup(NetDeviceContainer c)
{
    NS_LOG_DEBUG(this << "GroupFinder::SetGroup: " << c.GetN());
    m_devices = c;
}

NetDeviceContainer
GroupFinder::GetGroup() const
{
    return m_devices;
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

void 
GroupFinder::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
}

void 
GroupFinder::StopApplication ()
{
  NS_LOG_FUNCTION (this);
}

} // Namespace ns3
