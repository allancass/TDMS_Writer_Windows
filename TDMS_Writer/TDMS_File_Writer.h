#pragma once
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <vector>

namespace TDMS_File_Writer
{
	namespace tdsDataTypedetail
	{
		typedef enum {
			tdsTypeVoid,
			tdsTypeI8,
			tdsTypeI16,
			tdsTypeI32,
			tdsTypeI64,
			tdsTypeU8,
			tdsTypeU16,
			tdsTypeU32,
			tdsTypeU64,
			tdsTypeSingleFloat,
			tdsTypeDoubleFloat,
			tdsTypeExtendedFloat,
			tdsTypeSingleFloatWithUnit = 0x19,
			tdsTypeDoubleFloatWithUnit,
			tdsTypeExtendedFloatWithUnit,
			tdsTypeString = 0x20,
			tdsTypeBoolean = 0x21,
			tdsTypeTimeStamp = 0x44,
			tdsTypeFixedPoint = 0x4F,
			tdsTypeComplexSingleFloat = 0x08000c,
			tdsTypeComplexDoubleFloat = 0x10000d,
			tdsTypeDAQmxRawData = 0xFFFFFFFF
		} tdsDataType;
	}

	using namespace tdsDataTypedetail;

	typedef enum {
		kTocMetaData = 0x02,		//#define kTocMetaData(1L << 1)	Segment contains meta data
		kTocRawData = 0x08,			//#define kTocRawData          (1L<<3)	Segment contains raw data
		kTocDAQmxRawData = 0x80,	//#define kTocDAQmxRawData     (1L<<7)	Segment contains DAQmx raw data
		kTocInterleavedData = 0x20, //#define kTocInterleavedData  (1L<<5)	Raw data in the segment is interleaved (if flag is not set, data is contiguous)
		kTocBigEndian = 0x40,		//#define kTocBigEndian        (1L<<6)	All numeric values in the segment, including the lead in, raw data, and meta data, are big-endian formatted (if flag is not set, data is little-endian). ToC is not affected by endianess; it is always little-endian.
		kTocNewObjList = 0x04,		//#define kTocNewObjList       (1L<<2)	Segment contains new object list (e.g. channels in this segment are not the same channels the previous segment contains)
		size_force = 0x10000000		//Force to 32bit size
	} ToC_Mask;

	typedef enum
	{
		TDMS_Writer_Uninitialized,
		TDMS_Writer_Adding_Initial_Channels,
		TDMS_Writer_Adding_New_Channels,
		TDMS_Writer_Logging_Data,
	} TDMS_Writer_State;

	struct TDMS_Channel_Info {
		std::string  Name;
		tdsDataType DataType;
	};

	struct TDMS_Channel_data {
		TDMS_Channel_Info Channel_Info;
		double value;
		bool requires_update;
	};

	class TDMS_Writer
	{
	public:
		TDMS_Writer(void);
		~TDMS_Writer(void);

		int TDMS_Initialize(std::string tdms_name, std::string group_name, std::vector<TDMS_Channel_Info> Initial_Channels);
		uint32_t TDMS_get_channel(std::string channel_name, tdsDataType type);
		void TDMS_set_channel(uint32_t channel_id, double value);
		void TDMS_Log_Data();			//Program Call, logs a line of data, will need to update values afterwards.
		void TDMS_close();				//Program Call, flush buffer, close file

	private:
		/*
		Call Header Write
		Then Open Section
		Then <Do Stuff>
		void TDMS_StartRAW()???
		Log Data
		Then Close Section
		*/
		void TDMS_create_root();

		void TDMS_Group_Initialize();
		void TDMS_Channel_Resume();

		void TDMS_add_channel(std::string channel_name, tdsDataType type);
		void TDMS_Write_Fill(std::string channel_name);


		void TDMS_Create_Initial_Object();
		void TDMS_Resume_Data();

		void TDMS_Lead_In(ToC_Mask mask);		 //TDMS Call, creates Section Header
		void TDMS_Write_Meta(uint32_t obj_count);
		void TDMS_EndMeta();						//Call before loging data for first time to update File Position data
		void TDMS_End_Section();					//Call when ending section to update file position data
		void TDMS_Write_Meta_Object(std::string name);

		void TDMS_Channel_Initialize();	//Program Call, Creates Initial File

		void TDMS_Write_Name(std::string property_name);
		void TDMS_Write_NI_ArrayColumn(uint32_t column);
		void TDMS_Write_Property(std::string property_name, tdsDataType type, void* value);


		//File Position Data
		uint64_t Header_End;
		uint64_t section_end;
		uint64_t section_length;

		FILE* TDMS_File;
		std::string file_name;
		std::string group;

		//TDMS_Writer_State Writer_State = TDMS_Writer_Uninitialized;
		uint16_t Writer_State = TDMS_Writer_Uninitialized;

		uint64_t fpos_NextSegment;
		uint64_t fpos_RawDataOffset;
		uint64_t lines_loged;
		std::vector<TDMS_Channel_data> Channel_List;
	};
}