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
VelocitySensor::RegisterVelocityCB(
		Callback<void,
		         VelocitySensor::VelocityState,
				 VelocitySensor::VelocityState> cb)
{
	m_state_change_notifier = cb;
}

Vector
VelocitySensor::GetVelocity()
{
	Ptr<Node> node = GetNode ();
	NS_ASSERT_MSG(node, "No node for this application instance.");
	Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
	NS_ASSERT_MSG(mobility, "No mobility for node:" << node->GetId());
	return mobility->GetVelocity ();
}

void VelocitySensor::StoreVelocity(const Vector&)
{}

void VelocitySensor::StoreVelocity()
{}

bool operator==(const Vector &a, const Vector &b)
{
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

void VelocitySensor::UpdateState()
{
	const Vector &v = m_cur_state.m_velocity;
	const VelocityState vs = m_cur_state.m_v_state;
	const double &f = m_cur_state.m_factor;
	const Time &t = m_cur_state.m_timestamp;
	(void)t;
	State s(GetVelocity());

	if (vs == VS_UNKNOWN)
	{
		Vector paused = Vector(0, 0, 0);
		s.m_v_state = (v == paused ? VS_STOPPED : VS_ACCELARATING);
	}
	else if (vs == VS_STOPPED && f < s.m_factor)
	{
		s.m_v_state = VS_ACCELARATING;
	}
	else if (vs == VS_ACCELARATING && f < s.m_factor)
	{
		s.m_v_state = VS_ONMOVE;
	}
	else if (vs == VS_ACCELARATING && f > s.m_factor)
	{
		s.m_v_state = VS_DESCELERATING;
	}
	else if (vs == VS_ONMOVE && f > s.m_factor)
	{
		s.m_v_state = VS_DESCELERATING;
	}
	else if (vs == VS_DESCELERATING && f > s.m_factor)
	{
		Vector paused = Vector(0, 0, 0);
		if(v == paused)
			s.m_v_state = VS_STOPPED;
	}

	if (s.m_v_state== VS_UNKNOWN)
	{
		NS_LOG_ERROR("Still VS_UNKNOWN!");
	}

	if (!m_state_change_notifier.IsNull() &&
			m_cur_state.m_v_state != s.m_v_state )
	{
		m_state_change_notifier(m_cur_state.m_v_state, s.m_v_state);
	}

	m_cur_state = s;
}

void
VelocitySensor::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

}//namespace
