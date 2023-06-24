/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.0 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 /*  ./ns3 run scratch/wifi-range-test --nNodes=10 --dis=200 --txp=true"      //for latest version having cmake build 
 *   ./waf --run scratch/wifi-range-test --nNodes=10 --dis=200 --txp=true"   //For waf build (Old versions)
 */
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/packet-sink.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiRangeTest");
int 
main (int argc, char *argv[])
{
  uint32_t nNodes = 5;
  std::string rate = "512bps"; 
  double duration = 100; 
  bool tPower = false;
  double txp = 7;
  double dis = 350;
  double SimStartTime = 5;
  std::string phyMode ("DsssRate11Mbps");
 
    CommandLine cmd;
    cmd.AddValue ("nNodes", "Number of nodes in the simulation", nNodes);
    cmd.AddValue ("tPower", "Enable or Disable Transmission Power", tPower);
    cmd.AddValue ("txp", "Transmission Power", txp);
    cmd.AddValue ("dis", "Increase or decrease distance value to check the impect", dis);
    
    cmd.Parse (argc, argv);
 
    
  Config::SetDefault  ("ns3::OnOffApplication::PacketSize",StringValue ("512"));
  Config::SetDefault ("ns3::OnOffApplication::DataRate",  StringValue (rate));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",StringValue (phyMode));
  
 NodeContainer Nodes;
  Nodes.Create (nNodes);


  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211b);
  //wifi.SetStandard (WIFI_PHY_STANDARD_80211b);   //For Old versions

  YansWifiPhyHelper wifiPhy;  
  //YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();     //For old versions
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
if(tPower)
 {
  wifiPhy.Set ("TxPowerStart",DoubleValue (txp));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (txp));
}
  wifiMac.SetType ("ns3::AdhocWifiMac");
  NetDeviceContainer Devices = wifi.Install (wifiPhy, wifiMac, Nodes);



    MobilityHelper mobility1;
	Ptr<ListPositionAllocator> positionAlloc1 = CreateObject<ListPositionAllocator> ();
  	for (uint16_t i = 0; i < nNodes; i++)
    	{
      	positionAlloc1->Add (Vector (dis*i, 0.0, 00));   //Node node to distance
    	}
	mobility1.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility1.SetPositionAllocator (positionAlloc1);
	mobility1.Install (Nodes);
 
    OlsrHelper olsr;
    Ipv4ListRoutingHelper list;
    list.Add (olsr, 10);

   InternetStackHelper stack;
   stack.SetRoutingHelper (list);
   stack.Install (Nodes);
  

  //Asigning IPv4 Address to all devices in the network
  Ipv4AddressHelper address;
  address.SetBase ("192.168.1.0", "255.255.255.0");
  Ipv4InterfaceContainer Interfaces;
  Interfaces = address.Assign (Devices);
 

     uint16_t port = 9;
    /* Setting applications */
     OnOffHelper onoff("ns3::UdpSocketFactory",InetSocketAddress(Interfaces.GetAddress(0), port)); //Reciever Nodes  (First Node)
     onoff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"));
     onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.1]"));
     
     ApplicationContainer apps = onoff.Install(Nodes.Get(4)); //Sender Node (Last Node)
     apps.Start(Seconds(SimStartTime));
     apps.Stop(Seconds(duration));
    

          //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  	  FlowMonitorHelper flowmon;
  	  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

          Simulator::Stop (Seconds (duration + 1));
          AnimationInterface anim ("WifiRangeTest.xml");
          anim.SetMaxPktsPerTraceFile(99999999999999);
          Simulator::Run ();

	Ptr < Ipv4FlowClassifier > classifier = DynamicCast < Ipv4FlowClassifier >(flowmon.GetClassifier());
	std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();

	double Delaysum = 0;
	uint64_t txPacketsum = 0;
	uint64_t rxPacketsum = 0;
	uint32_t txPacket = 0;
	uint32_t rxPacket = 0;
    uint32_t PacketLoss = 0;
    uint64_t txBytessum = 0; 
	uint64_t rxBytessum = 0;
    double delay;
    double throughput = 0;
    int hop_count = 0;

	for (std::map < FlowId, FlowMonitor::FlowStats > ::const_iterator iter = stats.begin(); iter != stats.end(); ++iter) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);
                NS_LOG_UNCOND("*****************************************");
		NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          
                  txPacket = iter->second.txPackets;
                  rxPacket = iter->second.rxPackets;
                  PacketLoss = txPacket - rxPacket;
                  delay = iter->second.delaySum.GetMilliSeconds();
                  
          std::cout << "  Tx Packets: " << iter->second.txPackets << "\n";  
          std::cout << "  Rx Packets: " << iter->second.rxPackets << "\n";
          std::cout << "  Packet Loss: " << PacketLoss << "\n";
  	  std::cout << "  Tx Bytes:   " << iter->second.txBytes << "\n";
          std::cout << "  Rx Bytes:   " << iter->second.rxBytes << "\n";
          std::cout << "  Throughput: " << iter->second.rxBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
          NS_LOG_UNCOND("  Delay for current flow ID: " << delay / txPacket << " ms");
          std::cout << "   PDR for current flow ID : " << ((rxPacket *100) / txPacket) << "%" << "\n";
                              
		txPacketsum += iter->second.txPackets;
		rxPacketsum += iter->second.rxPackets;
		txBytessum += iter->second.txBytes;
		rxBytessum += iter->second.rxBytes;
		Delaysum += iter->second.delaySum.GetMilliSeconds();


           if (iter ->second.rxPackets > 0)
           {
            hop_count += iter->second.timesForwarded / iter->second.rxPackets + 1;  
             NS_LOG_UNCOND("Total hop count from source to destination "<< hop_count);
           }     

	}
      
        NS_LOG_UNCOND("***********Sum of Results*************");
	throughput = rxBytessum * 8 / (duration * 1000000.0); //Mbit/s  
	NS_LOG_UNCOND("Sent Packets = " << txPacketsum);
	NS_LOG_UNCOND("Received Packets = " << rxPacketsum);
        NS_LOG_UNCOND("Total Packet Loss = " << (txPacketsum-rxPacketsum));
        NS_LOG_UNCOND("Total Byte Sent = "<<txBytessum);
        NS_LOG_UNCOND("Total Byte Received = "<<rxBytessum);
	NS_LOG_UNCOND("Mean Delay: " << Delaysum / txPacketsum << " ms");
	std::cout << "Mean Throughput = "<<throughput << " Mbit/s" << std::endl;
        std::cout << "Packets Delivery Ratio: " << ((rxPacketsum *100) / txPacketsum) << "%" << "\n";

	Simulator::Destroy();

return 0;
}
