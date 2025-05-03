#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/wifi-module.h"
#include "ns3/wifi-phy.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ComplexTcpSimulation");

uint32_t phyRxDropCount = 0;
AsciiTraceHelper asciiTraceHelper;
Ptr<OutputStreamWrapper> cwndStream;

void
PhyStateTrace(std::string context, Time start, Time duration, WifiPhyState state)
{
    NS_LOG_UNCOND(Simulator::Now() << " " << context << " state: " << state);
}

void
SocketStateTrace(Ptr<Socket> socket,
                 TcpSocketState::TcpCongState_t old,
                 TcpSocketState::TcpCongState_t newState)
{
    NS_LOG_UNCOND(Simulator::Now() << " Socket state: " << newState);
}

void
MyCallback(std::string context, Ptr<const Packet> packet, WifiPhyRxfailureReason reason)
{
    phyRxDropCount++;
}

void
TcpConnectionTrace(Ptr<Socket> socket)
{
    socket->TraceConnectWithoutContext("Connect", MakeCallback(&SocketStateTrace));
}

void
ThroughputMonitor(Ptr<PacketSink> sink, Time lastTime, uint64_t lastTotalRx)
{
    Time now = Simulator::Now();
    double throughput =
        (sink->GetTotalRx() - lastTotalRx) * 8.0 / (now.GetSeconds() - lastTime.GetSeconds()) / 1e6;
    std::cout << now.GetSeconds() << "s: Aggregate Throughput: " << throughput << " Mbps"
              << std::endl;
    Simulator::Schedule(Seconds(1.0), &ThroughputMonitor, sink, now, sink->GetTotalRx());
}

static std::map<uint32_t, Ptr<OutputStreamWrapper>> cwndStreams;

static void
CwndTracer(uint32_t nodeId, uint32_t oldval, uint32_t newval)
{
    auto it = cwndStreams.find(nodeId);
    if (it != cwndStreams.end())
    {
        *(it->second)->GetStream()
            << Simulator::Now().GetSeconds() << " " << newval / 1024 * 1024 << std::endl;
    }
}

static void
TraceCwnd(uint32_t nodeId, Ptr<Socket> socket)
{
    if (socket)
    {
        socket->TraceConnectWithoutContext("CongestionWindow",
                                           MakeBoundCallback(&CwndTracer, nodeId));
    }
}

static void
ScheduleTracing(uint32_t nodeId, Ptr<Application> app)
{
    Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication>(app);
    if (onoff)
    {
        Simulator::Schedule(Seconds(0.001), &TraceCwnd, nodeId, onoff->GetSocket());
    }
}

int
main(int argc, char* argv[])
{
    double simulationTime = 120.0;
    int packet_size = 102;
    double distance = 50.0;

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(3);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    YansWifiChannelHelper channel;
    channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel",
                               "Exponent",
                               DoubleValue(2.7),
                               "ReferenceLoss",
                               DoubleValue(40.0));
    channel.AddPropagationLoss("ns3::NakagamiPropagationLossModel",
                               "m0",
                               DoubleValue(1.0),
                               "m1",
                               DoubleValue(1.0),
                               "m2",
                               DoubleValue(1.0));

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());
    phy.Set("TxPowerStart", DoubleValue(15.0));
    phy.Set("TxPowerEnd", DoubleValue(15.0));
    phy.Set("RxNoiseFigure", DoubleValue(7.0));
    phy.Set("CcaEdThreshold", DoubleValue(-82.0));

    phy.SetErrorRateModel("ns3::YansErrorRateModel");

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate54Mbps"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));
    wifi.SetStandard(WIFI_STANDARD_80211a);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, wifiStaNodes);

    WifiMacHelper apMac;
    apMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, apMac, wifiApNode);

    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
                    MakeCallback(&MyCallback));

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(distance),
                                  "DeltaY",
                                  DoubleValue(0.0),
                                  "GridWidth",
                                  UintegerValue(1),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds",
                              RectangleValue(Rectangle(-30, 30, -30, 30)));
    mobility.Install(wifiStaNodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);

    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(2));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(MilliSeconds(1000)));
    InternetStackHelper stack;
    stack.Install(wifiStaNodes);
    stack.Install(wifiApNode);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterfaces = address.Assign(apDevice);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;
    Address sinkAddress(InetSocketAddress(apInterfaces.GetAddress(0), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
    ApplicationContainer sinkApp = sinkHelper.Install(wifiApNode.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    std::vector<std::string> tcpVariants = {"ns3::TcpNewReno", "ns3::TcpCubic", "ns3::TcpVegas"};

    for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
    {
        Config::Set("/NodeList/" + std::to_string(wifiStaNodes.Get(i)->GetId()) +
                        "/$ns3::TcpL4Protocol/SocketType",
                    TypeIdValue(TypeId::LookupByName(tcpVariants[i % tcpVariants.size()])));

        if (i == 2)
        {
            Config::Set("/NodeList/" + std::to_string(wifiStaNodes.Get(i)->GetId()) +
                            "/$ns3::TcpVegas/Alpha",
                        UintegerValue(2));
            Config::Set("/NodeList/" + std::to_string(wifiStaNodes.Get(i)->GetId()) +
                            "/$ns3::TcpVegas/Beta",
                        UintegerValue(4));
        }

        std::ostringstream oss;
        oss << "cwnd-node-" << wifiStaNodes.Get(i)->GetId() << ".txt";
        cwndStreams[wifiStaNodes.Get(i)->GetId()] = asciiTraceHelper.CreateFileStream(oss.str());

        OnOffHelper client("ns3::TcpSocketFactory", sinkAddress);
        client.SetAttribute("DataRate", DataRateValue(DataRate("20Mbps")));
        client.SetAttribute("PacketSize", UintegerValue(packet_size));
        client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        client.SetAttribute("StartTime", TimeValue(Seconds(1.0 + i * 0.5)));
        client.SetAttribute("StopTime", TimeValue(Seconds(simulationTime)));

        ApplicationContainer clientApp = client.Install(wifiStaNodes.Get(i));

        Ptr<OnOffApplication> onoff = DynamicCast<OnOffApplication>(clientApp.Get(0));
        onoff->TraceConnectWithoutContext("Socket", MakeCallback(&TcpConnectionTrace));

        Simulator::Schedule(Seconds(1.0 + i * 0.5 + 0.1),
                            &ScheduleTracing,
                            wifiStaNodes.Get(i)->GetId(),
                            clientApp.Get(0));
    }

    Simulator::Schedule(Seconds(1.1),
                        &ThroughputMonitor,
                        DynamicCast<PacketSink>(sinkApp.Get(0)),
                        Seconds(0.0),
                        0);

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime + 1));
    Simulator::Run();

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        std::cout << "\nFlow " << flow.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Tx packets:   " << flow.second.txBytes / packet_size << "\n";
        std::cout << "  Rx packets:   " << flow.second.rxBytes / packet_size << "\n";
        std::cout << "  Lost Packets (Tx-Rx): "
                  << (flow.second.txBytes - flow.second.rxBytes) / packet_size << "\n";
        std::cout << "  Throughput: "
                  << flow.second.rxBytes * 8.0 /
                         (flow.second.timeLastRxPacket.GetSeconds() -
                          flow.second.timeFirstTxPacket.GetSeconds()) /
                         1e6
                  << " Mbps\n";
        std::cout << "  Avg Delay: "
                  << (flow.second.rxPackets > 0
                          ? flow.second.delaySum.GetSeconds() / flow.second.rxPackets
                          : 0.0)
                  << " s\n";
    }

    std::cout << "\nPHY layer packet drops: " << phyRxDropCount << std::endl;

    monitor->SerializeToXmlFile("tcp-flows.xml", true, true);
    Simulator::Destroy();

    return 0;
}