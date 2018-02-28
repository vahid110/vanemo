#include "ns3/velocity-sensor.h"
#include "velocity-sensor-helper.h"
#include "ns3/log.h"
#include "ns3/names.h"

namespace ns3 {

VelocitySensorHelper::VelocitySensorHelper (
		const Time &updateInterval,
		VelocitySensor::UpdateMode updateMethod)
	: m_updateInterval(updateInterval)
	, m_updateMode(updateMethod)
{
	m_factory.SetTypeId (VelocitySensor::GetTypeId ());
}

void
VelocitySensorHelper::SetAttribute (
  std::string name,
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer VelocitySensorHelper::Install (Ptr<Node> node) const
{
	return ApplicationContainer (InstallPriv (node));
}
ApplicationContainer VelocitySensorHelper::Install (std::string nodeName) const
{
	  Ptr<Node> node = Names::Find<Node> (nodeName);
	  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer VelocitySensorHelper::Install (NodeContainer c) const
{
	  ApplicationContainer apps;
	  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
	    {
	      apps.Add (InstallPriv (*i));
	    }

	  return apps;
}

Ptr<Application> VelocitySensorHelper::InstallPriv (Ptr<Node> node) const
{
	  Ptr<VelocitySensor> app = m_factory.Create<VelocitySensor> ();
	  app->SetUpdateInterval(m_updateInterval);
	  app->SetUpdateMode(m_updateMode);
	  node->AddApplication (app);
	  return app;
}
}//namespace
