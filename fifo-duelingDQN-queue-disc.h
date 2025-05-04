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

#ifndef DUELINGDQN_FIFO_QUEUE_DISC_H
#define DUELINGDQN_FIFO_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/boolean.h"
#include "ns3/random-variable-stream.h"

#include "ns3/timer.h"
#include "ns3/event-id.h"

#include <vector>
#include <tuple>
#include <deque>
#include <algorithm>
#include <cmath>
#include <random>
#include "ns3/ns3socket-module.h"

using action_t = uint32_t;
using observation_t = std::vector<double>;

namespace ns3 {

static std::string m_cdf_outputFileName1; ///< output file name
static FILE * m_cdf_f1; ///< File handle for output (0 if none)

class cdf_link1
{
public:

  cdf_link1 ()
  {
    m_cdf_outputFileName1 = "";
    m_cdf_f1 = 0;
  }

  static void set_output_file (const std::string& fn)
  {
    if (m_cdf_f1)
      {
        return;
      }
    std::cout << "sg******************************************" << std::endl;

    // NS_LOG_INFO ("Creating new trace file:" << fn.c_str ());
    FILE * f = 0;
    f = std::fopen (fn.c_str (), "w");
    if (!f)
      {
        NS_FATAL_ERROR ("Unable to open output file:" << fn.c_str ());
        return; // Can't open output file
      }

      m_cdf_f1 = f;
      m_cdf_outputFileName1 = fn;
      
    return;
  }

  static  int  WriteN (const char* data, uint32_t count, FILE * f)
  {
    if (!f)
      {
        return 0;
      }
    // Write count bytes to h from data
    uint32_t    nLeft   = count;
    const char* p       = data;
    uint32_t    written = 0;
    while (nLeft)
      {
        int n = std::fwrite (p, 1,  nLeft, f);
        if (n <= 0)
          {
            return written;
          }
        written += n;
        nLeft -= n;
        p += n;
      }
    return written;
  }

  static void ns_CDF (Time t_now, uint32_t length_now, uint32_t buffer_length_now, double singleReward)
  {
    double cdf = (double(length_now) / double(buffer_length_now)) * 100;
    std::string elementString = std::to_string (t_now.GetSeconds()) 
                                + "\t" + std::to_string (length_now)
                                + "\t" + std::to_string (buffer_length_now)
                                + "\t" + std::to_string(cdf)
                                + "\t" + std::to_string(singleReward)                                
                                + "\n";
    WriteN (elementString.c_str (), elementString.length (), m_cdf_f1);
  }
  static void ns_CDF (Time t_now, uint32_t length_now, uint32_t buffer_length_now)
  {
    double cdf = (double(length_now) / double(buffer_length_now)) * 100;
    std::string elementString = std::to_string (t_now.GetSeconds()) 
                                + "\t" + std::to_string (length_now)
                                + "\t" + std::to_string (buffer_length_now)
                                + "\t" + std::to_string(cdf)
                                + "\n";
    WriteN (elementString.c_str (), elementString.length (), m_cdf_f1);
  }
};

/**
 * \ingroup traffic-control
 *
 * Simple queue disc implementing the DUELINGDQN_FIFO policy.
 *
 */
class DuelingDQNFifoQueueDisc : public QueueDisc {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * \brief DuelingDQNFifoQueueDisc constructor
   *
   * Creates a queue with a depth of 50 packets by default
   */
  DuelingDQNFifoQueueDisc ();

  virtual ~DuelingDQNFifoQueueDisc();

  // Reasons for dropping packets
  static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

protected:
  /**
   * \brief Dispose of the object
   */
  virtual void DoDispose (void);

private:
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
  virtual Ptr<QueueDiscItem> DoDequeue (void);
  virtual Ptr<const QueueDiscItem> DoPeek (void);
  virtual bool CheckConfig (void);
  virtual void InitializeParams (void);

  void DropByDQN(void); //Reduce Queue Maxsize
  void AddByDQN(void);  //Add Queue Maxsize
  void KeepByDQN(void); //Maintain Queue Maxsize
  void CalculateRewards(void);
  void createTxt (void);
  void PacketProcessingRate(Ptr<QueueDiscItem>& item, bool& measurement, uint32_t& threshold, double& start, uint64_t& count, double& rate);  //Measure en/dequeue rate
  
  void track_queue_length();  //Record queue length
  EventId m_eventId;
  void SelectAction(void);
  observation_t GetObservation(void); //Get state

  uint32_t m_dequeueThreshold;
  Time m_updatePeriod;  // Slot time
  Time m_desiredQueueDelay;
  uint32_t m_episode;
  bool m_statusTrigger;
  uint32_t count;
  bool m_actionTrigger;
  Time m_currQueueDelay;
  Time m_oldQueueDelay;
  uint32_t m_enqueuedPacket;  // Number of enqueued packets in queue within update period
  uint32_t m_droppedPacket; // Number of dropped packets in queue within update period
  bool m_done;  // True if simulation is done
  action_t m_action;  // Selected action
  float m_singleReward; // Rewards generated by single action
  float m_rewardsSum; // Sum of rewards
  uint32_t m_episodeStepCount;  //Count step for actions

  double m_dequeueRate;
  observation_t m_currState;  //Current state

  uint64_t m_dequeueCount;
  static const uint64_t COUNT_INVALID = std::numeric_limits<uint64_t>::max();	// invalid packet count value

  bool m_dequeueMeasurement;
  double m_dequeueStart;

  TracedValue<double> trace_rewardSum;
  
  std::vector<uint32_t> maxsizeAvg;
  NS3Client DRLclient;  //Socket client

  uint32_t m_addCount;	// Number of add action
  uint32_t m_reduceCount; // Number of reduce action
  uint32_t m_keepCount; // Number of maintain action
  uint32_t iscongest; //Congestion level
};

} // namespace ns3

#endif
