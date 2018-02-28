#ifndef VELOCITY_SENSOR_HELPER_H
#define VELOCITY_SENSOR_HELPER_H

#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

namespace ns3 {

/**
 * \ingroup groupfinder
 * \brief Create an application which sends a UDP packet and waits for an echo of this packet
 */
class VelocitySensorHelper
{
public:
  /**
   * Create VelocitySensorHelper which will make life easier for people trying
   * to set up simulations with echos.
   *
   * \param ip The IPv6 address of the remote udp echo server
   * \param port The port number of the remote udp echo server
   */
  VelocitySensorHelper (
		  const Time &updateInterval,
		  VelocitySensor::UpdateMode updateMethod =
				VelocitySensor::MSU_ALL);
  void SetAttribute (std::string name,const AttributeValue &value);
  /**
   * Create a udp echo client application on the specified node.  The Node
   * is provided as a Ptr<Node>.
   *
   * \param node The Ptr<Node> on which to create the UdpEchoClientApplication.
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (Ptr<Node> node) const;

  /**
   * Create a udp echo client application on the specified node.  The Node
   * is provided as a string name of a Node that has been previously 
   * associated using the Object Name Service.
   *
   * \param nodeName The name of the node on which to create the UdpEchoClientApplication
   *
   * \returns An ApplicationContainer that holds a Ptr<Application> to the 
   *          application created
   */
  ApplicationContainer Install (std::string nodeName) const;

  /**
   * \param c the nodes
   *
   * Create one udp echo client application on each of the input nodes
   *
   * \returns the applications created, one application per input node.
   */
  ApplicationContainer Install (NodeContainer c) const;

private:
  /**
   * Install an ns3::UdpEchoClient on the node configured with all the
   * attributes set with SetAttribute.
   *
   * \param node The node on which an UdpEchoClient will be installed.
   * \returns Ptr to the application installed.
   */
  Ptr<Application> InstallPriv (Ptr<Node> node) const;
//  void LinktoGroupFinder(Ptr<VelocitySensor> app, Ptr<Node> node);
  ObjectFactory m_factory; //!< Object factory.
  Time m_updateInterval;
  VelocitySensor::UpdateMode m_updateMode;

};

} // namespace ns3

#endif /* VELOCITY_SENSOR_HELPER_H */
