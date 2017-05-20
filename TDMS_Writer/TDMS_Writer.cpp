// TDMS_Writer.cpp : Defines the entry point for the console application.
//
#pragma warning(disable:4996)
#define _SCL_SECURE_NO_WARNINGS

#include "TDMS_Logger.h"
#include <iostream>
#include <time.h>
//#include <thread>
#include <chrono>

using std::cout;
using std::endl;

/*void auto_log(TDMS_Logger::TDMS_File_Writer::TDMS_Writer &Log, uint64_t lines)
{
	for (auto i = 0; i < lines; i++)
	{
		Log.TDMS_Log_Data();
	}
}/**/

int main()
{
	
	//file:///D:/Program%20Files%20(x86)/boost_1_63_0/doc/html/boost/lockfree/queue.html
	//http://en.cppreference.com/w/cpp/thread/thread/thread
	

	

	/*TDMS_Channel AChan;
	TDMS_Logger log;
	
	std::vector<TDMS_Channel_Info> Initial_Channels = { { "Channel_A", tdsTypeDoubleFloat } };
			
	std::cout << "Initializing Channels" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	log.initialize("TDMS_Test", "Test_Group", Initial_Channels);
	AChan.get("TDMS_Test", "Test_Group", "Channel_A");
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "Initialized Channels" << std::endl;
	std::cout << "Setting Rate" << std::endl;
	log.set_rate(100);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "Rate Set" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	std::cout << "Starting" << std::endl;
	log.start();
	AChan.set(5);
	std::cout << "Started" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	std::cout << "Pausing" << std::endl;
	AChan.set(6);
	std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	log.pause();
	std::cout << "Paused" << std::endl;
	std::cout << "Starting" << std::endl;
	log.start();
	AChan.set(7);
	std::cout << "Started" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(10000));
	AChan.set(8);
	std::cout << "Closing" << std::endl;
	log.close();
	std::cout << "Closed" << std::endl;
	/**/

	uint32_t NumChans = 4;
	uint32_t NumLines = 50;

	TDMS_Logger log;

	std::vector<TDMS_Channel_Info> Initial_Channels = { {"Channel 1", tdsTypeDoubleFloat},{"Channel 2", tdsTypeDoubleFloat} };
	//std::vector<TDMS_Channel> All_Channels;
	TDMS_Channel All_Channels[4];
	
	
	
	cout << "All Declared" << endl;

	log.initialize("perftest", "perfgroup", Initial_Channels);
	cout << "Log Initialized" << endl;
	log.set_rate(40);
	cout << "Log Rate Set" << endl;

	log.start();
	cout << "Log Started" << endl;
	
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	for (auto i = 0; i < NumChans; i++)
	{
		All_Channels[i].get("perftest", "perfgroup", "Channel " + std::to_string(i));
	}

	

	for (auto j = 0; j < NumLines; j++)
	{
		std::cout << j << std::endl;

		for (auto i = 0; i < NumChans; i++)
		{
			All_Channels[i].set(rand());
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}/**/

	log.close();

	scanf("");
	return 0;
}

