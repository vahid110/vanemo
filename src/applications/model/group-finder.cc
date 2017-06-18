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

TypeId
GroupFinder::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::GroupFinder")
    .SetParent<Application> ()
    .AddConstructor<GroupFinder> ();
  return tid;
}

GroupFinder::GroupFinder ()
{

}

void
GroupFinder::SetGroup(NetDeviceContainer c)
{
	NS_LOG_UNCOND(this << "GroupFinder::SetGroup: " << c.GetN());
	m_devices = c;
}

NetDeviceContainer
GroupFinder::GetGroup() const
{
	return m_devices;
}

GroupFinder::~GroupFinder()
{

}

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
