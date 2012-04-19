#include "ns3/core-module.h"
#include "ns3/net-device.h"
#include "pep-wifi-net-device_1.h"
#include <kodo/rlnc/full_vector_codes.h>
#include "ns3/llc-snap-header.h"
#include "ns3/adhoc-wifi-mac.h"
#include "ns3/packet.h"
#include "ns3/llc-snap-header.h"	

NS_LOG_COMPONENT_DEFINE ("PepWifiNetDevice");
namespace ns3 {
NS_OBJECT_ENSURE_REGISTERED (PepWifiNetDevice);


PepWifiNetDevice::PepWifiNetDevice ()
  : m_configComplete (false), 
	recode(1),
    max_symbols(1), 
    max_size(128), 
    m_encoder_factory(max_symbols, max_size),	
    m_decoder_factory(max_symbols, max_size)
{
  NS_LOG_FUNCTION_NOARGS ();
  code=1;
  sent_packet=0;
  interval=0.1;
  generation=0;
  received=0;
  countcode=0;
  from_source=0;
  from_relay=0;
  rank=0;
  inc=0;
  ninc=0;
  rsource=0;
  r=0;
  temp=0;
  sent_code=0;
  encoder=m_encoder_factory.build((max_symbols), max_size);
  payload.resize(encoder->payload_size());
  relay_activity=1;
  seed=100;
}


TypeId
PepWifiNetDevice::GetTypeId (void)
        {
  static TypeId tid = TypeId ("ns3::PepWifiNetDevice")
    .SetParent<WifiNetDevice> ()
    .AddConstructor<PepWifiNetDevice> ()
    .AddAttribute ("SymbolsNum",
                   "The number of Symbols in each generation",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PepWifiNetDevice::max_symbols),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("EnableCode",
                   "Enable coding",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PepWifiNetDevice::code),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("EnableRecode",
                   "Enable Recoding",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PepWifiNetDevice::recode),
                   MakeUintegerChecker<uint32_t> ())
	.AddAttribute ("RelayActivity",
                   "relay activity",
                   UintegerValue (1),
                   MakeUintegerAccessor (&PepWifiNetDevice::relay_activity),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}	
       

PepWifiNetDevice::~PepWifiNetDevice ()
{
  NS_LOG_FUNCTION_NOARGS ();
}



void PepWifiNetDevice::SetReceiveCallback (NetDevice::ReceiveCallback receiveCallback)
{

    
	m_receiveCallback = receiveCallback;
	
	if (code == 1)
		WifiNetDevice::SetReceiveCallback (ns3::MakeCallback(&PepWifiNetDevice::DecodingReceive, this));
	else
		WifiNetDevice::SetReceiveCallback (m_receiveCallback);
}


 Ptr<Packet>  
 PepWifiNetDevice::rencoding (Ptr<Packet> packet,int seq)
 {

               uint8_t *buffer1 = new uint8_t[packet->GetSize()]; 
               
               if (forward.find(seq)!=forward.end()) 
               {
					packet->CopyData(buffer1,packet->GetSize());
					
					forward[seq]->decode( buffer1 );
					forward[seq]->recode( &payload[0]);
					
					cout << "payload recode " << encoder->payload_size() << endl;
					cout << "rbuffer size " << packet->GetSize() << endl;
					
					Ptr<Packet> pkt = Create<Packet> (&payload[0],encoder->payload_size());
				return pkt;
					} 
			  else{      
					
					forward[seq]= m_decoder_factory.build((max_symbols), max_size);
					packet->CopyData(buffer1,packet->GetSize());
					
					forward[seq]->decode( buffer1 );
					forward[seq]->recode( &payload[0]);
					
					Ptr<Packet> pkt = Create<Packet> (&payload[0],encoder->payload_size());
						return pkt ;
                 }

 }
        
        
bool
PepWifiNetDevice::DecodingReceive(Ptr< NetDevice > device, Ptr< const
 Packet > packet1, uint16_t type, const Address & from)
{
					cout << "Max symbols" << max_symbols<< endl;
					
					Ptr<Packet> packet = packet1->Copy();
					Mac48Address des=Mac48Address("00:00:00:00:00:01");
					Mac48Address source=Mac48Address("00:00:00:00:00:02");
			
					PointerValue ptr;
					GetAttribute ("Mac",ptr); 
					Ptr<AdhocWifiMac> m_mac = ptr.Get<AdhocWifiMac> (); 
					
				if (from == source && m_mac->GetAddress () == des){
					cout << "from_source:" <<from_source << endl ;
					from_source++;
						}
					
				  if (m_mac->GetAddress ()!=source && (m_mac->GetAddress ()!=des))
					{
						CodeHeader h1;
						packet->RemoveHeader (h1);
						
						if( recode == 1){
							 
							 Ptr<Packet> pkt =rencoding( packet,(int)h1.GetGeneration());
							 pkt->AddHeader (h1);
							 srand ( seed );
							 seed++;
								cout << "recode2:" <<(rand() % 110 + 1) << endl;
						    	if ((rand() % 100 + 1)>  relay_activity)
						    	{
									 cout << "recode3:" <<m_mac->GetAddress () << endl;
									// Send recoded packet	
									 WifiNetDevice::Send(pkt,des,type );
									 sent_code++;
									 cout << "sent_code:" <<sent_code << endl;
								
									}
								}
								else
								 {
									 // Just forwarding
									 packet->AddHeader (h1);
									 srand ( (int)h1.GetGeneration() );

									if ((rand() % 100 + 1)>  relay_activity){
										sent_code++;
										cout << "sent_code:" <<sent_code << endl;
										WifiNetDevice::Send(packet,des,type );
										}
									}
						  }
						
					if ( m_mac->GetAddress ()==source && from==des)
					{
						CodeHeader h1;
						packet->RemoveHeader (h1);
						cout << "Generation is decoded:" << (int)h1.GetGeneration() << endl;
						decoded_flag[(int)h1.GetGeneration()]=1;
					}
				
					if (m_mac->GetAddress ()==des && code==1){
						received++;
						cout << "received:"<<received<< endl;
						uint8_t *buffer1 = new uint8_t[packet->GetSize()];   
						
						CodeHeader h1;
						packet->RemoveHeader (h1);


						if (from!=source){
							cout << "from_relay:" <<from_relay << endl ;
							from_relay++;
						}
						cout << "Generation 10: " << h1.GetGeneration()<< endl;
					
						//inja eshkal dare 
						if (decoding.find((int)h1.GetGeneration())!=decoding.end())
						{
						

							rank=(int)decoding[h1.GetGeneration()]->rank();
							packet->CopyData(buffer1,packet->GetSize());
							decoding[h1.GetGeneration()]->decode( buffer1 );
					   

							cout << "Generation 3: " << h1.GetGeneration()<< endl;
							if ((rank+1)==(int)(decoding[h1.GetGeneration()]->rank()) && from!=source){
								cout << "increased:" <<inc << endl ;
								inc++;
							}
							if ((rank)==(int)(decoding[h1.GetGeneration()]->rank()) && from!=source){
								cout << "not increased:" <<ninc << endl;
								ninc++;
							}
							if (from==source ){
								cout << "recevied_source:" <<rsource++<< endl;

							}
								cout << "rank after:" << decoding[h1.GetGeneration()]->rank()<< endl;
					

						
						//	decoded_flag[(int)h1.GetGeneration()] = 0;
							
						} 
						else
						{ 
							    
							cout << "first : " << h1.GetGeneration()<< endl;
							temp=1;
							decoding[h1.GetGeneration()]= m_decoder_factory.build((max_symbols), max_size);
							
							packet->CopyData(buffer1,packet->GetSize());
							cout << "Generation :" << h1.GetGeneration()<< endl;
							decoding[h1.GetGeneration()]->decode( buffer1 );
							
							rank=(int)decoding[h1.GetGeneration()]->rank();
							
							if ((rank+1)==(int)(decoding[h1.GetGeneration()]->rank()) && from!=source)
							{
								cout << "increased:" <<inc << endl ;
								inc++;
							}
							if ((rank)==(int)(decoding[h1.GetGeneration()]->rank()) && from!=source)
							{
								cout << "not increased:" <<ninc << endl;
								ninc++;
							}
							if (from==source){
								cout << "recevied_source:" <<rsource<< endl;
								rsource++;
							}
								cout << "rank after:" << decoding[h1.GetGeneration()]->rank()<< endl; 
						}


							if (decoding[h1.GetGeneration()]->is_complete() &&  decoded_flag[(int)h1.GetGeneration()] == 0)
							{
								decoded_flag[(int)h1.GetGeneration()]=1;
								
								cout << "time:" << 	Simulator::Now().GetSeconds()<< endl;
							
								countcode++;
								cout << "decoded packets:" << (countcode*(max_symbols)) << endl;
								cout << "decoded packets:" << from << endl;
								
								Ptr<Packet> ACK = Create<Packet> (10);
								ACK->AddHeader (h1);
								WifiNetDevice::Send(ACK,Mac48Address("00:00:00:00:00:02"),100 );
														
								std::vector<uint8_t> data_out(decoding[h1.GetGeneration()]->block_size());
								kodo::copy_symbols(kodo::storage(data_out), decoding[h1.GetGeneration()]); 
							
								for(int i=0;i<(max_symbols);i++)
								{
								    uint8_t *buffer1 = new uint8_t[max_size];
									memcpy(buffer1,&data_out[i*max_size],max_size);
									
									Ptr<Packet> pkt = Create<Packet> (buffer1,max_size);
									m_mac->NotifyRx (pkt);
									m_receiveCallback (this, pkt, type, Mac48Address("00:00:00:00:00:01"));
									
								}

							}
							else if (decoding[h1.GetGeneration()]->is_complete() &&  decoded_flag[(int)h1.GetGeneration()] == 1)
							{
															
									Ptr<Packet> ACK = Create<Packet> (10);
									ACK->AddHeader (h1);
									WifiNetDevice::Send(ACK,Mac48Address("00:00:00:00:00:02"),100 );
									
							}

	}

	return true;
}


bool PepWifiNetDevice::Send (Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber){

   if (code==1)
   {
	    cout << "code is enabled"  <<flush;
	    coding(packet,dest, protocolNumber);
	   }
   else 
   {
	    cout << "code is disabled"  <<endl;
    	WifiNetDevice::Send(packet,dest,protocolNumber);
	}

  return true;	
	}
	
 void 
 PepWifiNetDevice::SendCode(Ptr <coded> m_coded)
 {

		if(decoded_flag[(int)m_coded->h1.GetGeneration()] == 0)
		{

		    sent_packet++;      
			std::cout <<"sent:" << sent_packet << std::endl ;
			std::cout <<"Interval:" << interval << std::endl ;
			
            m_coded->k++;
            kodo::set_symbols(kodo::storage(m_coded->m_encoder_data), m_coded->m_encoder);
    
            std::vector<uint8_t> payload(m_coded->m_encoder->payload_size());
            m_coded->m_encoder->encode( &payload[0] );
	
			Ptr<Packet> pkt = Create<Packet> (&payload[0], m_coded->m_encoder->payload_size());
		
			pkt->AddHeader (m_coded->h1);

			//PointerValue ptr;
			cout << "generation number: " << 	m_coded->h1.GetGeneration() << endl;
						
			WifiNetDevice::Send(pkt,m_coded->realTo,m_coded->protocolNumber );
			
			//m_coded.t2=m_coded.t2+interval;
			Simulator::Schedule ( Seconds (interval), &PepWifiNetDevice::SendCode, this,m_coded);

        }
        else{
              return;
}

}

void
PepWifiNetDevice::Enqueue1 (Ptr<Packet> packet)
{
  
 m_queue.push_back ((packet));
}

bool 
PepWifiNetDevice::coding(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber){
  
  NS_ASSERT (Mac48Address::IsMatchingType (dest));
  Mac48Address realTo = Mac48Address::ConvertFrom (dest);
  
  int k=0;

  Enqueue1(packet);

  if ((int)m_queue.size() == max_symbols )
  {     
		encoder = m_encoder_factory.build((max_symbols), packet->GetSize());
		//encoder->systematic_off();
		
	
		CodeHeader h1;
		h1.SetGeneration (generation);
		generation++;
		
		Ptr<coded> m_coded = Create<coded> (); 
	    m_coded->t2=interval;
	    m_coded->k = k;
	    m_coded->m_encoder = encoder;
	    m_coded->protocolNumber = protocolNumber;
	    m_coded->h1 = h1;
	    m_coded->realTo = realTo;
	    m_coded->m_encoder_data.resize(encoder->block_size());
	
		uint8_t *buffer1 = new uint8_t[packet->GetSize()];

		for(int i=0;i<(max_symbols);i++)
		{
		    Item p = m_queue.front();
		    m_queue.pop_front();
		    // is it correct?? copy data ??
		    p.m_packet->CopyData(buffer1,p.m_packet->GetSize());
		    memcpy(&m_coded->m_encoder_data[(i*packet->GetSize())],buffer1,p.m_packet->GetSize());
		    cout << "data in " << (i*packet->GetSize())<< endl;
	    }
     
	    decoded_flag[(int)h1.GetGeneration()]=0;
	    Simulator::Schedule ( Seconds (interval),&PepWifiNetDevice::SendCode, this, m_coded);
     
     }       
  return true;


}

}
	
