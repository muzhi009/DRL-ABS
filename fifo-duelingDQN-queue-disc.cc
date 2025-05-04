/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
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
 * 
 */

#include "ns3/log.h"
#include "ns3/enum.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "ns3/abort.h"
#include "fifo-duelingDQN-queue-disc.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DuelingDQNFifoQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (DuelingDQNFifoQueueDisc);

TypeId DuelingDQNFifoQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DuelingDQNFifoQueueDisc")
    .SetParent<QueueDisc> ()
    .SetGroupName ("TrafficControl")
    .AddConstructor<DuelingDQNFifoQueueDisc> ()
    .AddAttribute ("MaxSize",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("50p")),
                   MakeQueueSizeAccessor (&QueueDisc::SetMaxSize,
                                          &QueueDisc::GetMaxSize),
                   MakeQueueSizeChecker ())
     .AddAttribute ("UpdatePeriod",
                   "Slot time",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&DuelingDQNFifoQueueDisc::m_updatePeriod),
                   MakeTimeChecker ())
    .AddAttribute ("DequeueThreshold",
                   "Minimum queue size in byte before dequeue rate is measured",
                   UintegerValue (2000),
                   MakeUintegerAccessor (&DuelingDQNFifoQueueDisc::m_dequeueThreshold),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("DesiredQueueDelay",
                   "Desired queueing delay",
                   TimeValue (Seconds (2)),
                   MakeTimeAccessor (&DuelingDQNFifoQueueDisc::m_desiredQueueDelay),
                   MakeTimeChecker ())
    .AddAttribute ("Episode",
                   "N th episode",
                   UintegerValue (1),
                   MakeUintegerAccessor (&DuelingDQNFifoQueueDisc::m_episode),
                   MakeUintegerChecker<uint32_t> ())
		.AddAttribute ("StatusTrigger",
                   "Show status in std output",
                   BooleanValue (false),
                   MakeBooleanAccessor (&DuelingDQNFifoQueueDisc::m_statusTrigger),
                   MakeBooleanChecker ())
    .AddTraceSource ("SumReward",
                    "the sum reward of one episode",
                    MakeTraceSourceAccessor (&DuelingDQNFifoQueueDisc::trace_rewardSum),
                    "ns3::TracedValueCallback::double")
  ;
  return tid;
}

DuelingDQNFifoQueueDisc::DuelingDQNFifoQueueDisc ()
  : QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
  NS_LOG_FUNCTION (this);
  count = 0;
  
  Simulator::Schedule (Seconds (0.0), &DuelingDQNFifoQueueDisc::createTxt, this);
  
  Simulator::Schedule(Seconds(0.1), &DuelingDQNFifoQueueDisc::track_queue_length, this);

  m_eventId = Simulator::Schedule (Seconds (0.0), &DuelingDQNFifoQueueDisc::SelectAction, this);
}

DuelingDQNFifoQueueDisc::~DuelingDQNFifoQueueDisc ()
{
  NS_LOG_FUNCTION (this);
}

void
DuelingDQNFifoQueueDisc::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  std::cout << std::endl << "Sum of rewards: " << m_rewardsSum << std::endl;
  trace_rewardSum = (double)m_rewardsSum;
	std::cout << "Episode " << m_episode << " step count: " << m_episodeStepCount << std::endl;
	std::cout << "Number of Add action: " << m_addCount << ", Reduce action: " << m_reduceCount << ", Keep action: " << m_keepCount << std::endl << std::endl;
  double sum = std::accumulate(maxsizeAvg.begin(), maxsizeAvg.end(), 0.0);
  double average = sum / maxsizeAvg.size();
  std::cout<<"The average buffer size: "<<average<<std::endl;
  DRLstate state1 = {(float)0.0, (float)0.0, (float)0.0, (float)0.0, (float)0.0, true};
  DRLclient.SendData(&state1);
  std::cout<<"Train over."<<std::endl;
  DRLclient.CloseClient();

  QueueDisc::DoDispose ();
	Simulator::Remove (m_eventId);
  QueueDisc::DoDispose ();
}

void
DuelingDQNFifoQueueDisc::createTxt(void){
  std::string prefix = "FIFO_Westwood1.5/duelingDQN_FIFO__buffer";
  std::stringstream ss;
  ss << prefix << m_episode << ".txt";
  std::string filepath = ss.str();
  std::cout<<filepath<<std::endl;
  cdf_link1::set_output_file(filepath);
}

bool
DuelingDQNFifoQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item)
{
  NS_LOG_FUNCTION (this << item);
  
  if (GetCurrentSize () + item > GetMaxSize ())
    {
      NS_LOG_LOGIC ("Queue full -- dropping pkt");
      m_droppedPacket++;
      DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP);
      
      return false;
    }
  m_enqueuedPacket++;
  bool retval = GetInternalQueue (0)->Enqueue (item);

  if(retval && iscongest <5){ //Record the current congestion situation
    iscongest++;
  }else if(!retval && iscongest >-5){
    iscongest--;
  }

  // If Queue::Enqueue fails, QueueDisc::DropBeforeEnqueue is called by the
  // internal queue because QueueDisc::AddInternalQueue sets the trace callback

  NS_LOG_LOGIC ("Number packets " << GetInternalQueue (0)->GetNPackets ());
  NS_LOG_LOGIC ("Number bytes " << GetInternalQueue (0)->GetNBytes ());

  return retval;
}

Ptr<QueueDiscItem>
DuelingDQNFifoQueueDisc::DoDequeue (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<QueueDiscItem> item = GetInternalQueue (0)->Dequeue ();
  
  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }
  PacketProcessingRate(item, m_dequeueMeasurement, m_dequeueThreshold, m_dequeueStart, m_dequeueCount, m_dequeueRate);  //Calculate the rate of leaving the queue
  return item;
}

Ptr<const QueueDiscItem>
DuelingDQNFifoQueueDisc::DoPeek (void)
{
  NS_LOG_FUNCTION (this);

  Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

  if (!item)
    {
      NS_LOG_LOGIC ("Queue empty");
      return 0;
    }

  return item;
}

bool
DuelingDQNFifoQueueDisc::CheckConfig (void)
{
  NS_LOG_FUNCTION (this);
  if (GetNQueueDiscClasses () > 0)
    {
      NS_LOG_ERROR ("DuelingDQNFifoQueueDisc cannot have classes");
      return false;
    }

  if (GetNPacketFilters () > 0)
    {
      NS_LOG_ERROR ("DuelingDQNFifoQueueDisc needs no packet filter");
      return false;
    }

  if (GetNInternalQueues () == 0)
    {
      // add a DropTail queue
      AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>
                          ("MaxSize", QueueSizeValue (GetMaxSize ())));
    }

  if (GetNInternalQueues () != 1)
    {
      NS_LOG_ERROR ("DuelingDQNFifoQueueDisc needs 1 internal queue");
      return false;
    }

  return true;
}

void
DuelingDQNFifoQueueDisc::InitializeParams (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_INFO ("Initializing DuelingDQNQueueDisc params.");

	m_dequeueRate = 0.0;
	m_dequeueMeasurement = false;
	m_dequeueStart = 0;
	m_dequeueCount = COUNT_INVALID;

	m_actionTrigger = true;
	m_currQueueDelay = Seconds(0);
	m_oldQueueDelay = Seconds(0);
	m_enqueuedPacket = 0;
	m_droppedPacket = 0;
	m_rewardsSum = 0;
	m_done = false;

	m_episodeStepCount = 0;
	m_action = 1;

	m_addCount = 0;
	m_reduceCount = 0;
  m_keepCount =0;
  iscongest = 0;
}

void DuelingDQNFifoQueueDisc::SelectAction(void) {

	if (GetCurrentSize ().GetValue() > 0)  {
    if (m_statusTrigger == true) {
      double now = Simulator::Now ().GetSeconds ();
			std::cout << std::endl << "Current virtual time: " << now << std::endl;
			std::cout << "*** Current State ***" << std::endl;
		}
		m_currState.clear();
		m_currState = GetObservation(); //Get current state
    
    DRLstate state1 = {(float)m_currState[0], (float)m_currState[1], (float)m_currState[2], (float)m_currState[3], 
    (float)m_singleReward, false};
    DRLclient.SendData(&state1);  //Send to RL algorithm
    m_action =DRLclient.RecvData();   //Recive action from RL algorithm

    if(m_action==0){
        AddByDQN();
        m_addCount++;
    }
    else if(m_action==1){
        KeepByDQN();
        m_keepCount++;
    }
    else if(m_action==2){
        DropByDQN();
        m_reduceCount++;
    }
		m_actionTrigger = false;	// Set trigger false before going to next state
		m_enqueuedPacket = 0;
		m_droppedPacket = 0;
		m_oldQueueDelay = m_currQueueDelay;
		m_eventId = Simulator::Schedule (m_updatePeriod, &DuelingDQNFifoQueueDisc::CalculateRewards, this); //Calculate reward after slot time

	}

	if (m_actionTrigger == true) {	// Keep checking if queue delay is 0
		m_eventId = Simulator::Schedule (m_updatePeriod, &DuelingDQNFifoQueueDisc::SelectAction, this);
	}
}

void DuelingDQNFifoQueueDisc::DropByDQN(void) {

  uint32_t currentQueueSize = GetInternalQueue(0)->GetNPackets();
  uint32_t maxSize = QueueDisc::GetMaxSize().GetValue();

  uint32_t newMaxSize = maxSize-1 >= currentQueueSize ? maxSize-1 : currentQueueSize; //Pay attention to queue length when reducing buffer size
  newMaxSize = newMaxSize >= 1? newMaxSize : 1; //Pay attention to minimum buffer size
  std::string _pkt_max = std::to_string(newMaxSize) + "p";
  QueueDisc::SetMaxSize(QueueSize(_pkt_max));    
}

void DuelingDQNFifoQueueDisc::KeepByDQN(void) {

    uint32_t maxSize = QueueDisc::GetMaxSize().GetValue();
    uint32_t newMaxSize = maxSize;
    std::string _pkt_max = std::to_string(newMaxSize) + "p";
    QueueDisc::SetMaxSize(QueueSize(_pkt_max));
}

void DuelingDQNFifoQueueDisc::AddByDQN(void) {

  uint32_t maxSize = QueueDisc::GetMaxSize().GetValue();

  uint32_t newMaxSize = maxSize+1 <= 100 ? maxSize+1 : 100; //The maximum buffer length is 100

  std::string _pkt_max = std::to_string(newMaxSize) + "p";
  QueueDisc::SetMaxSize(QueueSize(_pkt_max));
}

void DuelingDQNFifoQueueDisc::PacketProcessingRate(Ptr<QueueDiscItem>& item, bool& measurement, uint32_t& threshold, double& start, uint64_t& count, double& rate) {	
	// Measure en/dequeue rate after processing the item
	double now = Simulator::Now ().GetSeconds ();
	uint32_t pktSize = item->GetSize();

	if ((GetInternalQueue (0)->GetNBytes () >= threshold) && (!measurement)) {
		start = now;
		count = 0;
		measurement = true;
	}
	if (measurement) {
		count += pktSize;

		if (count >= threshold) {
			double tmp = now - start;

			if (tmp > 0) {
				if (rate == 0) {
					rate = (double)count / tmp;
				}
				else {	// Proportion of old/new processing rate can be changed
					rate = (0.5 * rate) + (0.5 * (count / tmp));
				}
			}
			// Restart a measurement cycle if number of packets in queue exceeds the threshold
			if (GetInternalQueue (0)->GetNBytes () > threshold) {
				start = now;
				count = 0;
				measurement = true;
			}
			else {
				count = 0;
				measurement = false;
			}
		}
	}
}

void DuelingDQNFifoQueueDisc::CalculateRewards(void) {
  if (m_statusTrigger == true)
      std::cout << std::endl << "*** Rewards ***" << std::endl;

  if (m_dequeueRate > 0) {
		m_currQueueDelay = Time (Seconds (GetInternalQueue (0)->GetNBytes () / m_dequeueRate));
	}
	else {
		m_currQueueDelay = Time (Seconds(0));
	}

  if(iscongest <= 0){
    m_singleReward = (float)m_currQueueDelay.GetSeconds() / m_desiredQueueDelay.GetSeconds () ;
  }else{
    m_singleReward = (float)GetInternalQueue (0)->GetNPackets() / maxSize;
  }

  m_singleReward = std::max((float)-1.0, m_singleReward);   // Clipped by min / max value
  m_singleReward = std::min((float)1.0, m_singleReward);

  if (m_statusTrigger == true) {
    std::cout << "reward: " << m_singleReward << std::endl << std::endl;;
  }

  m_rewardsSum += m_singleReward;
  m_episodeStepCount++;   // Increment of step count

  if ( (m_currQueueDelay.GetSeconds () < 0.5 * m_desiredQueueDelay.GetSeconds ()) && 
    (m_oldQueueDelay.GetSeconds () < (0.5 * m_desiredQueueDelay.GetSeconds ())) && 
    (m_action == 1) && 
    (m_dequeueRate > 0)) {
    m_dequeueCount = COUNT_INVALID;
    m_dequeueRate = 0.0;
  }

  m_action = 1;
  m_actionTrigger = true;
  m_eventId = Simulator::Schedule (NanoSeconds(0), &DuelingDQNFifoQueueDisc::SelectAction, this);
}

observation_t DuelingDQNFifoQueueDisc::GetObservation(void) {
	observation_t ob;

	if (m_dequeueRate > 0) {
		m_currQueueDelay = Time (Seconds (GetInternalQueue (0)->GetNBytes () / m_dequeueRate));
	}
	else {
		m_currQueueDelay = Time (Seconds(0));
	}
  uint32_t maxSize = QueueDisc::GetMaxSize().GetValue();
	ob.push_back(GetCurrentSize ().GetValue());
	ob.push_back(m_dequeueRate * 8 / 1e+6);	// Convert to Mbps
	ob.push_back(m_currQueueDelay.GetSeconds());
	ob.push_back(maxSize);
	
	if (m_statusTrigger == true) {
		std::cout << "Current queue size in packet: " << GetCurrentSize ().GetValue() << "p" << std::endl;
		std::cout << "dequeue rate: " << m_dequeueRate * 8 / 1e+6 << "Mbps" << std::endl;
		std::cout << "Current queue delay: " << m_currQueueDelay.GetSeconds() << "s" << std::endl;
    std::cout << "Current maxSize: " << maxSize << "p"<< std::endl;
	}

	return ob;
}

void DuelingDQNFifoQueueDisc::track_queue_length()
{
  maxsizeAvg.push_back(QueueDisc::GetMaxSize().GetValue()); //Record Buffer size
  cdf_link1::ns_CDF(Simulator::Now(), GetInternalQueue (0)->GetNPackets (), QueueDisc::GetMaxSize().GetValue());
  Simulator::Schedule(Seconds(0.1), &DuelingDQNFifoQueueDisc::track_queue_length, this);
}

} // namespace ns3
