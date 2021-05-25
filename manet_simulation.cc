#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aodv-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/applications-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/default-deleter.h"
#include "ns3/point-to-point-module.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include "string"
#include "vector"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("manet-routing");

class RoutingExperiment
{
public:
  RoutingExperiment ();
  void Run ();
  std::string CommandSetup (int argc, char **argv);

private:
  Ptr<Socket> SetupPacketReceive (Ipv4Address addr, Ptr<Node> node);
  void ReceivePacket (Ptr<Socket> socket);
  void CheckThroughput ();

  uint32_t port;
  uint32_t bytesTotal;
  uint32_t packetsReceived;

  std::string m_CSVfileName;
  std::string m_protocolName;
  bool m_traceMobility;
  uint32_t m_protocol;
};

RoutingExperiment::RoutingExperiment ()
  : port (9),
    bytesTotal (0),
    packetsReceived (0),
    m_CSVfileName ("/home/vlad/jupyter_notebook_project_dir/p2p_manet.output.csv"),
    m_traceMobility (false),
    m_protocol (3) // DSDV
{
}

static inline std::string
PrintReceivedPacket (Ptr<Socket> socket, Ptr<Packet> packet, Address senderAddress)
{
  std::ostringstream oss;

  oss << Simulator::Now ().GetSeconds () << " " << socket->GetNode ()->GetId ();

  if (InetSocketAddress::IsMatchingType (senderAddress))
    {
      InetSocketAddress addr = InetSocketAddress::ConvertFrom (senderAddress);
      oss << " received one packet from " << addr.GetIpv4 ();
    }
  else
    {
      oss << " received one packet!";
    }
  return oss.str ();
}

void
RoutingExperiment::ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Address senderAddress;
  while ((packet = socket->RecvFrom (senderAddress)))
    {
      bytesTotal += packet->GetSize ();
      packetsReceived += 1;
      NS_LOG_UNCOND (PrintReceivedPacket (socket, packet, senderAddress));
    }
}

void
RoutingExperiment::CheckThroughput ()
{
  double kbs = (bytesTotal * 8.0) / 1024;
  bytesTotal = 0;

  std::ofstream out (m_CSVfileName.c_str (), std::ios::out);

  out << (Simulator::Now ()).GetSeconds () << ","
      << kbs << ","
      << packetsReceived << ","
      << m_protocolName << ","
      << std::endl;


  out.close ();
  packetsReceived = 0;
  Simulator::Schedule (Seconds (1.0), &RoutingExperiment::CheckThroughput, this);
}

Ptr<Socket>
RoutingExperiment::SetupPacketReceive (Ipv4Address addr, Ptr<Node> node)
{
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> sink = Socket::CreateSocket (node, tid);
  InetSocketAddress local = InetSocketAddress (addr, port);
  sink->Bind (local);
  sink->SetRecvCallback (MakeCallback (&RoutingExperiment::ReceivePacket, this));

  return sink;
}

std::string
RoutingExperiment::CommandSetup (int argc, char **argv)
{
  CommandLine cmd;
  cmd.AddValue ("CSVfileName", "The name of the CSV output file name", m_CSVfileName);
  cmd.AddValue ("traceMobility", "Enable mobility tracing", m_traceMobility);
  cmd.AddValue ("protocol", "1=OLSR;2=AODV;3=DSDV;4=DSR", m_protocol);
  cmd.Parse (argc, argv);
  return m_CSVfileName;
}

int main (int argc, char *argv[])
{
  RoutingExperiment experiment;
  std::string CSVfileName = experiment.CommandSetup (argc,argv);

  std::ofstream out (CSVfileName.c_str ());
  out << "SimulationSecond," <<
  "ReceiveRate," <<
  "PacketsReceived," <<
  "NumberOfSinks," <<
  "RoutingProtocol," <<
  "TransmissionPower" <<
  std::endl;
  out.close ();

  experiment.Run ();
}

void
RoutingExperiment::Run ()
{
  Packet::EnablePrinting ();

  std::string rate ("2048bps");
  std::string phyMode ("DsssRate11Mbps");

  std::string txt_name ("/home/vlad/jupyter_notebook_project_dir/my_network_with_nodes32.txt");
  std::string tr_name ("/home/vlad/jupyter_notebook_project_dir/p2p_manet32");
  m_protocolName = "DSDV";

  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("64"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
  
  uint16_t port = 9;
  double txp = 4.6875;
	
  WifiHelper wifi;

  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  YansWifiPhyHelper wifiPhy2 =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel2;
  wifiChannel2.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel2.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy2.SetChannel (wifiChannel2.Create ());

  // Add a mac and disable rate control
  
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));

  
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));

  wifiPhy2.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy2.Set ("TxPowerEnd", DoubleValue (txp));

  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  WifiMacHelper wifiMac2;
  wifiMac2.SetType ("ns3::AdhocWifiMac");


	ifstream in;
	in.open(txt_name);

  if (in.is_open())
  {
    cout << "File open" << endl;
  }

  else
  {
    cout << "File open is failed" << endl;
  }
  int nodesNumber;
  int dronsNumber;
  in >> nodesNumber >> dronsNumber;
  NodeContainer adhocNodes;
  adhocNodes.Create(nodesNumber);
  NodeContainer dronContainer;
  dronContainer.Create(dronsNumber);

  // Кординаты узлов и их принадлежность игроку
  double gridScalingFactor = 100.0;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject <ListPositionAllocator>();  
  double x, y;
  int player_id;
  int playersNodes[nodesNumber];
  for (int node = 0; node < nodesNumber; node++)
  {
    in >> x >> y >> player_id;		  
    positionAlloc ->Add(Vector(x*gridScalingFactor, y*gridScalingFactor, 0));
    playersNodes[node] = player_id;
  }

  for (int dron = 0; dron < dronsNumber; dron++)
  {
    in >> x >> y;		  
    positionAlloc ->Add(Vector(x*gridScalingFactor, y*gridScalingFactor, 0));
  }

  MobilityHelper mobilityAdhoc;
  mobilityAdhoc.SetPositionAllocator(positionAlloc);
  mobilityAdhoc.SetMobilityModel("ns3::ConstantPositionMobilityModel"); 
  mobilityAdhoc.Install (adhocNodes);
  mobilityAdhoc.Install (dronContainer);


  NodeContainer n_container;
  NodeContainer n_container2;
  int container_size = 0;
  int container2_size = 0;
  for (int i = 0; i < nodesNumber; i++)
  {     
      if (playersNodes[i] == 1)
      {
        n_container.Add(adhocNodes.Get (i));
        container_size++;   
      }
      if (playersNodes[i] == 2)
      {
        n_container2.Add(adhocNodes.Get (i));
        container2_size++; 
      }  
  }

  for (int i = 0; i < dronsNumber; i++)
  {
    n_container.Add(dronContainer.Get (i));
    n_container2.Add(dronContainer.Get (i));
  }

  Ipv4ListRoutingHelper list;
  InternetStackHelper internet; 
  DsdvHelper dsdv;
  list.Add (dsdv, 100);
  internet.SetRoutingHelper (list);
  internet.Install(adhocNodes);
  internet.Install(dronContainer);

  Ipv4AddressHelper address;
  Ipv4InterfaceContainer adhocInterfaces;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  wifi.SetStandard (WIFI_PHY_STANDARD_80211b);
  NetDeviceContainer adhocDevices = wifi.Install (wifiPhy, wifiMac, n_container);
  adhocInterfaces.Add(address.Assign(adhocDevices));

  NetDeviceContainer adhocDevices2 = wifi.Install (wifiPhy2, wifiMac2, n_container2);
  adhocInterfaces.Add(address.Assign(adhocDevices2));


  in.close();

  cout << "PACKETS INITIALIZE\n";

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  OnOffHelper onoff ("ns3::UdpSocketFactory",Address ());
  onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1.0]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"));
  double startTime = 25.0;
  double shift = 0.7;
  

  for (int i = 0; i < container_size; i++)
  {
    for (int j = 0; j < container2_size; j++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (container_size+container2_size-1), dronContainer.Get (0));

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (container_size+container2_size-1), port));
      onoff.SetAttribute ("Remote", remoteAddress);

      ApplicationContainer temp = onoff.Install (n_container2.Get (j));
      
      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      temp.Start (Seconds (var->GetValue (startTime,startTime+0.03)));
      temp.Stop (Seconds (startTime+shift)); 
    }
    startTime += shift;
  }

  for (int i = 0; i < container_size; i++)
  {
    for (int j = 0; j < container2_size; j++)
    {
      Ptr<Socket> sink = SetupPacketReceive (adhocInterfaces.GetAddress (i), n_container.Get (i));

      AddressValue remoteAddress (InetSocketAddress (adhocInterfaces.GetAddress (i), port));
      onoff.SetAttribute ("Remote", remoteAddress);

      ApplicationContainer temp = onoff.Install (dronContainer.Get (0));
      Ptr<UniformRandomVariable> var = CreateObject<UniformRandomVariable> ();
      temp.Start (Seconds (var->GetValue (startTime,startTime+0.03)));
      temp.Stop (Seconds (startTime+shift)); 
    }
    startTime += shift;
  }  

  cout << "FLOWMONITOR INITIALIZE\n";
  Ptr<FlowMonitor> flowmon;
  FlowMonitorHelper flowmonHelper;
  flowmon = flowmonHelper.InstallAll ();

  NS_LOG_INFO ("Run Simulation.");

  CheckThroughput ();

  Simulator::Stop (Seconds (startTime));
  Simulator::Run ();

  flowmon->SerializeToXmlFile ((tr_name + ".flowmon").c_str(), true, true);

  Simulator::Destroy ();

}
