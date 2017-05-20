#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "TDMS_File_Writer.h"
#include <boost\interprocess\ipc\message_queue.hpp>
#include <condition_variable>
#include <thread>
//#include <Windows.h>

/*These two data types exist from the TDMS File Writer class.  We need to extract
them for use and feed down (For now, at least).*/
using TDMS_File_Writer::TDMS_Channel_Info;
using namespace TDMS_File_Writer::tdsDataTypedetail;


namespace TDMSdetail {
	struct TDMS_Logger_Init {
		std::string tdms_name;
		std::string group_name;
		std::vector<TDMS_Channel_Info> Initial_Channels;
	};

	typedef enum {
		tdms_log_get_channel,
		tdms_log_set_value,
		tdms_log_update_rate,
		tdms_log_start,
		tdms_log_pause,
		tdms_log_stop,
		tdms_log_log_line
	} TDMS_Log_Command;

	struct TDMS_Channel_Init_Data {
		std::string Name;
		uint32_t channel;
		std::condition_variable cv;
		std::mutex m;
	};

	struct TDMS_Logging_Packet {
		TDMS_Log_Command command;
		uint32_t channel;
		double value;
		TDMS_Channel_Init_Data* channel_data;
	};

	struct TDMS_Timer_Packet {
		TDMS_Log_Command command;
		uint32_t value;
	};
}


class TDMS_Channel
{
	/*This class is used to add or update the value of a TDMS channel to the logging function.
	Currently group name does not perform any function and may be ignored at this time.  Get is a blocking function
	and should not be used where time limits are critical.  Set does not block.*/
public:
	void get(std::string tdms_name, std::string group_name, std::string channel_name);
	void set(double value);

private:
	bool initialized = false;
	boost::interprocess::message_queue *TDMS_Queue;
	TDMSdetail::TDMS_Logging_Packet data;
	TDMSdetail::TDMS_Channel_Init_Data channel_data;
};/**/


class TDMS_Logger
{
	/*This class creates a log file in the TDMS File Format.  It will spawn two threads to do this.  Adding and updating values
	is performed at run-time using the TDMS_Channel class.  Initial channels only need to be provided if you have a specific
	column order you desire the channels to appear in otherwise they will appear as they are first requested.
	*/
public:
	TDMS_Logger(void);
	~TDMS_Logger(void);
	/*Initializes the TDMS Logger class.  The file will be based on the TDMS name provided.  For now, only one group allowed.
	Also, for now, only type Double Float is supported.*/
	void initialize(std::string tdms_name, std::string group_name, std::vector<TDMS_Channel_Info> Initial_Channels);
	/*Begins logging the TDMS channles*/
	void start(void);
	/*Pauses but does not halt TDMS logging*/
	void pause(void);
	/*Closes the TDMS logger and deinitializes it allowing for reuse*/
	void close(void);
	/*Sets the time increment (in miliseconds) the logger operates at.  Can be changed on the fly.*/
	void set_rate(uint32_t ms = 20);
	
private:
	uint64_t logging_rate = 20;
	bool logging = false;
	bool initialized = false;
	std::string root_name = "";
	boost::interprocess::message_queue *TDMS_Queue;
	boost::interprocess::message_queue *TDMS_Timer;

	//Pre-declared packets so I can be lazy later on.
	TDMSdetail::TDMS_Timer_Packet timer_start = { TDMSdetail::tdms_log_start, 0 };
	TDMSdetail::TDMS_Timer_Packet timer_pause = { TDMSdetail::tdms_log_pause , 0 };
	TDMSdetail::TDMS_Timer_Packet timer_stop = { TDMSdetail::tdms_log_stop , 0 };
	TDMSdetail::TDMS_Timer_Packet timer_update_rate = { TDMSdetail::tdms_log_update_rate, 20 };

	std::thread* TimerThread;
	std::thread* LoggerThread;
};
