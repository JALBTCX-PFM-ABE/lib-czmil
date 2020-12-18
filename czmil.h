
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! are being used by Doxygen to document the
    software.  Dashes in these comment blocks are used to create bullet lists.  The lack of
    blank lines after a block of dash preceeded comments means that the next block of dash
    preceeded comments is a new, indented bullet list.  I've tried to keep the Doxygen
    formatting to a minimum but there are some other items (like <br> and <pre>) that need
    to be left alone.  If you see a comment that starts with / * ! and there is something
    that looks a bit weird it is probably due to some arcane Doxygen syntax.  Be very
    careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#ifndef __CZMIL_H__
#define __CZMIL_H__

#ifdef  __cplusplus
extern "C" {
#endif


#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#if (defined _WIN32) && (defined _MSC_VER)
  #include <process.h>
#else
  #include <unistd.h>
#endif
#include <errno.h>
#include <signal.h>



#ifndef DOXYGEN_SHOULD_SKIP_THIS


  /*  Preparing for language translation using GNU gettext at some point in the future.  */

#define _(String) (String)
#define N_(String) String

  /*
#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)
  */



  /*  Windoze insanity.  Note that there may be nothing defined for any of these DLL values.  The CZMIL API is always
      built dynamically on Windows because it will be called from ENVI's IDL language.  While statically built libraries
      can be called from IDL it is problematic.  Of course, on Linux/UNIX we don't have to do anything to these ;-)  */

#ifdef CZMIL_DLL_EXPORT
#  define CZMIL_DLL __declspec(dllexport)
#else
#  ifdef _WIN32
#    ifdef CZMIL_STATIC
#      define CZMIL_DLL
#    else
#      define CZMIL_DLL __declspec(dllimport)
#    endif
#  else
#    define CZMIL_DLL
#  endif
#endif


#endif /*  DOXYGEN_SHOULD_SKIP_THIS  */


#include "czmil_macros.h"


  /*! \mainpage CZMIL Data Formats


      <br><br>\section disclaimer Disclaimer


      This is a work of the US Government. In accordance with 17 USC 105, copyright protection is not available for any work of
      the US Government.

      Neither the United States Government nor any employees of the United States Government, makes any warranty, express or
      implied, without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any
      liability or responsibility for the accuracy, completeness, or usefulness of any information, apparatus, product, or
      process disclosed, or represents that its use would not infringe privately-owned rights. Reference herein to any specific
      commercial products, process, or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
      constitute or imply its endorsement, recommendation, or favoring by the United States Government. The views and opinions
      of authors expressed herein do not necessarily state or reflect those of the United States Government, and shall not be
      used for advertising or product endorsement purposes.







      <br><br>\section introduction Introduction


      The Coastal Zone Mapping and Imaging LiDAR (CZMIL) system has the ability to collect a very large amount of data in a very
      short time. The system fires the laser 10,000 times per second and generates nine waveforms of up to CZMIL_MAX_PACKETS ten
      bit samples each for every shot. Each waveform can be processed to generate up to CZMIL_MAX_RETURNS per waveform with each
      return having a unique latitude, longitude, elevation, horizontal uncertainty, vertical uncertainty, classification, and
      status.  Due to the size of the data sets that are generated by the CZMIL system, the data must be stored in an internally
      compressed format.  Since the internally compressed records are variable length there must be an index file associated with
      the data files.  Throughout this document we will refer to data types from stdint.h.  For example int8_t or uint32_t.  The
      he names should be fairly easy to figure out, for example, uint32_t is an unsigned 32 bit integer while int16_t is a 16 bit
      integer.  If you are confused you can just search the interwebs for stdint.h.  When working on code in the CZMIL API always
      use these data types instead of native C data types (like unsigned int).  This helps to keep the API portable to different
      operating systems.

      At present there are four CZMIL files defined. These are the the CZMIL Waveform File (CWF), CZMIL Point File (CPF), the CZMIL
      SBET File (CSF), and the CZMIL Index File (CIF).






      <br><br>\section cwf CZMIL Waveform File (CWF) format description


      The CWF file consists of an ASCII header and variable length, compressed, bit-packed binary records. The header
      will contain tagged ASCII fields. The tags are enclosed in brackets (e.g. [HEADER SIZE]). The reason that we
      don't use XML for the header is that CZMIL files are going to be very large while the CZMIL headers are only
      a few tens of thousands of bytes long.  If a user (or an operating system) were to mistakenly think that a CZMIL
      file was an XML file and tried to open the file with an XML reader it would crash, but usually not before it hangs
      the system for a while.  The other reason for not using XML is that XML is really designed for more complicated data.
      It also usually requires a schema and/or external information. We don't really need all that complexity.  Some of
      the fields that appear in the header, such as bit lengths, are used internally in packing and unpacking the records.
      These fields are not available to the application programmer in the CZMIL_CWF_Header structure.  The easiest way to
      understand what is stored in the header is to look at the CZMIL_CWF_Header structure in czmil.h.  The following is
      an example header:

      <pre>
      [VERSION] = CZMIL library V0.10 - 07/02/12
      [FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Waveform File
      [CREATION DATE] = 2012 07 18 (200) 14:09:02
      [CREATION TIMESTAMP] = 1342620542000000
      [MODIFICATION DATE] = 2012 07 18 (200) 14:09:02
      [MODIFICATION TIMESTAMP] = 1342620542000000
      [NUMBER OF RECORDS] = 392491
      [HEADER SIZE] = 65536
      [FILE SIZE] = 725267435
      [SYSTEM TYPE] = 1
      [SYSTEM NUMBER] = 1
      [SYSTEM REP RATE] = 10000
      [PROJECT] = Unknown
      [MISSION] = CZ01MD12001_P_120427_1347
      [DATASET] = CZ01MD12001_P_120427_1347_C
      [FLIGHT ID] = 00003
      [FLIGHT START DATE] = 2012 04 27 (118) 13:53:40
      [FLIGHT START TIMESTAMP] = 1335534820321983
      [FLIGHT END DATE] = 2012 04 27 (118) 13:54:20
      [FLIGHT END TIMESTAMP] = 1335534859540279

      ########## [FORMAT INFORMATION] ##########

      [BUFFER SIZE BYTES] = 2
      [TYPE BITS] = 3
      [TYPE 1 START BITS] = 10
      [TYPE 2 START BITS] = 10
      [DELTA BITS] = 4
      [CZMIL MAX PACKETS] = 15
      [TIME BITS] = 31
      [ANGLE SCALE] = 10000.000000
      [SCAN ANGLE BITS] = 22
      [PACKET NUMBER BITS] = 7
      [RANGE BITS] = 10
      [RANGE SCALE] = 16.000000
      [SHOT ID BITS] = 25
      [VALIDITY REASON BITS] = 4

      ########## [APPLICATION DEFINED FIELDS] ##########

      [SECURITY KEY] = 100

      ########## [END OF HEADER] ##########
      </pre>

      Note that application defined fields can be added to the header using czmil_add_field_to_cwf_header.  These fields
      are ignored by the API but are preserved when the header is modified in any way.  The application defined fields
      can be queried using czmil_get_field_from_cwf_header and modified using czmil_update_field_in_cwf_header.<br><br>


      Here's the fun part.  How are we actually compressing the waveform data?  It's not really all that important that
      the application programmer actually understands how the waveform data is compressed or how it is stored on disk
      but, in the interest of completeness, the following is an explanation:

      The data will be compressed using first or second difference coding or, in the case of the shallow channels,
      using a difference from the central shallow channel (channel[0]).  The data will be preceded by the full block buffer
      size.  After that will come the packet numbers, the MCWP ranges, the compressed 64 sample packets, and then the rest
      of the shot information (timestamp, shot_id, etc).  First, we need to look at how we compress a single, 64 sample 
      waveform packet.  The compressed 64 sample waveform packets will always (with the exception of the T0 packet) have a 3
      bit header containing the compression type.   Following that will be either the uncompressed, bit packed 10 bit waveform
      values (for compression type 0) or more header information, the contents of which depend on the type of compression
      used.  This will be followed by either the bit-packed first differences, the bit-packed second differences, or the
      bit-packed differences from the central shallow channel depending on the compression type.  We know that the count of
      values in a packet is always 64 and the starting value can't exceed 10 bits.  We're making the assumption that 99.9%
      of the time there are no NULL (i.e. 0) values in the waveforms.  This greatly simplifies compressing with first and
      second differences.  In the very few cases where we have NULL values amongst valid data we'll probably waste a little
      space but it should make compressing and decompressing faster since we will be eliminating a bunch of "if" statements.
      In the following definition the tag in brackets (]) is used as a comment in the code for the referenced field.
      That way you can search the code for the tag (e.g. [CWF:3]) to find the code that writes and reads the referenced
      field.  Tags with a dash (-) are looped fields (e.g. [CWF:10-2]).


      The following is the structure of a single compressed 64 sample waveform packet based on the compression type used:

      - Compression type 0 (ten bit, bit-packed values):
               - 64 * 10 bits            =    Waveform values

      - Compression type 1 (first difference coded):
               - type_1_start_bits       =    Starting value
               - type_1_offset_bits      =    Offset value (offset by 2^type_1_offset_bits - 1)
               - delta_bits              =    (DBS) Delta bit size
               - 63 * DBS                =    Difference from previous value + offset

      - Compression type 2 (second difference coded):
               - type_1_start_bits       =    Starting value
               - type_2_start_bits       =    Starting first difference value + first difference offset
               - type_1_offset_bits      =    First difference offset value (offset by 2^type_1_offset_bits - 1)
               - type_2_offset_bits      =    Second difference offset value (offset by 2^type_2_offset_bits - 1)
               - delta_bits              =    (DBS) Delta bit size
               - 62 * DBS                =    Difference from previous first difference value + second difference offset

      - Compression type 3 (shallow channel difference coded):
               - type_3_offset_bits      =    Offset value (offset by 2^type_3_offset_bits - 1 : based on compression type 1 starting value bits)
               - delta_bits              =    (DBS) Delta bit size
               - 64 * DBS                =    Difference from the corresponding value in the central shallow channel (channel[0])

      - Compression types 4 through 7 (future use)


      The number of bits used for delta values (DBS) is computed in czmil_compress_cwf_record in the czmil.c file.  This 
      is fairly simple to understand but somewhat hard to implement ;-)

      What follows the compression type is dependent on the compression type.  For type 0 compression it will be
      followed by 64 samples stored in 10 bits each.

      For compression type 1 it will be followed by 10 bits that contain the starting value for the difference values.
      Then 11 bits containing the offset value used to zero base the difference values.  Next is 4 bits containing the
      number of bits (first difference delta bits) used to store each difference value.  Then 63 fields of first
      difference delta bits that contains the difference from the previous value in the packet offset by the offset
      value.

      For compression type 2 it gets even more confusing.  We start with 10 bits that contain the starting value for
      the first difference values.  Then 10 bits that contain the starting value for the second difference values.
      Then 11 bits containing the offset value used to zero base the first difference values.  Next is 11 bits
      containing the offset value used to zero base the second difference values.  That is followed by 4 bits
      containing the number of bits (second difference delta bits) used to store each second difference value.
      Finally, 62 fields of second difference delta bits that contains the difference from the previous first
      difference value in the packet offset by the second difference offset value.

      For compression type 3 it's a bit simpler.  We start with 11 bits that contain the offset value used to zero base
      the difference values.  Following that we have 4 bits containing the number of bits (channel difference delta
      bits) used to store each difference value.  Then 64 fields of channel difference delta bits of differences from 
      the central shallow channel.

      We assume that the first compression method (bit-packed ten bit values) occurs rarely since any of the other
      compression schemes will, almost invariably, give us better results.

      Now that we know how each packet will be compressed we can look at the full waveform block for a single shot.
      This is the structure of a full waveform data block:

      - cwf_buffer_size_bytes * 8        =    [CWF:0] Actual buffer size (in bytes) of the entire CWF record (including this)

      - num_packets_bits                 =    [CWF:1-0] (NPKTS1) Number of packets for the first channel (CZMIL_SHALLOW_CHANNEL_1)
      - NPKTS1 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS1 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS1 CZMIL_SHALLOW_CHANNEL_1 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS2) Number of packets for the second channel (CZMIL_SHALLOW_CHANNEL_2)
      - NPKTS2 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS2 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS2 CZMIL_SHALLOW_CHANNEL_2 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS3) Number of packets for the third channel (CZMIL_SHALLOW_CHANNEL_3)
      - NPKTS3 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS3 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS3 CZMIL_SHALLOW_CHANNEL_3 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS4) Number of packets for the fourth channel (CZMIL_SHALLOW_CHANNEL_4)
      - NPKTS4 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS4 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS4 CZMIL_SHALLOW_CHANNEL_4 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS5) Number of packets for the fifth channel (CZMIL_SHALLOW_CHANNEL_5)
      - NPKTS5 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS5 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS5 CZMIL_SHALLOW_CHANNEL_5 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS6) Number of packets for the sixth channel (CZMIL_SHALLOW_CHANNEL_6)
      - NPKTS6 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS6 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS6 CZMIL_SHALLOW_CHANNEL_6 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS7) Number of packets for the seventh channel (CZMIL_SHALLOW_CHANNEL_7)
      - NPKTS7 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS7 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS7 CZMIL_SHALLOW_CHANNEL_7 compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS8) Number of packets for the eighth channel (CZMIL_IR_CHANNEL)
      - NPKTS8 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS8 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS8 CZMIL_IR_CHANNEL compressed 64 sample waveform packets [CWF:3]

      - num_packets_bits                 =    [CWF:1-0] (NPKTS9) Number of packets for the ninth channel (CZMIL_DEEP_CHANNEL)
      - NPKTS9 times packet_number_bits  =    [CWF:1-1] Packet numbers for each packet
      - NPKTS9 times range_bits          =    [CWF:1-2] Scaled integer ranges for each packet
      - type_bits                        =    [CWF:2] Compression type
      - NPKTS9 CZMIL_DEEP_CHANNEL compressed 64 sample waveform packets [CWF:3]

      - T0 compressed 64 sample waveform packet (this is always first difference compressed, AKA compression type 1, so we skip
        the compression type) [CWF:4]

      - shot_id_bits                     =    [CWF:5] Shot ID from MCWP
      - time_bits                        =    [CWF:6] Timestamp as an offset from the start timestamp in the header
      - scan_angle_bits                  =    [CWF:7] Scaled and offset scan angle
      - 9 times validity_reason_bits     =    [CWF:8] Per channel waveform validity reasons


      IMPORTANT NOTE: The "bits" values listed above (e.g. num_packets_bits) are based on default values that are
      contained in the czmil_internals.h file.  These may change over time so they (or the information used to
      create them) are stored in the header (e.g. num_packets_bits is based on [CZMIL_MAX_PACKETS]).  To see how
      each of those fields is computed you can look at the czmil_read_cwf_header function in czmil.c.

      So, how big is a CWF record in bytes?  It depends on the compression type and the waveforms in each 64 sample
      packet.  Simply put, it is the number of bits written divided by 8.  If the number of bits written is not an even
      multiple of 8 then one byte will be added to the record size.  Luckily, this is all handled internally and you
      shouldn't ever have to worry about it.  Just call czmil_read_cwf_record and it will hand you an unpacked
      CZMIL_CWF_Data structure. The record structure for the CZMIL CWF records, CZMIL_CWF_Data, is very simple (as
      opposed to how it is stored on disk).

      From the application programmer's point of view there are only two things that need to be understood.  These are the
      CZMIL_WAVEFORM_RAW_Data structure and the CZMIL_CWF_Data structure.  The CZMIL_WAVEFORM_RAW_Data structure is used
      when writing the CWF data to disk.  Only HydroFusion will ever be doing that.  The CZMIL_CWF_Data structure is the
      structure that is returned from czmil_read_cwf_record.








      <br><br>\section cpf CZMIL Point File (CPF) format description


      The CPF file consists of an ASCII header and variable length, compressed, bit-packed binary records.  The header
      will contain tagged ASCII fields. The tags are enclosed in brackets (e.g. [HEADER SIZE]) for single line values
      and in braces (e.g. {WELL-KNOWN TEXT = }) for multi-line values.  Some of the fields that appear in the header,
      such as bit lengths, are used internally in packing and unpacking the records.  These fields are not available to
      the application programmer in the CZMIL_CPF_Header structure.  The easiest way to understand what is stored in the
      header is to look at the CZMIL_CPF_Header structure in czmil.h.  The following is an example header:

      <pre>
      [VERSION] = CZMIL library V3.00 - 06/27/16
      [FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Point File
      [CREATION DATE] = 2012 07 18 (200) 14:13:47
      [CREATION TIMESTAMP] = 1342620827000000
      [MODIFICATION DATE] = 2012 07 18 (200) 14:32:19
      [MODIFICATION TIMESTAMP] = 1342621939000000
      [MIN LONGITUDE] = -89.45807045740
      [MIN LATITUDE] = 30.36534822506
      [MAX LONGITUDE] = -89.44589052747
      [MAX LATITUDE] = 30.37732910499
      [BASE LONGITUDE] = -89.44923180745
      [BASE LATITUDE] = 30.37732910499
      [NUMBER OF RECORDS] = 300000
      [HEADER SIZE] = 65536
      [FILE SIZE] = 33078716
      [SYSTEM TYPE] = 1
      [SYSTEM NUMBER] = 1
      [SYSTEM REP RATE] = 10000
      [PROJECT] = Unknown
      [MISSION] = CZ01MD12001_P_120427_1347
      [DATASET] = CZ01MD12001_P_120427_1347_C
      [FLIGHT ID] = 00003
      [FLIGHT START DATE] = 2012 04 27 (118) 13:53:40
      [FLIGHT START TIMESTAMP] = 1335534820321983
      [FLIGHT END DATE] = 2012 04 27 (118) 13:54:10
      [FLIGHT END TIMESTAMP] = 1335534850298414
      [NULL Z VALUE] = 99999.00000
      {WELL-KNOWN TEXT =
      COMPD_CS["WGS84 with ellipsoid Z",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9108"]],AXIS["Lat",NORTH],AXIS["Long",EAST],AUTHORITY["EPSG","4326"]],VERT_CS["ellipsoid Z in meters",VERT_DATUM["Ellipsoid",2002],UNIT["metre",1],AXIS["Z",UP]]]
      }
      [LOCAL VERTICAL DATUM] = 00
      {COMMENTS =
      Comments on the file contents.
      }
      {USER DATA DESCRIPTION =
      Description of the way the user_data field of the CPF record is being used.
      }

      ########## [FORMAT INFORMATION] ##########

      [BUFFER SIZE BYTES] = 2
      [CZMIL MAX RETURNS] = 31
      [TIME BITS] = 31
      [ANGLE SCALE] = 10000.000000
      [OFF NADIR ANGLE BITS] = 21
      [LAT BITS] = 27
      [LON BITS] = 27
      [LAT SCALE] = 72000000.000000
      [LON SCALE] = 72000000.000000
      [LAT DIFF SCALE] = 36000000.000000
      [LON DIFF SCALE] = 36000000.000000
      [LAT DIFF BITS] = 18
      [LON DIFF BITS] = 18
      [ELEV BITS] = 22
      [ELEV SCALE] = 1000.000000
      [UNCERT BITS] = 14
      [UNCERT SCALE] = 1000.000000
      [REFLECTANCE BITS] = 14
      [REFLECTANCE SCALE] = 10000.000000
      [RETURN STATUS BITS] = 8
      [VALIDITY REASON BITS] = 8
      [RETURN CLASS BITS] = 8
      [CZMIL MAX PACKETS] = 15
      [INTEREST POINT SCALE] = 10.000000
      [KD BITS] = 14
      [KD SCALE] = 10000.000000
      [LASER ENERGY BITS] = 16
      [LASER ENERGY SCALE] = 10000.000000
      [PROBABILITY SCALE] = 10000.000000
      [PROBABILITY BITS] = 14
      [USER DATA BITS] = 8
      [RETURN FILTER REASON BITS] = 6
      [OPTECH CLASSIFICATION BITS] = 6
      [IP_RANK BITS] = 1
      [D_INDEX BITS] = 10
      [D_INDEX_CUBE BITS] = 10

      ########## [APPLICATION DEFINED FIELDS] ##########

      [SECURITY KEY] = 100

      ########## [END OF HEADER] ##########
      </pre>

      Looking at the documentation for the CZMIL_CPF_Header structure, the header key fields, in parentheses, defined for
      each member define how and when each field is populated.  The (a) key means that the field is populated and/or
      modified by the API and does not need to be set by the application program.  A key of (c) means that field needs to
      be set by the application program prior to creation of the file.  An (m) key indicates that the field is modifiable
      at creation time or later using the czmil_update_cpf_header function.  The modifiable fields are all optional with
      the exception of the local_vertical_datum field.  The local_vertical_datum field will be set to 0
      (CZMIL_V_DATUM_UNDEFINED) by default.  The wkt and the comments field are multi-line text fields. That is, they can
      contain line-feeds.  Of the CZMIL_CPF_Header fields that are needed for creation of the file only the null_z_value
      is actually required.  The wkt field is provided so that application programs can warp the data from the datums that
      the data was collected in (e.g. WGS84/WGS84E) to other datums.  The string will usually be the same as the one in
      the example header, above, but may be different.<br><br>


      Each record in the CPF file is stored in a bit-packed, variable length record.  The values in each record are
      physically stored as unsigned, scaled integers.  They are packed and unpacked by the API and the actual format of
      each value is defined by the extra bit, scale, and/or offset values stored in the header.  We could probably get
      better compression but this is reasonable, simple, and the CWF file is really the one to worry about in terms of
      size.  How the values are stored is really irrelevant to the application programmer.  All that is required is to use
      the czmil_read_cpf_record, czmil_update_cpf_record, czmil_update_cpf_return_status, and/or czmil_write_cpf_record
      functions to read, update, or write (HydroFusion only) the contents of the CZMIL CPF record.  The CPF record is
      defined in the CZMIL_CPF_Data structure.  Unfortunately, the CZMIL_CPF_Data structure contains a CZMIL_Return_Data
      structure.  You can look at the czmil.h file to get a full picture of the structures.

      Note that application defined fields can be added to the header using czmil_add_field_to_cpf_header.  These fields
      are ignored by the API but are preserved when the header is modified in any way.  The application defined fields
      can be queried using czmil_get_field_from_cpf_header and modified using czmil_update_field_in_cpf_header.<br><br>

      In the following definition the values NRET1 through NRET9 represent the number of returns for shallow channels
      1 through 7, the IR channel, and the deep channel.  In the code these are referenced as channels 0 through 8 since
      it's much easier to deal with arrays that start at 0.  The return_bits value is computed from the [CZMIL MAX RETURNS]
      header field.  In ther words, if the value is 31 then return_bits is 5.  The tag in brackets (]) is used as a comment
      in the code for the referenced field.  That way you can search the code for the tag (e.g. [CPF:3]) to find the code
      that writes and reads the referenced field.  Tags with a dash (-) are looped fields (e.g. [CPF:10-2]).
      

      Once again, in the interest of completeness, here is a simple look at how the data is actually stored on the
      disk:

      - cpf_buffer_size_bytes * 8        =    [CPF:0] Actual buffer size (in bytes) of the entire CPF record (including this)
      - 9 times return_bits              =    [CPF:1] (NRET1 - NRET9) Number of returns per channel
      - time_bits                        =    [CPF:2] Timestamp as an offset from the start timestamp in the header
      - off_nadir_angle_bits             =    [CPF:3] Scaled and offset off nadir angle
      - lat_bits                         =    [CPF:4] Reference latitude (latitude of the first encountered return) as a difference
                                              from the base latitude in the header (in 20,000ths of an arcsecond)
      - lon_bits                         =    [CPF:5] Reference longitude (longitude of the first encountered return) as a difference
                                              from the base latitude in the header (in 20,000ths of an arcsecond times the
                                              cosine of the latitude degrees)
      - elev_bits                        =    [CPF:6] Scaled, offset water level
      - elev_bits                        =    [CPF:7] Scaled, offset vertical datum offset (this is normally 0.0 until a post
                                              processing program adds the value)
      - user_data_bits                   =    [CPF:8] User data - this was originally (in v1) the shot status but it was never used.  In
                                              v2 it was changed to "spare" but also wasn't used.  We have now decided to make this an
                                              application defined user data field.  It can hold an 8 bit integer.  Since the previous
                                              instantiations of this field weren't used even though the storage was used, the API will
                                              back fill them if an application requests it.


      - NRET1 times the following block [CPF:9]
          - lat_diff_bits                    =    [CPF:9-0] Return latitude as a difference from the reference latitude (in
                                                  10,000ths of an arcsecond)
          - lon_diff_bits                    =    [CPF:9-1] Return longitude as a difference from the reference longitude (in
                                                  10,000ths of an arcsecond times the cosine of the latitude degrees)
          - elev_bits                        =    [CPF:9-2] Scaled, offset return elevation
          - reflectance_bits                 =    [CPF:9-3] Scaled, offset reflectance
          - uncert_bits                      =    [CPF:9-4] Scaled, offset horizontal uncertainty
          - uncert_bits                      =    [CPF:9-5] Scaled, offset vertical uncertainty
          - return_status_bits               =    [CPF:9-6] Integer return status bit flags
          - class_bits                       =    [CPF:9-7] Return classification
          - interest_point_bits              =    [CPF:9-8] Interest point value (point on the waveform that this return was computed
                                                  from)
          - ip_rank_bits                     =    [CPF:9-9] Interest point rank (now defined as non-water surface return flag, see
                                                  CZMIL_Return_Data structure for explanation)


      - NRET2 through NRET9 times the above block definition


      - 7 times the following block (for shallow channels 0 through 7) [CPF:10]
          - lat_diff_bits                    =    [CPF:10-0] Bare earth latitude as a difference from the reference latitude (in
                                                  10,000ths of an arcsecond) 
          - lon_diff_bits                    =    [CPF:10-1] Bare earth longitude as a difference from the reference latitude (in
                                                  10,000ths of an arcsecond) 
          - elev_bits                        =    [CPF:10-2] Scaled, offset bare earth elevation

      - kd_bits                          =    [CPF:11] Scaled, offset KD value
      - laser_energy_bits                =    [CPF:12] Scaled, offset laser energy value
      - interest_point_bits              =    [CPF:13] T0 interest point


      **************************************************************** V2.0 additions *******************************************************

      - 9 times the following block [CPF:14]
          - optech_classification_bits   =    [CPF:14-0] The Optech waveform procesing mode (e.g. land, water, shallow water, turbid water, etc,
                                              see czmil_macros.h)
          - NRET1 times the following block
              - probability_bits                 =    [CPF:14-1] Probability of detection (TBD)
              - return_filter_reason_bits        =    [CPF:14-2] Return filter reason (TBD)


              - NRET2 through NRET9 times the above block definition


      **************************************************************** V3.0 additions *******************************************************

      - d_index_cube_bits                =    [CPF:15] Prediction of the normalized signal amplitude from a cube on the seafloor (deep channel).

      - 9 times the following block
          - NRET1 times the following block [CPF:16]
              - d_index_bits                     =    [CPF:16-0] Noise normalized signal amplitude for the related interest point


              - NRET2 through NRET9 times the above block definition


      IMPORTANT NOTE: The "bits" values listed above (e.g. lat_bits) are based on default values that are
      contained in the czmil_internals.h file.  These may change over time so they (or the information used to
      create them) are stored in the header (e.g. [LAT_BITS]).  To see how each of those fields is computed you can
      look at the czmil_read_cpf_header function in czmil.c.

      As with the CWF record, the actual size in bytes of a CPF record as it is stored on disk is dependent on the contents
      of the CPF record.  In this case mostly on the number of returns for each channel.

      The application programmer need only understand the CZMIL_CPF_Data and CZMIL_Return_Data structures.  As with the CWF
      and CSF data, only HydroFusion will be creating CPF files.  Unlike CWF and CSF data, an application can update a CPF
      record by writing it back to it's original location.  The only values that should be modified by an application are
      the local_vertical_datum_offset, the per return status, the classification, the reflectance, the horizontal uncertainty,
      and the vertical uncertainty.  Only the HydroFusion software should be using the czmil_write_cpf_record function since
      it will replace fields that shouldn't be modified by an application (e.g. return latitude).  All other applications
      should use the czmil_update_cpf_record function.








      <br><br>\section csf CZMIL SBET File (CSF) format description


      Finally, something fairly simple, a file with fixed length records.  It's still bit-packed though. The CZMIL Smoothed,
      Best Estimated Trajectory (SBET) File (CSF) consists of an ASCII header and variable length, compressed, bit-packed
      binary records. The header will contain tagged ASCII fields. The tags are enclosed in brackets (e.g. [HEADER SIZE]).
      Some of the fields that appear in the header, such as bit lengths, are used internally in packing and unpacking the
      records. These fields are not available to the application programmer in the CZMIL_CSF_Header structure. The easiest
      way to understand what is stored in the header is to look at the CZMIL_CSF_Header structure in czmil.h.  The following
      is an example header:

      <pre>
      [VERSION] = CZMIL library V0.10 - 07/02/12
      [FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) SBET File
      [CREATION DATE] = 2012 07 18 (200) 14:13:37
      [CREATION TIMESTAMP] = 1342620817000000
      [MODIFICATION DATE] = 2012 07 18 (200) 14:13:37
      [MODIFICATION TIMESTAMP] = 1342620817000000
      [NUMBER OF RECORDS] = 392491
      [HEADER SIZE] = 65536
      [FILE SIZE] = 13802721
      [PROJECT] = Unknown
      [MISSION] = CZ01MD12001_P_120427_1347
      [DATASET] = CZ01MD12001_P_120427_1347_C
      [FLIGHT ID] = 00003
      [FLIGHT START DATE] = 2012 04 27 (118) 13:53:40
      [FLIGHT START TIMESTAMP] = 1335534820321983
      [FLIGHT END DATE] = 2012 04 27 (118) 13:54:20
      [FLIGHT END TIMESTAMP] = 1335534859540279
      [MIN LONGITUDE] = -89.45648587195
      [MIN LATITUDE] = 30.36708618595
      [MAX LONGITUDE] = -89.44304172489
      [MAX LATITUDE] = 30.37732838127
      [BASE LONGITUDE] = -89.44304172489
      [BASE LATITUDE] = 30.37732838127

      ########## [FORMAT INFORMATION] ##########

      [TIME BITS] = 31
      [LAT BITS] = 26
      [LON BITS] = 26
      [LAT SCALE] = 36000000.000000
      [LON SCALE] = 36000000.000000
      [ALTITUDE SCALE] = 1000.000000
      [ALTITUDE BITS] = 22
      [ANGLE SCALE] = 10000.000000
      [SCAN ANGLE BITS] = 22
      [ROLL AND PITCH BITS] = 20
      [HEADING BITS] = 22
      [RANGE BITS] = 10
      [RANGE SCALE] = 16.000000

      ########## [APPLICATION DEFINED FIELDS] ##########

      [SECURITY KEY] = 100

      ########## [END OF HEADER] ##########
      </pre>


      Looking at the documentation for the CZMIL_CSF_Header structure, the header key fields, in parentheses, defined
      for each member define how and when each field is populated.  The (a) key means that the field is populated
      and/or modified by the API and does not need to be set by the application program.  A key of (c) means that
      field needs to be set by the application program prior to creation of the file.  An (m) key indicates that the
      field is modifiable at creation time or later using the czmil_update_csf_header function.  The modifiable
      fields are all optional.

      Note that application defined fields can be added to the header using czmil_add_field_to_csf_header.  These fields
      are ignored by the API but are preserved when the header is modified in any way.  The application defined fields
      can be queried using czmil_get_field_from_csf_header and modified using czmil_update_field_in_csf_header.<br><br>


      Each record in the CSF file is stored in a bit-packed, fixed length record.  The values in each record are
      physically stored as unsigned, scaled integers.  They are packed and unpacked by the API and the actual format of
      each value is defined by the extra bit, scale, and/or offset values stored in the header.  How they are stored
      is really irrelevant to the application programmer.  All that is required is to use the czmil_write_csf_record
      and/or czmil_read_csf_record functions to write or read the contents of the CZMIL CSF record.  The CSF record
      is defined in the CZMIL_CSF_Data structure.  You can look at the czmil.h file to get a full picture of the
      structure.

      In the following definition the tag in brackets (]) is used as a comment in the code for the referenced field.
      That way you can search the code for the tag (e.g. [CSF:3]) to find the code that writes and reads the referenced
      field.

      Once again, in the interest of completeness, here is a simple look at how the data is actually stored on the
      disk:

      - time_bits                        =    [CSF:0] Timestamp as an offset from the start timestamp in the header
      - scan_angle_bits                  =    [CSF:1] Scaled and offset scan angle
      - lat_bits                         =    [CSF:2] Latitude as a difference from the base latitude in the header (in
                                              10,000ths of an arcsecond)
      - lon_bits                         =    [CSF:3] Longitude as a difference from the base latitude in the header (in
                                              10,000ths of an arcsecond times the cosine of the latitude degrees)
      - elev_bits                        =    [CSF:4] Altitude as a scaled, offset value
      - roll_pitch_bits                  =    [CSF:5] Scaled, offset roll value
      - roll_pitch_bits                  =    [CSF:6] Scaled, offset pitch value
      - heading_bits                     =    [CSF:7] Scaled, offset heading value
      - 9 times range_bits               =    [CSF:8] Scaled, offset range values per channel


      The following are V2.0 additions:

      - 9 times range_bits               =    [CSF:9] Scaled ranges in water per channel
      - 9 times intensity_bits           =    [CSF:10] Scaled intensities per channel
      - 9 times intensity_bits           =    [CSF:11] Scaled intensities in water per channel


      IMPORTANT NOTE: The "bits" values listed above (e.g. lat_bits) are based on default values that are
      contained in the czmil_internals.h file.  These may change over time so they (or the information used to
      create them) are stored in the header (e.g. [LAT_BITS]).  To see how each of those fields is computed you can
      look at the czmil_read_cpf_header function in czmil.c.

      Since the CSF record is a fixed length record we can actually compute the size of the record using the default
      bit sizes from the czmil_internals.h file.  With the current bit field sizes it should be:

      - (TIME_BITS + CSF_LAT_BITS + CSF_LON_BITS + CSF_ALTITUDE_BITS + SCAN_ANGLE_BITS + CSF_HEADING_BITS +
        2 * CSF_ROLL_PITCH_BITS + 9 * CSF_RANGE_BITS
      - (31 + 26 + 26 + 22 + 22 + 22 + 2 * 20 + 9 * 10) / 8
      - 279 / 8
      - 34.875
      - 35 bytes

      Like the CWF data, only HydroFusion will ever write a CSF record.  This will be done using the CZMIL_CSF_Data structure.
      There are no modifiable fields in the CSF structure and any attempt to append to the CSF file after its initial
      creation will result in an error.  The only operation that is important to the application programmer is the 
      czmil_read_csf_record function.









      <br><br>\section cif CZMIL Index File (CIF/CWI) format description


      The CIF file is created and modified exclusively by the API.  There are no modifiable fields and no public
      functions that interact with the CIF file.  The CIF file is created automatically by the API as the CWF or 
      CPF file is created.  When Optech creates the CWF file from the raw waveform data the file will have the
      .cwi extension.  This is the CZMIL Waveform Index file and it is in the exact same format as the final CIF
      file except that the CPF addresses and buffer sizes are set to zero.  When Optech creates the CPF file the
      CWI file will be read and a new CIF file will be created with the CPF addresses and buffer sizes populated.
      If the CIF file is ever lost or accidentally deleted, the API will regenerate it the next time the associated
      CWF or CPF file is opened.  Like the other files, it has a tagged ASCII header.  Unlike the other files,
      application defined fields are not supported since the CIF file doesn't contain anything other than buffer
      sizes and byte addresses.  The following is an example header:

      <pre>
      [VERSION] = CZMIL library V0.10 - 07/02/12
      [FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Index File
      [CREATION DATE] = 2012 07 18 (200) 14:13:47
      [CREATION TIMESTAMP] = 1342620827000000
      [NUMBER OF RECORDS] = 392491
      [HEADER SIZE] = 16384

      ########## [FORMAT INFORMATION] ##########

      [CPF ADDRESS BITS] = 48
      [CWF ADDRESS BITS] = 48
      [CPF BUFFER SIZE BITS] = 16
      [CWF BUFFER SIZE BITS] = 16

      ########## [END OF HEADER] ##########
      </pre>


      The CZMIL_CIF_Data records, consisting of just four fields, are really simple.  It's pretty easy to see how the
      API uses the CIF file to find the associated CWF and CPF records.  It just multiples the record number requested
      by the size of the CIF record, adds the CIF header_size, moves to that location in the CIF file, reads the CIF
      record, and then moves to cwf_address or cpf_address in the associated file and reads cwf_buffer_size or
      cpf_record_size bytes.  The bytes read are then unpacked and uncompressed and handed back to the application as
      a simple CWF or CPF structure.  The overhead of reading the CIF record is the price we must pay to get decent
      compression of both the waveforms and the point cloud.  By compressing the data by even a modest two to one ratio
      we more than make up for the time used to read the CIF record and the time used to uncompress the record.

      The records in the CIF file are bit-packed, fixed length records that consist of byte addresses and buffer sizes
      for the correspondfing records in the CWF and CPF files.  The contents of each record are as follows:

      - CWF_ADDRESS_BITS                 =    Address of the CWF record in the CWF file
      - CPF_ADDRESS_BITS                 =    Address of the CPF record in the CPF file
      - CWF_BUFFER_SIZE_BITS             =    Size of bit-packed CWF record buffer in the CWF file
      - CPF_BUFFER_SIZE_BITS             =    Size of bit-packed CPF record buffer in the CPF file

      The record size in bytes using the present default bit sizes would be:

      - (CIF_CWF_ADDRESS_BITS + CIF_CPF_ADDRESS_BITS + CIF_CWF_BUFFER_SIZE_BITS + CIF_CPF_BUFFER_SIZE_BITS) / 8
      - (48 + 48 + 16 + 16) / 8
      - 128 / 8
      - 16 bytes

      From an application programmer's point of view, the CIF file is completely transparent.  There is no reason for 
      an application program to have anything to do with the CIF file.  If the programmer is working on the API itself,
      that is another matter entirely.  In that case, the CZMIL_CIF_Data structure is the important data structure.










      <br><br>\section caf CZMIL Audit File (CAF) format description


      The CAF file is an audit file that contains information on manually invalidated returns in the CPF file.  The
      CAF file is created by reading a CPF file that has been edited via the Area-Based Editor (ABE) or the CZMIL
      Manual Editor (CME) and writing the record/shot number, channel number, Optech classification, and interest
      point to the CAF file.  The CAF file is used to apply edits made in ABE and/or CME that have been unloaded to
      the CPF files to reprocessed CPF files.  CPF files may need to be reprocessed due to changes in the SBET
      processing or a perceived need to change the processing parameters.  After reprocessing the audits may be
      applied to the new CPF files by comparing the interest points of the manually invalidated points with the
      interest points of the reprocessed points and invalidating points that have the same Optech classification
      and are within a certain tolerance of the interest point.  While the CAF file is not CZMIL data it is, like
      the CIF file, metadata about the CZMIL data and is included in the API to control its format.  It also has a
      tagged ASCII header:

      <pre>
      [VERSION] = CZMIL library V2.10 - 09/04/14
      [FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Audit File
      [CREATION DATE] = 2012 07 18 (200) 14:13:47
      [CREATION TIMESTAMP] = 1342620827000000
      [APPLICATION DATE] = 2012 07 18 (200) 14:13:47
      [APPLICATION TIMESTAMP] = 1342620827000000
      [NUMBER OF RECORDS] = 392491
      [HEADER SIZE] = 16384

      ########## [FORMAT INFORMATION] ##########

      [SHOT ID BITS] = 25
      [CHANNEL BITS] = 4 (this will always be 4 since the number of CZMIL channels will never change)
      [OPTECH CLASSIFICATION BITS] = 6
      [CZMIL MAX RETURNS] = 31
      [INTEREST POINT SCALE] = 10
      [CZMIL MAX PACKETS] = 15

      ########## [END OF HEADER] ##########
      </pre>


      The CZMIL_CAF_Data records, like the CIF records, consist of just four fields.  The records in the CAF file
      are bit-packed and, in the case of the interest point, scaled integers.  The contents of each record are as
      follows:

      - SHOT ID BITS                     =    Record number
      - CHANNEL BITS                     =    Channel number (0-8)
      - OPTECH CLASSIFICATION BITS       =    Optech processing mode (0-63)
      - interest_point_bits              =    Interest point value scaled by INTEREST POINT SCALE
      - return_bits                      =    Return number
      - return_bits                      =    Number of returns

      The record size in bytes using the present default bit sizes would be:

      - (SHOT ID BITS + CHANNEL BITS + OPTECH CLASSIFICATION BITS + interest_point_bits) / 8
      - (25 + 4 + 6 + 10 + 5 + 5) / 8
      - 55 / 8
      - 6.875 bytes (rounded up to 7 bytes)









      <br><br>\section caveats CZMIL API Caveats


      Due to the way we are compressing the data there are some limitations on the input data.  These shouldn't be
      big problems but it's good to know about them up front.

      - Flight lines, and therefor files, cannot be longer than 35 minutes in duration.
      - Flight lines cannot exceed 104 kilometers in length.
      - Due to the fact that the CZMIL system does not allow surveying across the International Dateline there is
        no provision to handle it in the API.  There are very few places on earth where this will be a problem.







      <br><br>\section example Example code


      Since only HydroFusion will be creating CZMIL files the following example code covers reading the records,
      updating the CPF records, and updating the headers.  Note that we're using /# and #/ instead of the normal
      C comment markers for (hopefully) obvious reasons.  In this example we are modifying the local vertical datum
      offset using the czmil_update_cpf_record function.  If we only wanted to modify the return status values
      (something that is fairly common) we should use the czmil_update_cpf_return_status function since it would
      be slightly faster and wouldn't have the effect of possibly aliasing any of the other values.


      <pre>

      int32_t main (int32_t argc, int32_t *argv[])
      {
        CZMIL_CPF_Header     cpf_header;
        CZMIL_CPF_Data       cpf_record;
        CZMIL_CWF_Header     cwf_header;
        CZMIL_CWF_Data       cwf_record;
        CZMIL_CSF_Header     csf_header;
        CZMIL_CSF_Data       csf_record;
        int32_t              i, cwf_hnd, cpf_hnd, csh_hnd;
        char                 cpf_path[1024], cwf_path[1024], csf_path[1024];


        strcpy (cpf_path, argv[1]);


        if ((cpf_hnd = czmil_open_cpf_file (cpf_path, &cpf_header, CZMIL_UPDATE) < 0)
          {
            czmil_perror ();
            exit (-1);
          }


        /#  Open CWF and CSF for update since we're going to modify their headers.  #/

        strcpy (cwf_path, cpf_path);
        sprintf (&cwf_path[strlen (cwf_path) - 4], ".cwf");

        if ((cwf_hnd = czmil_open_cwf_file (cwf_path, &cwf_header, CZMIL_UPDATE) < 0)
          {
            czmil_perror ();
            exit (-1);
          }

        strcpy (csf_path, cpf_path);
        sprintf (&csf_path[strlen (csf_path) - 4], ".csf");

        if ((csf_hnd = czmil_open_csf_file (csf_path, &csf_header, CZMIL_UPDATE) < 0)
          {
            czmil_perror ();
            exit (-1);
          }


        for (i = 0 ; i < cpf_header.number_of_records ; i++)
          {
            if (czmil_read_cpf_record (cpf_hnd, i, &cpf_record) < 0 )
              {
                czmil_perror ();
                exit (-1);
              }

            if (czmil_read_cwf_record (cwf_hnd, i, &cwf_record) < 0)
              {
                czmil_perror ();
                exit (-1);
              }

            if (czmil_read_csf_record (csf_hnd, i, &csf_record) < 0)
              {
                czmil_perror ();
                exit (-1);
              }


            /#  We may want to check status of a return to see if we're going to use it.  #/

            if (!(czmil_cpf_return_is_invalid (cpf_record.channel[3][5].status)))
              {
                /#  Do something fun here... Maybe add a local_vertical_datum_offset value  #/

                cpf_record.local_vertical_datum_offset = some_magic_value;


                /#  If you changed the status or the data (in this case the vertical datum value) you will
                    need to update the CPF record.  #/

                if (czmil_update_cpf_record (cpf_hnd, i, &cpf_record) < 0)
                  {
                    czmil_perror ();
                    exit (-1);
                  }
              }


            /#  Do something with the CWF and CSF records...  #/

            WHATEVER;
          }


        /#  Since we added local_vertical_datum_offset values we need to modify the CPF header to reflect what the
            local_vertical_datum is.  #/

        cpf_header.local_vertical_datum = CZMIL_V_DATUM_MLLW;

        if (czmil_update_cpf_header (cpf_hnd, &cpf_header) < 0)
          {
            czmil_perror ();
            exit (-1);
          }


        /#  Add an application defined fields to the headers of each file.  #/

        if ((czmil_add_field_to_cwf_header (cwf_hnd, "SECURITY KEY", "100")) < 0)
          {
            czmil_perror ();
            exit (-1);
          }

        if ((czmil_add_field_to_cpf_header (cpf_hnd, "SECURITY KEY", "100")) < 0)
          {
            czmil_perror ();
            exit (-1);
          }

        if ((czmil_add_field_to_csf_header (csf_hnd, "SECURITY KEY", "100")) < 0)
          {
            czmil_perror ();
            exit (-1);
          }


        /#  Close the files.  #/

        czmil_close_cpf_file (cpf_hnd);
        czmil_close_cwf_file (cwf_hnd);
        czmil_close_csf_file (csf_hnd);
      }

      </pre><br><br>








      <br><br>\section threads Thread Safety


      The CZMIL I/O library is thread safe if you follow some simple rules.  First, it is only thread safe if you use unique CZMIL
      file handles per thread.  Also, the czmil_create_c*f, czmil_open_c*f, and czmil_close_c*f functions are not thread safe due
      to the fact that they assign or clear the CZMIL file handles.  In order to use the library in multiple threads you must open
      (or create) the CZMIL files prior to starting the threads and close them after the threads have completed.  Some common sense
      must be brought to bear when trying to create a multithreaded program that works with CZMIL files.  When you are creating
      files, threads should only work with one file.  So, for example, if you want to create 16 CWF files from 16 sets of raw data
      you would open all 16 new CWF files using czmil_create_cwf_file prior to starting your threads.  After that, each thread would
      append records to each of the files.  When the threads are finished you would use czmil_close_cwf_file to close each of the
      new CWF files.

      If you want multiple threads to read from multiple or single CZMIL files at the same time you must open each file for each
      thread to get a separate CZMIL file handle for each file and thread.  The static data in the CZMIL API is segregated by the 
      CZMIL file handle so there should be no collision problems.  Again, you have to open the files prior to starting your threads
      and close them after the threads are finished.

      In general, all of the public functions, with the exception of the create, opem, close, and error functions are thread safe.








      <br><br>\section complexity Memory, Speed, I/O, and complexity


      If you look at the CZMIL_CPF_Data structure, the CZMIL_Return_Data structure, and the CZMIL_CWF_Data structure you'll see 
      that they are built using static arrays.  Actually, they are built using <b>huge</b> static arrays.  If you want to read
      a large number of CPF records using the czmil_read_c?f_record_array functions you will have to allocate the memory for this.
      Given the size of the static arrays in the CPF and CWF records, this can eat up a large chunk of memory.  Since we will
      probably never see waveforms with CZMIL_MAX_PACKETS packets in every (or even any) of the waveform arrays or CZMIL_MAX_RETURNS
      in the CPF records, we're wasting memory space.

      The reason this was done was, first, to reduce complexity in the API.  Since the API is compressing and decompressing the
      waveforms and the point cloud using a number of different techniques it is already pretty complicated.  Adding
      allocation/deallocation of arrays based on the number of packets or the number of returns would have made it even worse.
      Rest assured that only the actual number of packets in the waveforms and the actual number of returns in the point cloud are
      stored on disk (after compression).  This reduces our I/O time by approximately half for the waveforms and some unknown amount
      for the point cloud data.

      The second reason for not allocating/deallocating the needed memory in the API is that memory allocation/deallocation causes a
      system wide mutex (mutually exclusive flag) to be raised.  If you are running multiple threads, any time the API would allocate
      or free a portion of memory, any other thread trying to allocate or free memory would be blocked until the first thread finished
      allocating or freeing memory.  This could cause your multithreaded application to be slower than it could be.

      In the big picture, memory is cheap.  So is disk storage usually.  But, when you're collecting on average about 40,960,000 ten
      bit samples per second for waveform data (51.2MB) and some unknown amount of ancillary data and point cloud data, disk storage
      becomes a lot less cheap.  It also becomes a lot heavier.  The bottom line is that we're trading more memory for higher speed
      and less complexity.








      <br><br>\section notes Documentation Notes


      The easiest way to find documentation for a particular public C function is by looking for it in the Doxygen documentation
      for the czmil.h file.  All of the publicly accessible functions are defined there.  Some other publicly accessible definitions
      are covered in the czmil_macros.h file (which is "#included" in the czmil.h file).  For the application programmer there should
      be nothing much else of interest (assuming everything is working properly).

      For anyone wanting to work on the API itself, the czmil.c file is where the action is.  The other file that is extremely
      important to the API is the czmil_internals.h file.  That is where all of the private definitions are kept.  If you would like
      to see a history of the changes that have been made to the CZMIL API you can look at czmil_version.h.

      <br><br>

  */


  /*!

      - CZMIL CWF file header structure.  Header key definitions are as follows:

          - (a) = Set or modified by the API at creation time or later (e.g. modification_timestamp)
          - (c) = Defined by the application program only at creation time
          - (m) = Modifiable by the application program either at creation time or later using the czmil_update_cwf_header function

  */

  typedef struct
  {                                                  /*   Key             Definition  */
    char                 version[128];               /*!< (a)      Library version information  */
    char                 file_type[128];             /*!< (a)      File type  (e.g. Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL)
                                                                   Waveform File)  */
    uint64_t             creation_timestamp;         /*!< (a)      File creation microseconds from 01-01-1970  */
    char                 creation_software[128];     /*!< (c)      File creation software (includes version)  */
    uint64_t             modification_timestamp;     /*!< (a)      File modification microseconds from 01-01-1970 (also set when file is modified)  */
    char                 modification_software[128]; /*!< (m)      File modification software (includes version)  */
    int32_t              number_of_records;          /*!< (a)      Number of records in file (counted by the API during creation).  */
    uint32_t             header_size;                /*!< (a)      Size of the ASCII header in bytes  */
    uint64_t             file_size;                  /*!< (a)      Size of the entire file in bytes (computed at creation).  */
    uint8_t              channel_valid[9];           /*!< (a)      Set if channel is valid (shallow 1-7, IR, deep).  */
    uint16_t             system_type;                /*!< (c)      Collection system type ID  */
    uint16_t             system_number;              /*!< (c)      Collection system serial number  */
    uint32_t             rep_rate;                   /*!< (c)      System rep rate  */
    char                 project[128];               /*!< (c)      Project information  */
    char                 mission[128];               /*!< (c)      Mission information  */
    char                 dataset[128];               /*!< (c)      Dataset information  */
    char                 flight_id[128];             /*!< (c)      Flight information  */
    uint64_t             flight_start_timestamp;     /*!< (a)      Start of flight in microseconds from 01-01-1970  */
    uint64_t             flight_end_timestamp;       /*!< (a)      End of flight in microseconds from 01-01-1970  */
    char                 comments[4096];             /*!< (m)      Comments.  */
  } CZMIL_CWF_Header;


  /*!

      - CZMIL CPF file header structure.  Header key definitions are as follows:

          - (a) = Set or modified by the API at creation time or later (e.g. modification_timestamp)
          - (c) = Defined by the application program only at creation time
          - (m) = Modifiable by the application program either at creation time or later using the czmil_update_cpf_header function

  */

  typedef struct
  {                                                  /*   Key             Definition  */
    char                 version[128];               /*!< (a)      Library version information  */
    char                 file_type[128];             /*!< (a)      File type (e.g. Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Point File) */
    uint64_t             creation_timestamp;         /*!< (a)      File creation microseconds from 01-01-1970  */
    char                 creation_software[128];     /*!< (c)      File creation software (includes version)  */
    uint64_t             modification_timestamp;     /*!< (a)      File modification microseconds from 01-01-1970  */
    char                 modification_software[128]; /*!< (m)      File modification software (includes version)  */
    double               min_lon;                    /*!< (a)      Minimum longitude value in file (west negative)  */
    double               min_lat;                    /*!< (a)      Minimum latitude value in file (south negative)  */
    double               max_lon;                    /*!< (a)      Maximum longitude value in file (west negative)  */
    double               max_lat;                    /*!< (a)      Maximum latitude value in file (south negative)  */
    double               base_lon;                   /*!< (c)      Base longitude for storing longitude offsets in the CPF record (west negative)  */
    double               base_lat;                   /*!< (c)      Base latitude for storing latitude offsets in the CPF record (south negative)  */
    int32_t              number_of_records;          /*!< (a)      Number of records in file (counted by the API during creation).  */
    uint32_t             header_size;                /*!< (a)      Size of the ASCII header in bytes  */
    uint64_t             file_size;                  /*!< (a)      Size of the entire file in bytes (computed at creation).  */
    uint16_t             system_type;                /*!< (c)      Collection system type ID  */
    uint16_t             system_number;              /*!< (c)      Collection system serial number  */
    uint32_t             rep_rate;                   /*!< (c)      System rep rate  */
    char                 project[128];               /*!< (c)      Project information  */
    char                 mission[128];               /*!< (c)      Mission information  */
    char                 dataset[128];               /*!< (c)      Dataset information  */
    char                 flight_id[128];             /*!< (c)      Flight information  */
    uint64_t             flight_start_timestamp;     /*!< (a)      Start of flight in microseconds from 01-01-1970  */
    uint64_t             flight_end_timestamp;       /*!< (a)      End of flight in microseconds from 01-01-1970  */
    float                null_z_value;               /*!< (c)      Null value to be used for elevations  */
    char                 wkt[4096];                  /*!< (c)      This is the Well-Known Text coordinate and datum
                                                                   information.  Well-known text is, according to Wikipedia, <b>a text markup
                                                                   language for representing vector geometry objects on a map, spatial reference
                                                                   systems of spatial objects and transformations between spatial reference systems.
                                                                   The formats are regulated by the Open Geospatial Consortium (OGC) and described
                                                                   in their Simple Feature Access and Coordinate Transformation Service
                                                                   specifications.</b>  These are supported by the GDAL/OGR library so you can use
                                                                   the OGRSpatialReference::importFromWkt() method (or the corresponding C
                                                                   function) and then warp from one to another pretty easily).  If this field is
                                                                   left blank by the creating program it will be automatically filled with the
                                                                   standard WGS84/WGS84E well-known text definition.
                                                                   (see <a href="index.html#CPF"><b>CZMIL Point File (CPF) format description</b></a>).  */
    uint16_t             local_vertical_datum;       /*!< (m)      Local vertical datum (possible values are defined in czmil_macros.h).
                                                                   This is the vertical datum that is associated with the 
                                                                   local_vertical_datum_offset in the CZMIL_CPF_Data structure.
                                                                   By default this is set to 0 (CZMIL_V_DATUM_UNDEFINED).  This value
                                                                   is NEVER set by the API and does not come from the collected CZMIL
                                                                   data.  It will be set by a datum shift application using the
                                                                   czmil_update_cpf_header function.  */
    char                 comments[4096];             /*!< (m)      Comments  */
    char                 user_data_description[4096];/*!< (m)      Description of the way the application program is using the user_data field of the
                                                                   CZMIL_CPF_Data structure.  */
  } CZMIL_CPF_Header;



  /*!

      - CZMIL CSF file header structure.  Header key definitions are as follows:

          - (a) = Set or modified by the API at creation time or later (e.g. modification_timestamp)
          - (c) = Defined by the application program only at creation time
          - (m) = Modifiable by the application program either at creation time or later using the czmil_update_csf_header function

  */

  typedef struct
  {                                                  /*   Key             Definition  */
    char                 version[128];               /*!< (a)      Library version information  */
    char                 file_type[128];             /*!< (a)      File type (e.g. Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) SBET File)  */
    uint64_t             creation_timestamp;         /*!< (a)      File creation microseconds from 01-01-1970  */
    char                 creation_software[128];     /*!< (c)      File creation software (includes version)  */
    uint64_t             modification_timestamp;     /*!< (a)      File modification microseconds from 01-01-1970  */
    char                 modification_software[128]; /*!< (m)      File modification software (includes version)  */
    double               min_lon;                    /*!< (a)      Minimum longitude value in file (west negative)  */
    double               min_lat;                    /*!< (a)      Minimum latitude value in file (south negative)  */
    double               max_lon;                    /*!< (a)      Maximum longitude value in file (west negative)  */
    double               max_lat;                    /*!< (a)      Maximum latitude value in file (south negative)  */
    double               base_lon;                   /*!< (c)      Base longitude for storing longitude offsets in the CSF record (west negative)  */
    double               base_lat;                   /*!< (c)      Base latitude for storing latitude offsets in the CSF record (south negative)  */
    int32_t              number_of_records;          /*!< (a)      Number of records in file (counted by the API during creation).  */
    uint32_t             header_size;                /*!< (a)      Size of the ASCII header in bytes  */
    uint64_t             file_size;                  /*!< (a)      Size of the entire file in bytes (computed at creation).  */
    char                 project[128];               /*!< (c)      Project information  */
    char                 mission[128];               /*!< (c)      Mission information  */
    char                 dataset[128];               /*!< (c)      Dataset information  */
    char                 flight_id[128];             /*!< (c)      Flight information  */
    uint64_t             flight_start_timestamp;     /*!< (a)      Start of flight in microseconds from 01-01-1970  */
    uint64_t             flight_end_timestamp;       /*!< (a)      End of flight in microseconds from 01-01-1970  */
  } CZMIL_CSF_Header;


  /*!

      - CZMIL CAF file header structure.  Header key definitions are as follows:

          - (a) = Set or modified by the API at creation time or later (e.g. creation_timestamp)
          - (c) = Defined by the application program only at creation time
          - (m) = Modifiable by the application program either at creation time or later using the czmil_update_csf_header function

  */

  typedef struct
  {                                                  /*   Key             Definition  */
    char                 version[128];               /*!< (a)      Library version information  */
    char                 file_type[128];             /*!< (a)      File type (e.g. Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) SBET File)  */
    uint64_t             creation_timestamp;         /*!< (a)      File creation microseconds from 01-01-1970  */
    uint64_t             application_timestamp;      /*!< (m)      File creation microseconds from 01-01-1970  */
    uint32_t             number_of_records;          /*!< (a)      Number of records in file  */
    uint32_t             header_size;                /*!< (a)      Size of the ASCII header in bytes  */
  } CZMIL_CAF_Header;


  /*!

      CZMIL Waveform File data structure.

      Note that there is a lot of wasted space in this structure.  The reason for this is that there may be up to
      CZMIL_MAX_PACKETS packets of 64 values stored in some of the waveforms.  We could get real tricky and allocate the
      memory for each waveform when we read it but this would require some pretty bullet-proof garbage collection.  In
      addition, allocation and freeing of memory uses a system wide mutex so it might introduce some contention between
      threads.  In normal use we will probably not have a whole lot of these in memory at one time so, in the interest
      of simplicity, we're going to set the arrays at the maximum size.

  */

  typedef struct
  {
    uint32_t             shot_id;                            /*!<  Shot ID number from MCWP.  */
    uint64_t             timestamp;                          /*!<  Microseconds from January 01, 1970.  */
    uint16_t             T0[64];                             /*!<  T0 waveform data.  */
    float                scan_angle;                         /*!<  Scan angle (this will be normalized to 0-360 degrees by the API).  */
    uint8_t              number_of_packets[9];               /*!<  Number of packets per channel.  */
    uint8_t              channel_ndx[9][CZMIL_MAX_PACKETS];  /*!<  Packet number of each populated packet.  */
    float                range[9][CZMIL_MAX_PACKETS];        /*!<  MCWP range value (stored as float in the hopes that we
                                                                   will be getting interpolated values in the future).  */
    uint16_t             channel[9][CZMIL_MAX_PACKETS * 64]; /*!<  Channel data, 9 channels of CZMIL_MAX_PACKETS packets of
                                                                   64 samples.  First seven are shallow 1 through 7, next is
                                                                   IR, last is deep.  */


    /*  Added in version 2.0  */

    uint16_t             validity_reason[9];                 /*!< (0-15) Per channel waveform validity reason (see czmil_macros.h).  */
  } CZMIL_CWF_Data;


  /*!

      - CZMIL Point File per return data structure.  Key definitions are as follows:

          - (c) = Defined by the application program at creation time or replaced using czmil_write_cpf_record (hopefully only by
                  the HydroFusion software).
          - (m) = Modifiable by the application program using the czmil_update_cpf_record function (any application) or the
                  czmil_write_cpf_record function (preferrably only by HydroFusion due to possible aliasing of other (c) values)

  */

  typedef struct
  {                                                          /*   Key             Definition  */
    double               latitude;                           /*!< (c)      Latitude in degrees (south negative)  */
    double               longitude;                          /*!< (c)      Longitude in degrees (west negative)  */
    float                elevation;                          /*!< (c)      Elevation above ellipsoid  */
    float                interest_point;                     /*!< (c)      Location along X axis of waveform at which depth/position is computed.  */
    uint8_t              ip_rank;                            /*!< (c)      This was originally intended to provide a ranking of the return amplitude
                                                                           but it was only ever used to denote water processed water surface returns.
                                                                           If it was set to 0 the return was a water processed water surface return.
                                                                           Any other value denoted a non-water surface return.  In files prior to
                                                                           v3.00 this field required 5 bits.  It now only uses one.  The name has
                                                                           been left unchanged so as to not impact existing application software.  
                                                                           YET ANOTHER CHANGE - Now that Optech has added the CZMIL_OPTECH_CLASS_HYBRID 
                                                                           processing mode, this bit will still be used to mark non-water surface returns (e.g.
                                                                           land and water processed returns that aren't the water surface) but it is 
                                                                           actually redundant going forward since Optech has decided to use the classification
                                                                           field to denote water surface (based on "Proposed LAS Enhancements to Support Topo-Bathy 
                                                                           Lidar", July 17, 2013).  Due to this, the ip_rank field may, at some point in the
                                                                           future either be repurposed or removed.  */
    float                reflectance;                        /*!< (m)      Reflectance or intensity  */
    float                horizontal_uncertainty;             /*!< (m)      Horizontal uncertainty  */
    float                vertical_uncertainty;               /*!< (m)      Vertical uncertainty  */
    uint16_t             status;                             /*!< (m)      Status information per return (see czmil_macros.h)  */
    uint32_t             classification;                     /*!< (m)      Return classification (based on ASPRS LAS v1.4 and "Proposed LAS Enhancements to Support
                                                                           Topo-Bathy Lidar", July 17, 2013.
                                                                           IMPORTANT NOTE: The Optech waveform processing mode was originally stored in
                                                                           this field.  For v1 files this value will be transferred to the per channel
                                                                           optech_classification field.  Due to this, v1 files do not have the ability
                                                                           to store a per return classification (e.g. bare earth, building, tree, etc).  */


    /*  Added in version 2.0  */

    float                probability;                        /*!< (m)      Probability of detection (0.0 to 1.0)  */
    uint8_t              filter_reason;                      /*!< (m)      Reason a point has been marked CZMIL_FILTER_INVALID in the status field 
                                                                           (0 - 63, 0 = no reason).  Note that the values from 0 to 16 will be
                                                                           duplicated values from the CWF validity_reason field.
                                                                           IMPORTANT NOTE: This should ONLY be set by HydroFusion!   */


    /*  Added in version 3.0  */

    uint16_t             d_index;                            /*!< (c)      Amplitude of the interest point normalized for noise.  In other words,
                                                                           on a completely noise free signal, if the amplitude of the interest
                                                                           point was 623, d_index would be 623.  When the noise is taken into
                                                                           account d_index will (always) be less than the actual amplitude of the 
                                                                           interest point.  The range is from 0 to 1023.  */
  } CZMIL_Return_Data;


  /*!

      - CZMIL Point File per shot data structure.  Key definitions are as follows:

          - (c) = Defined by the application program at creation time or replaced using czmil_write_cpf_record (hopefully only by
                  the HydroFusion software).
          - (m) = Modifiable by the application program using the czmil_update_cpf_record function (any application), the 
                  czmil_update_cpf_return_status function, or the czmil_write_cpf_record function (preferrably only by HydroFusion
                  due to possible aliasing of other (c) values)
          - (s) = See per return data structure (CZMIL_Return_Data) definition.

  */

  typedef struct
  {                                                          /*   Key             Definition  */
    uint64_t             timestamp;                          /*!< (c)      Microseconds from January 01, 1970  */
    float                off_nadir_angle;                    /*!< (c)      Off nadir angle.  */
    double               reference_latitude;                 /*!< (c)      Reference latitude in degrees (south negative).  This will be the water surface
                                                                           for water shots and the last return (bare earth) for land shots.  */
    double               reference_longitude;                /*!< (c)      Reference longitude in degrees (west negative).  This will be the water surface
                                                                           for water shots and the last return (bare earth) for land shots.  */
    float                water_level;                        /*!< (c)      Water level elevation above ellipsoid.  */
    float                kd;                                 /*!< (m)      Diffused attenuation coefficient (m^-1) (AKA light clarity).  */
    float                laser_energy;                       /*!< (m)      Laser energy derived from T0 in mJ.  */
    float                t0_interest_point;                  /*!< (m)      T0 interest point (used to align waveforms).  */
    double               bare_earth_latitude[7];             /*!< (m)      Computed bare earth latitude in degrees (south negative).  */
    double               bare_earth_longitude[7];            /*!< (m)      Computed bare earth longitude in degrees (west negative).  */
    float                bare_earth_elevation[7];            /*!< (m)      Computed bare earth elevation above ellipsoid.  */
    float                local_vertical_datum_offset;        /*!< (m)      Local vertical datum offset to be subtracted from elevations.  This value is
                                                                           NEVER set by the API and does not come from the collected CZMIL data.  It
                                                                           will be set by a datum shift application.  In addition, correction of the
                                                                           elevations to local datum in this structure and in the CZMIL_Return_Data is
                                                                           NOT done by the API but must be done by any application that needs elevations
                                                                           relative to the local vertical datum.  The application needs to check the
                                                                           local_vertical_datum field of the CZMIL_CPF_Header structure.  If that field
                                                                           is non-zero (i.e. not CZMIL_V_DATUM_UNDEFINED) and the application requires
                                                                           elevations relative to the local vertical datum it is the responsibility of
                                                                           the application to SUBTRACT this value from each elevation.  */
    uint8_t              returns[9];                         /*!< (c)      Number of returns per channel.  */
    CZMIL_Return_Data    channel[9][CZMIL_MAX_RETURNS];      /*!< (s)      7 shallow channel data, IR channel data, deep channel data.  */
    uint16_t             user_data;                          /*!< (m)      User data - defined by the application and explained in the
                                                                           [USER DATA DESCRIPTION] field of the CPF file header.
                                                                           IMPORTANT NOTE: This field was originally called status but it was never used.
                                                                           It was an 8 bit status flag field.  Later (v2) it was called spare but was
                                                                           again unused.  Since we actually wrote v1 and v2 files with storage for this
                                                                           field in them an application can now back fill them with user data.  */



    /*  Added in version 2.0  */

    uint8_t              optech_classification[9];           /*!< (c)      (0-63) Optech waveform processing mode for each channel (land, water, shallow
                                                                           water, other see czmil_macros.h, e.g. CZMIL_OPTECH_CLASS_LAND).
                                                                           IMPORTANT NOTE: The Optech waveform processing mode was originally stored in
                                                                           the per return classification field.  The per return classification field for 
                                                                           v1 files will be read to populate this field.  */

    /*  Added in version 3.0  */

    uint16_t             d_index_cube;                       /*!< (m)      Prediction of the normalized (see d_index definition, above) signal amplitude
                                                                           from a cube on the seafloor (deep channel, 0 to 1023)  */
  } CZMIL_CPF_Data;


  /*!  CZMIL SBET File data structure.  */

  typedef struct
  {
    uint64_t             timestamp;                          /*!<  Microseconds from January 01, 1970  */
    float                scan_angle;                         /*!<  Scan angle (this will be normalized to 0-360 degrees by the API).  */
    double               latitude;                           /*!<  Platform latitude in degrees (south negative)  */
    double               longitude;                          /*!<  Platform longitude in degrees (west negative)  */
    float                altitude;                           /*!<  Platform altitude above ellipsoid  */
    float                roll;                               /*!<  Platform roll in degrees  */
    float                pitch;                              /*!<  Platform pitch in degrees  */
    float                heading;                            /*!<  Platform heading in degrees  */
    float                range[9];                           /*!<  LiDAR ranges per channel (not to be confused with MCWP ranges in CWF).  */


    /*  Added in version 2.0  */

    float                range_in_water[9];                  /*!<  LiDAR range in water per channel.  */
    float                intensity[9];                       /*!<  Intensity per channel.  */
    float                intensity_in_water[9];              /*!<  Intensity in water per channel.  */
  } CZMIL_CSF_Data;


  /*  The CAF data structure.  */

  typedef struct
  {
    int32_t           shot_id;                               /*!<  CPF file record number/shot ID for this return  */
    uint8_t           channel_number;                        /*!<  CPF file channel number for this return  */
    uint8_t           optech_classification;                 /*!<  Optech waveform processing mode for this channel/return  */
    float             interest_point;                        /*!<  Interest point location for this return  */
    uint8_t           return_number;                         /*!<  Return number  */
    uint8_t           number_of_returns;                     /*!<  Number of returns for this channel  */
  } CZMIL_CAF_Data;


  /*!  HydroFusion (Optech) structure and function definitions.  */

#include "czmil_optech.h"


  /*!  Typedef for indexing progress callback  */

  typedef void (*CZMIL_PROGRESS_CALLBACK) (int32_t hnd, char *path, int32_t percent);


  /*!  CZMIL Public function declarations.  */

  CZMIL_DLL void czmil_register_progress_callback (CZMIL_PROGRESS_CALLBACK progressCB);

  CZMIL_DLL int32_t czmil_create_caf_file (char *path, CZMIL_CAF_Header *caf_header);

  CZMIL_DLL int32_t czmil_open_cwf_file (const char *path, CZMIL_CWF_Header *cwf_header, int32_t mode);
  CZMIL_DLL int32_t czmil_open_cpf_file (const char *path, CZMIL_CPF_Header *cpf_header, int32_t mode);
  CZMIL_DLL int32_t czmil_open_csf_file (const char *path, CZMIL_CSF_Header *csf_header, int32_t mode);
  CZMIL_DLL int32_t czmil_open_caf_file (const char *path, CZMIL_CAF_Header *caf_header);

  CZMIL_DLL int32_t czmil_close_cwf_file (int32_t hnd);
  CZMIL_DLL int32_t czmil_close_cpf_file (int32_t hnd);
  CZMIL_DLL int32_t czmil_close_csf_file (int32_t hnd);
  CZMIL_DLL int32_t czmil_close_caf_file (int32_t hnd);

  CZMIL_DLL int32_t czmil_read_cwf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CWF_Data *record_array);
  CZMIL_DLL int32_t czmil_read_cwf_record (int32_t hnd, int32_t recnum, CZMIL_CWF_Data *record);
  CZMIL_DLL int32_t czmil_read_cpf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CPF_Data *record_array);
  CZMIL_DLL int32_t czmil_read_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record);
  CZMIL_DLL int32_t czmil_read_csf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CSF_Data *record_array);
  CZMIL_DLL int32_t czmil_read_csf_record (int32_t hnd, int32_t recnum, CZMIL_CSF_Data *record);
  CZMIL_DLL int32_t czmil_read_caf_record (int32_t hnd, CZMIL_CAF_Data *record);

  CZMIL_DLL int32_t czmil_write_caf_record (int32_t hnd, CZMIL_CAF_Data *record);

  CZMIL_DLL int32_t czmil_update_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record);
  CZMIL_DLL int32_t czmil_update_cpf_return_status (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record);

  CZMIL_DLL int32_t czmil_update_cwf_header (int32_t hnd, CZMIL_CWF_Header *cwf_header);
  CZMIL_DLL int32_t czmil_update_cpf_header (int32_t hnd, CZMIL_CPF_Header *cpf_header);
  CZMIL_DLL int32_t czmil_update_csf_header (int32_t hnd, CZMIL_CSF_Header *csf_header);

  CZMIL_DLL int32_t czmil_add_field_to_cwf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_add_field_to_cpf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_add_field_to_csf_header (int32_t hnd, char *tag, char *contents);

  CZMIL_DLL int32_t czmil_get_field_from_cwf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_get_field_from_cpf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_get_field_from_csf_header (int32_t hnd, char *tag, char *contents);

  CZMIL_DLL int32_t czmil_update_field_in_cwf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_update_field_in_cpf_header (int32_t hnd, char *tag, char *contents);
  CZMIL_DLL int32_t czmil_update_field_in_csf_header (int32_t hnd, char *tag, char *contents);

  CZMIL_DLL int32_t czmil_delete_field_from_cwf_header (int32_t hnd, char *tag);
  CZMIL_DLL int32_t czmil_delete_field_from_cpf_header (int32_t hnd, char *tag);
  CZMIL_DLL int32_t czmil_delete_field_from_csf_header (int32_t hnd, char *tag);

  CZMIL_DLL int32_t czmil_get_errno ();
  CZMIL_DLL char *czmil_strerror ();
  CZMIL_DLL void czmil_perror ();
  CZMIL_DLL char *czmil_get_version ();
  CZMIL_DLL void czmil_get_version_numbers (char *version, uint16_t *major_version, uint16_t *minor_version);

  CZMIL_DLL double czmil_compute_pd_cube (CZMIL_CPF_Data *record, int32_t false_alarm);

  CZMIL_DLL void czmil_dump_cwf_record (int32_t hnd, CZMIL_CWF_Data *record, FILE *fp);
  CZMIL_DLL void czmil_dump_cpf_record (CZMIL_CPF_Data *record, FILE *fp);
  CZMIL_DLL void czmil_dump_csf_record (CZMIL_CSF_Data *record, FILE *fp);

  CZMIL_DLL void czmil_cvtime (int64_t micro_sec, int32_t *year, int32_t *jday, int32_t *hour, int32_t *minute, float *second);
  CZMIL_DLL void czmil_inv_cvtime (int32_t year, int32_t jday, int32_t hour, int32_t min, float sec, uint64_t *timestamp);
  CZMIL_DLL void czmil_get_cpf_return_status_string (uint16_t status, char *status_string);
  CZMIL_DLL void czmil_get_cpf_filter_reason_string (uint16_t filter_reason, char *filter_reason_string);
  CZMIL_DLL void czmil_get_cwf_validity_reason_string (uint16_t validity_reason, char *validity_reason_string);
  CZMIL_DLL void czmil_jday2mday (int32_t year, int32_t jday, int32_t *mon, int32_t *mday);
  CZMIL_DLL void czmil_get_channel_string (int32_t channel_num, char *channel_string);
  CZMIL_DLL void czmil_get_proc_mode_string (int32_t optech_classification, char *proc_mode_string);
  CZMIL_DLL void czmil_get_short_proc_mode_string (int32_t optech_classification, char *proc_mode_string);
  CZMIL_DLL void czmil_get_local_vertical_datum_string (uint16_t local_vertical_datum, char *datum_string);

#ifdef  __cplusplus
}
#endif


#endif
