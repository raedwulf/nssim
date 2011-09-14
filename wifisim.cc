/**
 * Copyright (C) 2011 The University of York
 * Author(s):
 *   Tai Chi Minh Ralph Eastwood <tcmreastwood@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
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
 * \brief Wireless simulation example.
 * \author Tai Chi Minh Ralph Eastwood
 * \author University of York
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdio>
#include <termios.h>

NS_LOG_COMPONENT_DEFINE("WifiSimulationExample");

#include "PlayerNSDCommunication.h"

using namespace ns3;

// Global variables
static bool verbose = false;
static double tickInterval = 0.01;

class NS3Communication : public PlayerNSDCommunication
{
   public:
      NS3Communication(int maxClients, bool verbose)
         : PlayerNSDCommunication(verbose)
      {
         clientSocket.reserve(maxClients);
         index = 0;
         // Fixed properties for testing.
         for (int i = 0; i < maxClients; i++)
         {
            std::stringstream ss;
            std::stringstream ssv;
            ss << "__node" << (i + 1) << ".index";
            ssv << (i + 1);
            SetProperty(ss.str().c_str(), ssv.str().c_str());
         }
      }

      void ReceivePacket(Ptr<Socket> socket)
      {
         Address from;
         Ptr<Packet> packet;
         // This receives a packet from the socket and sends it straight
         // out to the playernsd daemon.
         while(packet = socket->RecvFrom(from))
         {
            int size = packet->GetSize();
            uint8_t *buffer = new uint8_t[size];
            packet->CopyData(buffer, size);
            writeRecv(addressID[from], socketID[socket], size, (char *)buffer);
            delete[] buffer;
         }
      }

      void AddSocket(Ptr<Socket> socket, InetSocketAddress socketAddress)
      {
         index++;
         socketID[socket] = index;
         addressID[socketAddress] = index;
         clientSocket.push_back(new ClientSocket(socket, socketAddress));
      }

      void Tick(Time pktInterval)
      {
         processInput();
         Simulator::Schedule(pktInterval, &NS3Communication::Tick, this, pktInterval);
      }

      void AdvancePosition(Ptr<Node> node, std::string name)
      {
         std::istringstream iss(GetProperty((name + ".position").c_str()));
         Vector pos;
         iss >> pos.x >> pos.y;
         pos.z = 0.0;

         Vector oldPos = node->GetObject<MobilityModel>()->GetPosition();

         if (oldPos.x != pos.x || oldPos.y != pos.y || oldPos.z != pos.z)
         {
            node->GetObject<MobilityModel>()->SetPosition(pos);
            if (verbose)
               std::cerr << "AdvancePosition: x="<< pos.x << " y=" << pos.y << " a=" << pos.z << std::endl;
         }
         Simulator::Schedule(Seconds(tickInterval), &NS3Communication::AdvancePosition, this, node, name);
      }

   protected:
      void sendMessage(uint32_t from, uint32_t to, uint32_t size, const char *buffer)
      {
         if (to) // Single destination.
            clientSocket[from-1]->socket->SendTo((const uint8_t *)buffer, size, 0, clientSocket[to-1]->address);
         else // Broadcast.
            clientSocket[from-1]->socket->SendTo((const uint8_t *)buffer, size, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), 12323));
      }

      void closeSocket(uint32_t socket)
      {
         clientSocket[socket-1]->socket->Close();
      }

   private:
      /**
       * ClientSocket class for internal use.
       */
      struct ClientSocket
      {
      public:
         ClientSocket(Ptr<Socket> s, Address a)
         {
            socket = s;
            address = a;
         }
         Ptr<Socket> socket;
         Address address;
      };

      // Maps to link IDs together.
      int index;
      std::map<Ptr<Socket>, int> socketID;
      std::map<Address, int> addressID;
      std::vector<ClientSocket *> clientSocket;
};

int main(int argc, char *argv[])
{
   std::string phyMode("DsssRate1Mbps");
   int packetSize = 1000;
   uint32_t numPackets = 10;
   uint32_t maxClients = 10;
   double txp = 7.5;

   // Parsing command line
   CommandLine cmd;
   cmd.AddValue("maxClients", "Maximum number of clients", maxClients);
   cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
   cmd.AddValue("txp", "Transmission power", txp);
   cmd.AddValue("numPackets", "number of packets generated", numPackets);
   cmd.AddValue("tickInterval", "interval of update between playernsd & simulation in seconds", tickInterval);
   cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);
   cmd.Parse(argc, argv);

   // Create the playernsd communication class
   NS3Communication nsdCom(verbose, maxClients);

   // Think in realtime
   GlobalValue::Bind("SimulatorImplementationType",
                     StringValue("ns3::RealtimeSimulatorImpl"));
   GlobalValue::Bind("ChecksumEnabled", BooleanValue (true));

   // Convert to time object
   Time interPacketInterval = Seconds(tickInterval);

   // disable fragmentation for frames below 2200 bytes
   Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue("2200"));
   // turn off RTS/CTS for frames below 2200 bytes
   Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("2200"));
   // Fix non-unicast data rate to be the same as that of unicast
   Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue(phyMode));

   NodeContainer c;
   c.Create(maxClients);

   // The below set of helpers will help us to put together the wifi NICs we want
   WifiHelper wifi;
   if(verbose)
   {
      ns3::LogComponentEnable("WifiSimulationExample", LOG_DEBUG);
      wifi.EnableLogComponents();   // Turn on all Wifi logging
   }
   else
   {
      ns3::LogComponentEnable("WifiSimulationExample", LOG_INFO);
   }
   NS_LOG(LOG_INFO, "Starting wifi simulator using ns3.");
   wifi.SetStandard(WIFI_PHY_STANDARD_80211b);

   YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default();
   wifiPhy.Set("TxPowerStart", DoubleValue(txp));
   wifiPhy.Set("TxPowerEnd", DoubleValue(txp));
   // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
   wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

   YansWifiChannelHelper wifiChannel;
   wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
   // The below FixedRssLossModel will cause the rss to be fixed regardless
   // of the distance between the two stations, and the transmit power
   //wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel","Rss",DoubleValue(rss));
   wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
   wifiPhy.SetChannel(wifiChannel.Create());

   // Add a non-QoS upper mac, and disable rate control
   NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();
   wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue(phyMode),
                                "ControlMode",StringValue(phyMode));
   // Set it to adhoc mode
   wifiMac.SetType("ns3::AdhocWifiMac");
   NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c);

   InternetStackHelper internet;
   internet.Install(c);

   Ipv4AddressHelper ipv4;
   NS_LOG_INFO("Assign IP Addresses.");
   ipv4.SetBase("10.1.1.0", "255.255.255.0");
   Ipv4InterfaceContainer ifc = ipv4.Assign(devices);

   // List of positions for nodes
   Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

   TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
   for(int i = 0; i < maxClients; i++)
   {
      InetSocketAddress socketAddress = InetSocketAddress(ifc.GetAddress(i, 0), 12323);
      Ptr<Socket> socket = Socket::CreateSocket(c.Get(i), tid);
      socket->SetAllowBroadcast(true);
      socket->Bind(socketAddress);
      // The NSD communication handler receives the packet
      socket->SetRecvCallback(MakeCallback(&NS3Communication::ReceivePacket, &nsdCom));
      // Add socket to the NSD communication handler
      nsdCom.AddSocket(socket, socketAddress);
      // Positions for nodes
      positionAlloc->Add(Vector(0.0, 0.0, 0.0));
      // Allow the nodes to move around a bit.
      std::stringstream nn;
      nn << "__node" << (i + 1); // accessed from the daemon using the index + 1
      Simulator::Schedule(Seconds(0.15), &NS3Communication::AdvancePosition, &nsdCom, c.Get(i), nn.str());
   }

   // Note that with FixedRssLossModel, the positions below are not
   // used for received signal strength.
   MobilityHelper mobility;
   mobility.SetPositionAllocator(positionAlloc);
   mobility.Install(c);

   // Tracing
   wifiPhy.EnablePcap("wifisim", devices);

   Simulator::Schedule(Seconds(0.1), &NS3Communication::Tick, &nsdCom, interPacketInterval);

   Simulator::Run();
   Simulator::Destroy();

   return 0;
}

