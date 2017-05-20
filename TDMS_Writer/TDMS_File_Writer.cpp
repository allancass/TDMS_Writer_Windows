#include "TDMS_File_Writer.h"

namespace TDMS_File_Writer
{

	TDMS_Writer::TDMS_Writer(void)
	{
		//Sets initial state for the state machine
		Writer_State = TDMS_Writer_Uninitialized;
	}
	TDMS_Writer::~TDMS_Writer(void)
	{
		//Safely closes the file
		TDMS_close();
	}

	void TDMS_Writer::TDMS_Lead_In(ToC_Mask mask)
	{

		char tag[] = "TDSm\0";

		//Version number (4713)
		uint32_t Version = 4713;
		//Default value so that if file is interupted it'll still read it.  See below.
		uint64_t NextSegment = 0xFFFFFFFFFFFFFFFF;
		//May be same as NextSegment, need to see more.
		uint64_t RawDataOffset = 0;

		//The lead in contains information used to validate a segment. The lead in also contains information used for random access to a TDMS file.
		/* The lead in starts with a 4-byte tag that identifies a TDMS segment ("TDSm").
		*/
		fwrite(tag, sizeof(uint8_t), 4, TDMS_File);
		/*The next four bytes are used as a bit mask in order to indicate what kind of data the segment contains. This bit mask is referred to as ToC (Table of Contents)
		*/
		fwrite(&mask, sizeof(ToC_Mask), 1, TDMS_File);
		/*The next four bytes contain a version number(32 - bit unsigned integer), which specifies the oldest TDMS revision a segment
		complies with.At the time of this writing, the version number is 4713. The only previous version of TDMS has number 4712.
		*/
		fwrite(&Version, sizeof(uint32_t), 1, TDMS_File);

		/*The next eight bytes (64-bit unsigned integer) describe the length of the remaining segment (overall length of the segment minus length of the lead in).
		If further segments are appended to the file, this number can be used to locate the starting point of the following segment.
		If an application encountered a severe problem while writing to a TDMS file (crash, power outage), all bytes of this integer can be 0xFF. This can only happen to the last segment in a file.
		*/
		fpos_NextSegment = ftell(TDMS_File);
		fwrite(&NextSegment, sizeof(uint64_t), 1, TDMS_File);
		/*The last eight bytes (64-bit unsigned integer) describe the overall length of the meta information in the segment. This information is used for random access to the raw data.
		If the segment contains no meta data at all (properties, index information, object list), this value will be 0.
		*/
		fpos_RawDataOffset = ftell(TDMS_File);
		fwrite(&RawDataOffset, sizeof(uint64_t), 1, TDMS_File);


		Header_End = ftell(TDMS_File);
	}

	//Property Writers
	//Writes the Property "name" which is used for channel/group/file declaration
	void TDMS_Writer::TDMS_Write_Name(std::string property_name)
	{
		uint32_t length = 0;
		tdsDataType tdsString = tdsTypeString;
		// uint64_t section_end;

		//Length of property name:			0x14					0x04	
		//Property name:					tdsDataType 1 1 0 0		name	
		//Data Type							I32						string
		//length of property (string only)	N/A (not string)		length(namestr)
		//value of property					column#					namestr

		//Writes length of property name
		length = property_name.length();
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
		//Writes property type	
		fwrite(property_name.c_str(), sizeof(uint8_t), length, TDMS_File);
		fwrite(&tdsString, sizeof(tdsDataType), 1, TDMS_File);
		//Writes length and property
		length = file_name.length();
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
		fwrite(file_name.c_str(), sizeof(uint8_t), length, TDMS_File);
	}
	//Writes teh propert "NI_ArrayColumn" which is each channel in the file
	void TDMS_Writer::TDMS_Write_NI_ArrayColumn(uint32_t column)
	{

		uint32_t intro[] = { 0x14,tdsTypeDoubleFloat,0x01,0x01 ,0x00 ,0x01 ,0x0E };
		// 0: Length of Index Information (s Value1-5)
		// 1: tdsDataType
		// 2: 0x01 Dimmension of the raw data array (must be 1)
		// 3: 0x01 Number of Values (u64) of interleaved data, N =  <Value A 1> .... <Value A N> <Value B 1> 
		// 4: 0x00 Number of Values part 2
		// 5: 0x01 Number of Properties (One, NI_ArrayColumn)
		// 6: Length of string "NI_ArrayColumn"
		char property[] = "NI_ArrayColumn";
		uint32_t outro[] = { tdsTypeI32 ,0x00 };

		// 0: Data Type of Property Value
		// 0b: If data type = string, then length of string
		// 1: Value of the property
		outro[1] = column;
		fwrite(intro, sizeof(uint32_t), 7, TDMS_File);
		fwrite(property, sizeof(uint8_t), 14, TDMS_File);
		fwrite(outro, sizeof(uint32_t), 2, TDMS_File);
	}
	//Currently unused generic property writer under development
	void TDMS_Writer::TDMS_Write_Property(std::string property_name, tdsDataType type, void* value)
	{
		//Length of property name:			04		14
		//Property name:					name	tdsDataType (u32):1 (u64):1 #Properties:n
		//Data Type							string	I32
		//length of property string only	big		N/A
		//value of property					big		column#

		uint32_t length = 0;

		//Writes length of property name
		length = property_name.length();
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
		fwrite(property_name.c_str(), sizeof(uint8_t), length, TDMS_File);

		//Writes property type
		fwrite(&type, sizeof(tdsDataType), 1, TDMS_File);


		switch (type)
		{
		case tdsTypeBoolean:
		case tdsTypeComplexDoubleFloat:
		case tdsTypeComplexSingleFloat:
		case tdsTypeDAQmxRawData:
		case tdsTypeDoubleFloatWithUnit:
		case tdsTypeExtendedFloat:
		case tdsTypeExtendedFloatWithUnit:
		case tdsTypeFixedPoint:
		case tdsTypeSingleFloatWithUnit:
		case tdsTypeTimeStamp:
		case tdsTypeVoid:
			break;
		case tdsTypeDoubleFloat:
			length = sizeof(double);
			break;
		case tdsTypeI8:
			break;
		case tdsTypeI16:
			break;
		case tdsTypeI32:
			break;
		case tdsTypeI64:
			break;
		case tdsTypeSingleFloat:
			length = sizeof(float);
			break;
		case tdsTypeString:
			length = (*((std::string*)value)).size();
			break;
		case tdsTypeU8:
			length = sizeof(uint8_t);
			break;
		case tdsTypeU16:
			length = sizeof(uint16_t);
			break;
		case tdsTypeU32:
			length = sizeof(uint32_t);
			break;
		case tdsTypeU64:
			length = sizeof(uint64_t);
			break;
		default:
			break;
		}

		if (type = tdsTypeString)
		{

		}
		else
		{
			fwrite(value, length, 1, TDMS_File);
		}


	}

	void TDMS_Writer::TDMS_EndMeta()
	{
		section_end = ftell(TDMS_File);
		section_length = section_end - Header_End;
		fseek(TDMS_File, fpos_RawDataOffset, SEEK_SET);
		fwrite(&section_length, sizeof(uint64_t), 1, TDMS_File);
		fseek(TDMS_File, 0, SEEK_END);
	}

	void TDMS_Writer::TDMS_End_Section()
	{
		section_end = ftell(TDMS_File);
		section_length = section_end - Header_End;
		fseek(TDMS_File, fpos_NextSegment, SEEK_SET);
		fwrite(&section_length, sizeof(uint64_t), 1, TDMS_File);
		fseek(TDMS_File, 0, SEEK_END);
	}

	//Meta Writers
	void TDMS_Writer::TDMS_Write_Meta(uint32_t obj_count)
	{
		uint32_t length = 0;
		std::string base_group = "/'" + group + "'";
		std::string object_name = "";
		uint32_t Zero = 0;

		/*
		TDMS meta data consists of a three-level hierarchy of data objects including a file, groups, and channels. Each of these object types can include any number of properties. The meta data section has the following binary layout on disk:

		Number of new objects in this segment (unsigned 32-bit integer).
		Binary representation of each of these objects.
		*/
		length = obj_count;
		//Number of Objects
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
	}

	void TDMS_Writer::TDMS_Write_Meta_Object(std::string name)
	{
		uint32_t length = 0;
		std::string base_group = "/'" + group + "'";
		std::string object_name = "";
		uint32_t Zero = 0;

		object_name = base_group + "/'" + name + "'";
		length = object_name.length();
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
		fwrite(object_name.c_str(), sizeof(uint8_t), length, TDMS_File);
	}

	void TDMS_Writer::TDMS_create_root()
	{
		uint32_t one = 1;
		uint32_t NoRaw = 0xFFFFFFFF;

		TDMS_Write_Meta(1);

		//Length of First Object Path
		fwrite(&one, sizeof(uint32_t), 1, TDMS_File);
		//Object Path for root "/"
		fwrite("/", sizeof(uint8_t), 1, TDMS_File);
		//No data sssigned so FF FF FF FF
		fwrite(&NoRaw, sizeof(uint32_t), 1, TDMS_File);
		//Number of Properties
		fwrite(&one, sizeof(uint32_t), 1, TDMS_File);
		//Write Property "name"	
		TDMS_Write_Name("name");

		TDMS_End_Section();
		TDMS_EndMeta();
	}

	void TDMS_Writer::TDMS_Create_Initial_Object()
	{
		//uint32_t quantity = ;
		//tdsDataType Type;
		uint32_t length = 0;
		uint32_t NoRaw = 0xFFFFFFFF;
		uint32_t Zero = 0;
		std::string base_group = "/'" + group + "'";
		std::string object_name = "";



		TDMS_Write_Meta(1 + Channel_List.size());

		//Write Length & object name
		length = base_group.length();
		fwrite(&length, sizeof(uint32_t), 1, TDMS_File);
		fwrite(base_group.c_str(), sizeof(uint8_t), length, TDMS_File);

		//Raw Data Index: No data sssigned so FF FF FF FF
		fwrite(&NoRaw, sizeof(uint32_t), 1, TDMS_File);
		//Number of Properties
		fwrite(&Zero, sizeof(uint32_t), 1, TDMS_File);

		for (auto i = 0; i < Channel_List.size(); i++)
		{
			TDMS_Write_Meta_Object(Channel_List[i].Channel_Info.Name);
			TDMS_Write_NI_ArrayColumn(i);
			Channel_List[i].requires_update = false;
		}

		TDMS_EndMeta();

	}

	void TDMS_Writer::TDMS_Resume_Data()
	{
		uint32_t length = 0;
		std::string base_group = "/'" + group + "'";
		std::string object_name = "";
		uint32_t Zero = 0;

		uint32_t intro[] = { 0x14,tdsTypeDoubleFloat,0x01,0x01 ,0x00 ,0x00 };

		TDMS_Lead_In((ToC_Mask)(kTocInterleavedData | kTocRawData | kTocNewObjList | kTocMetaData));

		TDMS_Write_Meta(Channel_List.size());

		//for (auto i = 0; i < Channel_List.size() - 1; i++)
		for (auto i = 0; i < Channel_List.size(); i++)
		{
			TDMS_Write_Meta_Object(Channel_List[i].Channel_Info.Name);

			//Properties
			// 0: Length of Index Information (s Value1-5)
			// 1: tdsDataType
			// 2: 0x01 Dimmension of the raw data array (must be 1)
			// 3: 0x01 Number of Values (u64) of interleaved data, N =  <Value A 1> .... <Value A N> <Value B 1> 
			// 4: 0x00 Number of Values part 2
			// 5: 0x00 Number of Properties (Zero, no update)

			if (Channel_List[i].requires_update) { TDMS_Write_NI_ArrayColumn(i); }
			else { fwrite(intro, sizeof(uint32_t), 6, TDMS_File); }
		}

		TDMS_EndMeta();
	}

	void TDMS_Writer::TDMS_Write_Fill(std::string channel_name)
	{
		uint32_t length = 0;
		std::string base_group = "/'" + group + "'";
		std::string object_name = "";
		uint32_t Zero = 0;
		double Fill = 0.0;

		TDMS_Lead_In((ToC_Mask)(kTocRawData | kTocMetaData | kTocNewObjList));

		TDMS_Write_Meta(1);

		//Write Object Name
		TDMS_Write_Meta_Object(channel_name);

		//Properties
		uint32_t intro[] = { 0x14,tdsTypeDoubleFloat,0x01,0x01 ,0x00 ,0x00 };
		// 0: Length of Index Information (s Value1-5)
		// 1: tdsDataType
		// 2: 0x01 Dimmension of the raw data array (must be 1)
		// 3: 0x01 Number of Values (u64) of interleaved data, N =  <Value A 1> .... <Value A N> <Value B 1> 
		// 4: 0x00 Number of Values part 2
		// 5: 0x00 Number of Properties (Zero, no update)

		fwrite(intro, sizeof(uint32_t), 6, TDMS_File);

		TDMS_EndMeta();

		for (size_t i = 0; i < lines_loged; i++)
		{
			fwrite(&Fill, sizeof(double), 1, TDMS_File);
		}

		TDMS_End_Section();
	}

	void TDMS_Writer::TDMS_Log_Data()
	{
		double dummy = 0.0;

		switch (Writer_State)
		{
		case TDMS_Writer_Uninitialized:
			break;
		case TDMS_Writer_Adding_Initial_Channels:
			TDMS_Create_Initial_Object();
			Writer_State = TDMS_Writer_Logging_Data;
			break;
		case TDMS_Writer_Adding_New_Channels:
			Writer_State = TDMS_Writer_Logging_Data;
			TDMS_Resume_Data();
			break;
		case TDMS_Writer_Logging_Data:
			break;
		default:
			break;
		}/**/


		lines_loged++;

		for (auto i = 0; i < Channel_List.size(); i++)
		{
			fwrite(&Channel_List[i].value, sizeof(double), 1, TDMS_File);
		}
	}

	int TDMS_Writer::TDMS_Initialize(std::string tdms_name, std::string group_name, std::vector<TDMS_Channel_Info> Initial_Channels)
	{
		switch (Writer_State)
		{
		case TDMS_Writer_Uninitialized:
			break;
		case TDMS_Writer_Adding_Initial_Channels:
		case TDMS_Writer_Adding_New_Channels:
		case TDMS_Writer_Logging_Data:
			return -1;
			break;
		default:
			return -1;
			break;
		}/**/


		std::string filename = tdms_name + ".tdms";
		TDMS_File = std::fopen(filename.c_str(), "wb");

		lines_loged = 0;

		// Create initial TDMS File
		file_name = tdms_name;
		TDMS_Lead_In((ToC_Mask)(kTocMetaData | kTocNewObjList));

		TDMS_create_root();


		//Add Group, Time, and Buffer along with any initial groups
		TDMS_Lead_In((ToC_Mask)(kTocInterleavedData | kTocRawData | kTocNewObjList | kTocMetaData));

		group = group_name;
		Writer_State = TDMS_Writer_Adding_Initial_Channels;

		TDMS_add_channel("Time Stamp", tdsTypeDoubleFloat);
		TDMS_add_channel("TDMS Buffer Size", tdsTypeDoubleFloat);


		for (auto i = 0; i < Initial_Channels.size(); i++)
		{
			TDMS_add_channel(Initial_Channels[i].Name, Initial_Channels[i].DataType);
		}

		return 0;
	}

	void TDMS_Writer::TDMS_add_channel(std::string channel_name, tdsDataType type)
	{
		switch (Writer_State)
		{
		case TDMS_Writer_Uninitialized:
			return;
			break;
		case TDMS_Writer_Adding_Initial_Channels:
			break;
		case TDMS_Writer_Adding_New_Channels:
			TDMS_Write_Fill(channel_name);
			break;
		case TDMS_Writer_Logging_Data:
			TDMS_End_Section();
			Writer_State = TDMS_Writer_Adding_New_Channels;
			TDMS_Write_Fill(channel_name);
			break;
		default:
			return;
			break;
		}/**/

		//TDMS_Channel_data Temporary_Channel = { channel_name, type, 0.0, true };
		//Channel_List.emplace_back(Temporary_Channel);
		Channel_List.emplace_back(TDMS_Channel_data { channel_name, type, 0.0, true });
	}

	uint32_t TDMS_Writer::TDMS_get_channel(std::string channel_name, tdsDataType type)
	{
		uint32_t i = 0;
		for (i = 0; i < Channel_List.size(); i++)
		{
			if (Channel_List[i].Channel_Info.Name == channel_name)
			{
				return i;
			}
		}

		TDMS_add_channel(channel_name, type);
		Channel_List[i].value = 0.0;
		return (i);
	}

	void TDMS_Writer::TDMS_set_channel(uint32_t channel_id, double value)
	{
		Channel_List[channel_id].value = value;
	}

	void TDMS_Writer::TDMS_close()
	{
		switch (Writer_State)
		{
		case TDMS_Writer_Uninitialized:
			return;
			break;
		case TDMS_Writer_Adding_Initial_Channels:
			break;
		case TDMS_Writer_Adding_New_Channels:
			break;
		case TDMS_Writer_Logging_Data:
			TDMS_End_Section();
			break;
		default:
			break;
		}

		fflush(TDMS_File);
		fclose(TDMS_File);
		Writer_State = TDMS_Writer_Uninitialized;
	}
}