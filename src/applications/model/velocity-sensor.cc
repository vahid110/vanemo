#include "velocity-sensor.h"
#include "ns3/log.h"

namespace ns3 {


NS_LOG_COMPONENT_DEFINE ("VelocitySensor");

NS_OBJECT_ENSURE_REGISTERED (VelocitySensor);

TypeId
VelocitySensor::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::VelocitySensor")
    .SetParent<Application> ()
    .AddConstructor<VelocitySensor> ();
  return tid;
}

VelocitySensor::VelocitySensor()
{}

void
VelocitySensor::RegisterVelocityCB(state_change_notifier_t cb)
{
	m_stateChangeNotifiers.push_back(cb);
}

Vector VelocitySensor::GetCurVelocity()
{
	return m_curState.m_velocity;
}

VelocitySensor::MobilityState VelocitySensor::GetCurState()
{
	return m_curState.m_mobilityState;
}

void VelocitySensor::SetUpdateInterval(const Time &val)
{
	m_updateInterval = val;
}

Vector VelocitySensor::GetVelocity()
{
	Ptr<Node> node = GetNode ();
	NS_ASSERT_MSG(node, "No node for this application instance.");
	Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
	NS_ASSERT_MSG(mobility, "No mobility for node:" << node->GetId());
	return mobility->GetVelocity ();
}

bool operator==(const Vector &a, const Vector &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

void VelocitySensor::UpdateState()
{
	const Vector &v = m_curState.m_velocity;
	const MobilityState vs = m_curState.m_mobilityState;
	const double &f = m_curState.m_factor;
	const Time &t = m_curState.m_timeStamp;
	(void)t;
	(void)v;

	State s(GetVelocity());

	s.m_mobilityState = vs;

	if (vs == VS_UNKNOWN)
	{
		if (s.m_factor == 0)
			s.m_mobilityState = VS_STOPPED;
		if (s.m_factor > 0)
			s.m_mobilityState = VS_ACCELARATING;
	}
	else if (vs == VS_STOPPED)
	{
		if (f < s.m_factor)
			s.m_mobilityState = VS_ACCELARATING ;
	}
	else if (vs == VS_ACCELARATING)
	{
		if (f != s.m_factor)
			s.m_mobilityState = (f < s.m_factor ? VS_ONMOVE : VS_DESCELERATING);
	}
	else if (vs == VS_ONMOVE)
	{
		if (f != s.m_factor)
			s.m_mobilityState = (f > s.m_factor ? VS_DESCELERATING : VS_ACCELARATING);
	}
	else if (vs == VS_DESCELERATING)
	{
		if (f != s.m_factor)
		{
			if (f < s.m_factor)
				s.m_mobilityState = VS_ACCELARATING;
		}
	}

	if (s.m_factor == 0)
		s.m_mobilityState = VS_STOPPED;

	if (s.m_mobilityState == VS_UNKNOWN)
	{
		NS_LOG_ERROR("Still VS_UNKNOWN!");
	}

	MobilityState from = m_curState.m_mobilityState;
	MobilityState to = s.m_mobilityState;

	if (m_curState.m_mobilityState != s.m_mobilityState)
		NS_LOG_UNCOND("Changing state from " << m_curState.StateStr() << " to " << s.StateStr() <<".");
	else
		NS_LOG_UNCOND("state Unchanged from " << m_curState.StateStr() << " to " << s.StateStr() <<".");

	m_curState = s;

	//notify
	if (!m_stateChangeNotifiers.empty() && from != to)
	{
		std::list<state_change_notifier_t>::iterator it(
				m_stateChangeNotifiers.begin());
		for (; it != m_stateChangeNotifiers.end(); it++)
		{
			state_change_notifier_t cb = *it;
			cb(from, to);
		}
	}

	m_updateEvent = Simulator::Schedule (m_updateInterval,
	                                     &VelocitySensor::UpdateState,
	                                     this);
}

void
VelocitySensor::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}
void VelocitySensor::StartApplication (void)
{
    m_updateEvent = Simulator::ScheduleNow (&VelocitySensor::UpdateState, this);
}

void VelocitySensor::StopApplication (void)
{
	m_updateEvent.Cancel();
}

}//namespace
