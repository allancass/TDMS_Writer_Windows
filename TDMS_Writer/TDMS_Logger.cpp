#include "TDMS_Logger.h"

	
//TDMS Channel Functions

void TDMS_Channel::get(std::string tdms_name, std::string group_name, std::string channel_name)
{
	if (initialized) { return; }
	/*Initializes the message queue for communicating with the TDMS thread.*/
	TDMS_Queue = new boost::interprocess::message_queue(boost::interprocess::open_or_create, (tdms_name + "_Logger").c_str(), 100, sizeof(TDMSdetail::TDMS_Logging_Packet));
	data.command = TDMSdetail::tdms_log_get_channel;
	channel_data.Name = channel_name;
	/*We pass a pointer to the TDMS thread so we don't need backchannel communication*/
	data.channel_data = &channel_data;
	/*This blocks the current thread until the packet sent has been processed and the channel data is filled in.*/
	std::unique_lock<std::mutex> lk(channel_data.m);
	(*TDMS_Queue).send(&data, sizeof(TDMSdetail::TDMS_Logging_Packet), 0);
	channel_data.cv.wait(lk);

	data.channel = channel_data.channel;
	initialized = true;
	data.command = TDMSdetail::tdms_log_set_value;
}

void TDMS_Channel::set(double value)
{
	if (!initialized) { return; }
	data.value = value;
	(*TDMS_Queue).send(&data, sizeof(TDMSdetail::TDMS_Logging_Packet), 0);
}


void TDMS_Timer_Loop(std::string tdms_name)
{
	boost::interprocess::message_queue TDMS_Timer(boost::interprocess::open_or_create, (tdms_name + "_Timer").c_str(), 100, sizeof(TDMSdetail::TDMS_Timer_Packet));
	boost::interprocess::message_queue TDMS_Queue(boost::interprocess::open_or_create, (tdms_name + "_Logger").c_str(), 100, sizeof(TDMSdetail::TDMS_Logging_Packet));

	bool operating = true;
	bool logging = false;
	uint64_t ms = 20;
	uint64_t time_since_last_send = 0;
	TDMSdetail::TDMS_Timer_Packet timer_packet = { TDMSdetail::tdms_log_pause, 20 };
	TDMSdetail::TDMS_Logging_Packet comm_packet = { TDMSdetail::tdms_log_log_line, 0, 0.0 };
	std::chrono::time_point<std::chrono::system_clock> msNOW = std::chrono::system_clock::now();
	unsigned int junk1, junk2 = 0;

	while (operating)
	{
		/*Need to improve on this thread.*/
		//wait 1 ms
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		time_since_last_send++;
		
		if (logging && (time_since_last_send > ms))
		{
			time_since_last_send = 0;
			msNOW = std::chrono::system_clock::now();
			comm_packet.value = std::chrono::duration_cast<std::chrono::microseconds>(msNOW.time_since_epoch()).count() / 1000000.0;
			TDMS_Queue.send(&comm_packet, sizeof(comm_packet), 0);
		}
		
		if (TDMS_Timer.try_receive(&timer_packet, sizeof(timer_packet), junk1, junk2))
		{
			switch (timer_packet.command)
			{
			case TDMSdetail::tdms_log_update_rate:
				ms = timer_packet.value;
				break;
			case TDMSdetail::tdms_log_start:
				logging = true;
				break;
			case TDMSdetail::tdms_log_pause:
				logging = false;
				break;
			case TDMSdetail::tdms_log_stop:
				logging = false;
				operating = false;
				boost::interprocess::message_queue::remove((tdms_name + "_Timer").c_str());
				//send message to logger to halt here and then have it remove its queue
				comm_packet.command = TDMSdetail::tdms_log_stop;
				TDMS_Queue.send(&comm_packet, sizeof(comm_packet), 0);
				break;
			default:
				break;
			}
		}
	}
}


void TDMS_Logger_Loop(std::string tdms_name, std::string group_name, std::vector<TDMS_Channel_Info> Initial_Channels)
{
	//TDMSdetail::TDMS_Logger_Init params = *((TDMSdetail::TDMS_Logger_Init*) param);
	//delete param;

	boost::interprocess::message_queue TDMS_Queue(boost::interprocess::open_or_create, (tdms_name + "_Logger").c_str(), 100, sizeof(TDMSdetail::TDMS_Logging_Packet));
	bool operating = true;
	unsigned int junk1, junk2;
	TDMSdetail::TDMS_Logging_Packet coms_packet;
	TDMS_File_Writer::TDMS_Writer writer;
		
	writer.TDMS_Initialize(tdms_name, group_name, Initial_Channels);
	while (operating)
	{
		TDMS_Queue.receive(&coms_packet, sizeof(TDMSdetail::TDMS_Logging_Packet), junk1, junk2);

		switch (coms_packet.command)
		{
		case TDMSdetail::tdms_log_get_channel:
			(*coms_packet.channel_data).channel = writer.TDMS_get_channel((*coms_packet.channel_data).Name, TDMS_File_Writer::tdsTypeDoubleFloat);
			(*coms_packet.channel_data).cv.notify_one();
			break;
		case TDMSdetail::tdms_log_log_line:
			writer.TDMS_set_channel(0, coms_packet.value);
			writer.TDMS_set_channel(1, TDMS_Queue.get_num_msg());
			writer.TDMS_Log_Data();
			break;
		case TDMSdetail::tdms_log_set_value:
			writer.TDMS_set_channel(coms_packet.channel, coms_packet.value);
			break;
		case TDMSdetail::tdms_log_stop:
			writer.TDMS_close();
			operating = false;
			boost::interprocess::message_queue::remove((tdms_name + "_Logger").c_str());
			break;
		default:
			break;
		}
	}
}

//TDMS Logger Functions.  Initial rate set to 20ms as this value worked well on previous projects
TDMS_Logger::TDMS_Logger()
{
	logging_rate = 20;
	logging = false;
	initialized = false;
}

TDMS_Logger::~TDMS_Logger()
{
	close();
}

void TDMS_Logger::initialize(std::string tdms_name, std::string group_name, std::vector<TDMS_Channel_Info> Initial_Channels)
{	
	TDMS_Queue = new boost::interprocess::message_queue(boost::interprocess::open_or_create, (tdms_name + "_Logger").c_str(), 100, sizeof(TDMSdetail::TDMS_Logging_Packet));
	TDMS_Timer = new boost::interprocess::message_queue(boost::interprocess::open_or_create, (tdms_name + "_Timer").c_str(), 100, sizeof(TDMSdetail::TDMS_Timer_Packet));
	root_name = tdms_name;
	//std::string *ptdms_name = new std::string;
	//*ptdms_name = tdms_name;
	//Initialzes the TDMS Service Threads.  The Queue threads processes the incomming commands as sent.  The timer thread generates the log command packets
	//To whoever reads this, if you think this was easy, try debugging multiple threads simultaneously some time.	
	TimerThread = new std::thread(TDMS_Timer_Loop, tdms_name);
	LoggerThread = new std::thread(TDMS_Logger_Loop, tdms_name, group_name, Initial_Channels);

	initialized = true;
}

//Begins logging the TDMS channels
void TDMS_Logger::start()
{
	if (initialized)
	{
		(*TDMS_Timer).send(&timer_start, sizeof(TDMSdetail::TDMS_Timer_Packet), 0);
	}
}

//Pauses logging the TDMS data.  Set/Get channel commands will still be processed.
void TDMS_Logger::pause()
{
	if (initialized)
	{

		(*TDMS_Timer).send(&timer_pause, sizeof(TDMSdetail::TDMS_Timer_Packet), 0);
	}
}

//Closes the TDMS Logger and shuts down.  De-initializes the TDMS service
void TDMS_Logger::close()
{
	if (initialized)
	{
		(*TDMS_Timer).send(&timer_stop, sizeof(TDMSdetail::TDMS_Timer_Packet), 0);
		delete TDMS_Timer;
		delete TDMS_Queue;
		initialized = false;
	}
}

//Updates the Logging Rate by changing the timer log command.  Currently specified in miliseconds
void TDMS_Logger::set_rate(uint32_t ms)
{
	if (initialized)
	{
		timer_update_rate.value = ms;
		(*TDMS_Timer).send(&timer_update_rate, sizeof(TDMSdetail::TDMS_Timer_Packet), 0);
	}
}
