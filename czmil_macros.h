
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USMC 105, copyright protection
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



#ifndef __CZMIL_MACROS_H__
#define __CZMIL_MACROS_H__

#ifdef  __cplusplus
extern "C" {
#endif




#define       CZMIL_MAX_FILES                      128       /*!<  Maximum number of open CZMIL files.  */
#define       CZMIL_MAX_PACKETS                    15        /*!<  Maximum number of 64 sample packets in a single waveform.  
                                                                   <b>** WARNING WARNING - NEVER DECREASE THIS NUMBER! - WARNING WARNING **</b>
                                                                   Because it is used in czmil.h as an array size.  */
#define       CZMIL_MAX_RETURNS                    31        /*!<  Maximum number of returns per channel.
                                                                   <b>** WARNING WARNING - NEVER DECREASE THIS NUMBER! - WARNING WARNING **</b>
                                                                   Because it is used in czmil.h as an array size.  */

#define       CZMIL_CWF_IO_BUFFER_SIZE             36000000  /*!<  Default CWF I/O buffer size.  With the current compression rates this should
                                                                   be about 2 seconds of data.  This value should never be set larger than about
                                                                   2,147,000,000 for obvious reasons.  */
#define       CZMIL_CPF_IO_BUFFER_SIZE             36000000  /*!<  Default CPF I/O buffer size.  */
#define       CZMIL_CSF_IO_BUFFER_SIZE             15000000  /*!<  Default CSF I/O buffer size.  */
#define       CZMIL_CAF_IO_BUFFER_SIZE             6000000   /*!<  Default CAF I/O buffer size.  */


  /*  Channel indexes.  */

#define       CZMIL_SHALLOW_CHANNEL_1              0
#define       CZMIL_SHALLOW_CHANNEL_2              1
#define       CZMIL_SHALLOW_CHANNEL_3              2
#define       CZMIL_SHALLOW_CHANNEL_4              3
#define       CZMIL_SHALLOW_CHANNEL_5              4
#define       CZMIL_SHALLOW_CHANNEL_6              5
#define       CZMIL_SHALLOW_CHANNEL_7              6
#define       CZMIL_IR_CHANNEL                     7
#define       CZMIL_DEEP_CHANNEL                   8


  /*  File open modes.  */

#define       CZMIL_UPDATE                         0      /*!<  Open file for update.  */
#define       CZMIL_READONLY                       1      /*!<  Open file for read only.  */
#define       CZMIL_READONLY_SEQUENTIAL            2      /*!<  Open file for read only in sequential mode.  That is, the file will be read
                                                                sequentially from record zero to the end of the file.  This mode will cause
                                                                the API to use setvbuf to set a larger I/O buffer for, hopefully, faster
                                                                I/O.  */
#define       CZMIL_CWF_PROCESS_WAVEFORMS          3      /*!<  This mode is to be used only by Optech to open an existing CWF file for
                                                                processing waveforms to produce the CPF file.  It will be converted to
                                                                CZMIL_READONLY_SEQUENTIAL after some initial checking of files is done.  */


#define       CZMIL_NEXT_RECORD                    -1     /*!<  Use this as the record number if you are appending to (i.e. creating) a CPF
                                                                file or if you want to read an entire CSF file sequentially.  */


  /*  Per channel waveform validity reason definitions.  If you add to these be sure to modify czmil_get_cwf_validity_reason_string in
      czmil.c to match.  Values before 16 are for the entire waveform but will be put into the CPF per return filter_reason field.  */

#define       CZMIL_WAVEFORM_VALID                 0       /*!<  Waveform is valid.  */
#define       CZMIL_DATA_ID_CORRUPTED              1       /*!<  Data recorded is not in the expected format (corrupted)  */
#define       CZMIL_TIMESTAMP_INVALID              2       /*!<  Incorrect time was encountered in the CWF record.  It has
                                                                 been set to a nominal time of 100 microseconds greater
                                                                 than the previously supplied shot time.  This only occurs
                                                                 on creation of the CWF file.  */
#define       CZMIL_SCAN_ANGLE_INVALID             3       /*!<  Scan angle for the shot was invalid.  This only occurs on creation of the
                                                                 CWF file.  */
#define       CZMIL_DROPPED_PACKET                 4       /*!<  Dropped data packet (MCWP decides this)  */
#define       CZMIL_NUMBER_OF_PACKETS_INVALID      5       /*!<  Number of packets for the channel is invalid  */
#define       CZMIL_PACKET_NUMBER_INVALID          6       /*!<  Packet number is invalid  */
#define       CZMIL_WAVEFORM_DROPPED               7       /*!<  No waveform was recorded (MCWP)  */
#define       CZMIL_CHANNEL_FILE_DOES_NOT_EXIST    8       /*!<  Download program couldn't locate the LiDAR raw file  */
  /*          CZMIL_RESERVED                       9-15          Reserved for future use  */


  /*  Per return filter reason definitions.  If you add to these be sure to modify czmil_get_cpf_filter_reason_string in czmil.c to match.
      Values before 16 come from the waveform validity reason definitions above.  */

#define       CZMIL_DEEP_LAND                      16      /*!<  Deep channel land return  */
#define       CZMIL_PRE_PULSE                      17      /*!<  Return invalidated by pre-pulse filter  */
#define       CZMIL_AFTER_PULSE                    18      /*!<  Return invalidated by after-pulse filter  */
#define       CZMIL_AUTO_EDIT_SPATIAL_FILTERED     19      /*!<  Invalidated by auto-edit spatial filter in Level 2 lidar processing  */
#define       CZMIL_TURBID_WATER_LAYER_RETURN      20      /*!<  Return is most likely "turbid water layer return"  */
#define       CZMIL_WATER_LAST_RETURN_INVALID      21      /*!<  Water return invalidated if processed using "last return" or "strongest
                                                                 return" processing mode  */
#define       CZMIL_WAVEFORM_SATURATED             22      /*!<  Return invalidated because of waveform saturation  */
#define       CZMIL_INVALIDATED_DEEP_W_SH_PRESENT  23	   /*!<  Invalidated by filter in Level 2 lidar processing (deep returns filtered
                                                                 when shallow present)  */
#define       CZMIL_SPATIAL_FILTERED               24      /*!<  Invalidated by filter in Level 2 lidar processing (spatial filtering)  */
#define       CZMIL_DEEP_CHANNEL_MULTI_REFLECTION_RETURNS 25 /*!<  Invalidated by filter in Level 2 lidar processing (Deep channel multiple
                                                                   reflection returns filtering)  */
#define       CZMIL_DEEP_CHANNEL_MEDIAN_FILTER     26      /*!<  Invalidated by filter in Level 2 lidar processing (Deep channel Median
                                                                 filtering)  */
#define       CZMIL_DIGITIZER_NOISE                27      /*!<  Invalidated by digitizer noise filter in Level 2  */
#define       CZMIL_START_AMP_EXCEEDS_THRESHOLD    28      /*!<  Invalidated by starting amplitude filter in Level 2  */
#define       CZMIL_NO_T0                          29      /*!<  No corresponding T0 record found */
#define       CZMIL_IN_USER_EXCLUSION_ZONE         30      /*!<  Return in a user defined exclusion zone */
  /*          CZMIL_RESERVED                       31-52         Reserved for future use  */

#define       CZMIL_APP_HP_FILTERED                53      /*!<  Invalidated by hockey puck filter  */
  /*          APPLICATION_RESERVED                 54-62         Reserved for future use  */

#define       CZMIL_REPROCESSING_BUFFER            63      /*!<  This point has been placed in the file as a buffer point for reprocessing
                                                                 or it is a result of reprocessing when fewer points were created during
                                                                 reprocessing than were there after initial processing.  As of version 2 of
                                                                 the API Optech will place at least 4 returns in every channel.  If only one
                                                                 point was created during initial processing, three extra points will be
                                                                 created and marked as CZMIL_REPROCESSING_BUFFER points to allow for room
                                                                 to place extra points during reprocessing.  This usually occurs due to
                                                                 changing single return, land processed points to shallow water points or
                                                                 increasing the probability of detection for deep water points.  The reason
                                                                 for all of this complication is that the only way to do reprocessing
                                                                 on-the-fly from within an editor is to guarantee that there will never be a
                                                                 change in the number of returns per channel when reprocessing.  Obviously,
                                                                 if we try to increase the number of returns for a channel we would have to
                                                                 rewrite the entire file because of the compressed, variable length record
                                                                 structure.  <b>Points marked as CZMIL_RETURN_FILTER_INVAL with a filter
                                                                 reason of CZMIL_REPROCESSING_BUFFER should be ignored by all applications
                                                                 since they may or may not be valid.</b>  */



  /*!<  CZMIL urban noise attribute bit flags.  These are stored in an attribute in pfmLoader for later use in urban noise filters.  */

#define CZMIL_URBAN_NOT_TOPO                      0        /*!< 0 0000 - If no flags are set, the shot was not topo processed  */
#define CZMIL_URBAN_TOPO                          1        /*!< 0 0001 - This return is on a shot that was topo processed (by definition, all
                                                                         returns not marked as CZMIL_URBAN_NOT_TOPO will have this bit set)  */
#define CZMIL_URBAN_SOFT_HIT                      2        /*!< 0 0010 - This return came from a channel with more than 4 valid returns  */
#define CZMIL_URBAN_HARD_HIT                      4        /*!< 0 0100 - This return came from a channel with less than 3 valid returns  */
#define CZMIL_URBAN_MAJORITY                      8        /*!< 0 1000 - This return came from a shot where a majority of channels had less
                                                                         than 4 returns  */
#define CZMIL_URBAN_VICINITY                     16        /*!< 1 0000 - This return was within a user specified distance (H and V) of 
                                                                         a return marked CZMIL_URBAN_MAJORITY.  This is NOT done in pfmLoader
                                                                         but in a subsequent filter program or process.  */



  /*  Per return status bit definitions.  If you add to these be sure to modify return_string and czmil_get_cpf_return_status_string in
      czmil.c to match.  */

#define       CZMIL_RETURN_VALID                   0       /*!<  0000 0000 = 0x00  -  Point status has not been set (assumed valid)  */
#define       CZMIL_RETURN_MANUALLY_INVAL          1       /*!<  0000 0001 = 0x01  -  Point has been manually marked as invalid  */
#define       CZMIL_RETURN_FILTER_INVAL            2       /*!<  0000 0010 = 0x02  -  Point has been automatically marked as invalid  */
#define       CZMIL_RETURN_CLASSIFICATION_MODIFIED 4       /*!<  0000 0100 = 0x04  -  Point has had it's classification manually modified  */
#define       CZMIL_RETURN_SELECTED_FEATURE        8       /*!<  0000 1000 = 0x08  -  Point is on or near a selected feature  */
#define       CZMIL_RETURN_DESIGNATED_SOUNDING     16      /*!<  0001 0000 = 0x10  -  Indicates the depth record overrides all hypotheses.
                                                                                      Not all 'designated' soundings will be 'features'.
                                                                                      This is included to support the Open Navigation
                                                                                      Surface (ONS) Bathymetry Attributed Grid (BAG or
                                                                                      Big Ass Grid) standard format.*/
#define       CZMIL_RETURN_SUSPECT                 32      /*!<  0010 0000 = 0x20  -  Point has been marked as suspect  */
#define       CZMIL_RETURN_REFERENCE               64      /*!<  0100 0000 = 0x40  -  Point has been marked as reference data (not used in
                                                                                      PFM surfaces)  */
#define       CZMIL_RETURN_REPROCESSED             128     /*!<  1000 0000 = 0x80  -  Point has been reprocessed by the user (via the
                                                                                      reprocessing API)  */

#define       CZMIL_RETURN_INVAL                   3       /*!<  0000 0011 = 0x03  -  Combination of two invalid flags  */
#define       CZMIL_RETURN_STAT_MASK               255     /*!<  1111 1111 = 0xff  -  Mask for inverting flag settings  */


  /*  Optech waveform processing mode definitions (optech_classification field).  */

#define       CZMIL_OPTECH_CLASS_UNDEFINED         0       /*!<  Optech waveform processing mode is unknown (pre 2.0 file - check
                                                                 classification)  */
#define       CZMIL_OPTECH_CLASS_LAND              1       /*!<  Optech waveform processed as land shot  */
  /*          CZMIL_OTHER_LAND                     2-30          Reserved for future land sub-classes  */

#define       CZMIL_OPTECH_CLASS_HYBRID            31      /*!<  Optech waveform processed using hybrid class algorithm (when a waveform
                                                                 hits both land and water)  */

#define       CZMIL_OPTECH_CLASS_WATER             32      /*!<  Optech waveform processed as water shot  */
#define       CZMIL_OPTECH_CLASS_SHALLOW_WATER     33      /*!<  Optech waveform processed using shallow water algorithm  */
#define       CZMIL_OPTECH_CLASS_AVERAGE_DEPTH     34      /*!<  Optech waveforms processed using average depth algorithm  */
#define       CZMIL_OPTECH_CLASS_TURBID_WATER      35      /*!<  Optech waveform processed using turbid water algorithm  */
#define       CZMIL_OPTECH_CLASS_RSD_MSR           36      /*!<  Optech waveform processed using Robust Surface Detection (missed surface
                                                                 return algorithm - shallow channels only) */
#define       CZMIL_OPTECH_CLASS_RSD_MWSD          37      /*!<  Optech waveform processed using Robust Surface Detection (multiple water
                                                                 surface detection algorithm - shallow channels only)  */
#define       CZMIL_OPTECH_CLASS_RSD_MWSD_DEEP     38      /*!<  Optech waveform processed using Robust Surface Detection (multiple water
                                                                 surface detection algorithm - deep channel only)  */
#define       CZMIL_OPTECH_CLASS_WATER_DEEP_W_DEEP_SURFACE 39 /*!<  Optech waveform processed using deep channel surface return - deep
                                                                    channel only */
#define       CZMIL_OPTECH_CLASS_SMALL_BOTTOM      40      /*!<  Optech waveform processed using small bottom return algorithm  */


  /*  Convenience macros for checking single flags in return status variable.  */

#define       czmil_cpf_return_is_invalid(status)              (status & CZMIL_RETURN_INVAL)                /*!<  Returns TRUE if return
                                                                                                                  is invalid  */
#define       czmil_cpf_return_is_manually_invalid(status)     (status & CZMIL_RETURN_MANUALLY_INVAL)       /*!<  Returns TRUE if return is
                                                                                                                  manually invalid  */
#define       czmil_cpf_return_is_filter_invalid(status)       (status & CZMIL_RETURN_FILTER_INVAL)         /*!<  Returns TRUE if return is
                                                                                                                  filter invalid  */
#define       czmil_cpf_return_is_selected_sounding(status)    (status & CZMIL_RETURN_SELECTED_SOUNDING)    /*!<  Returns TRUE if return is
                                                                                                                  a selected sounding  */
#define       czmil_cpf_return_is_selected_feature(status)     (status & CZMIL_RETURN_SELECTED_FEATURE)     /*!<  Returns TRUE if return is
                                                                                                                  a selected feature  */
#define       czmil_cpf_return_is_designated_sounding(status)  (status & CZMIL_RETURN_DESIGNATED_SOUNDING)  /*!<  Returns TRUE if return is
                                                                                                                  a designated sounding  */
#define       czmil_cpf_return_is_suspect(status)              (status & CZMIL_RETURN_SUSPECT)              /*!<  Returns TRUE if return is
                                                                                                                  suspect  */
#define       czmil_cpf_return_is_reference(status)            (status & CZMIL_RETURN_REFERENCE)            /*!<  Returns TRUE if return is
                                                                                                                  reference  */
#define       czmil_cpf_return_is_reprocessed(status)          (status & CZMIL_RETURN_RREPROCESSED)         /*!<  Returns TRUE if return has
                                                                                                                  been reprocessed  */

  /*  Return validity reason values (TBD).  */

#define       CZMIL_CPF_RETURN_NO_REASON           0


  /*  Error conditions.  */

#define       CZMIL_SUCCESS                        0
#define       CZMIL_CAF_APPEND_ERROR               -1
#define       CZMIL_CAF_IO_BUFFER_ALLOCATION_ERROR -2
#define       CZMIL_CAF_CLOSE_ERROR                -3
#define       CZMIL_CAF_CREATE_ERROR               -4
#define       CZMIL_CAF_HEADER_READ_FSEEK_ERROR    -5
#define       CZMIL_CAF_HEADER_WRITE_ERROR         -6
#define       CZMIL_CAF_HEADER_WRITE_FSEEK_ERROR   -7
#define       CZMIL_CAF_INVALID_FILENAME_ERROR     -8
#define       CZMIL_CAF_OPEN_ERROR                 -9
#define       CZMIL_CAF_READ_ERROR                 -10
#define       CZMIL_CAF_SETVBUF_ERROR              -11
#define       CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR   -12
#define       CZMIL_CAF_WRITE_BUFFER_SIZE_ERROR    -13
#define       CZMIL_CAF_WRITE_ERROR                -14
#define       CZMIL_CIF_IO_BUFFER_ALLOCATION_ERROR -15
#define       CZMIL_CIF_CLOSE_ERROR                -16
#define       CZMIL_CIF_CREATE_ERROR               -17
#define       CZMIL_CIF_HEADER_READ_FSEEK_ERROR    -18
#define       CZMIL_CIF_HEADER_WRITE_ERROR         -19
#define       CZMIL_CIF_HEADER_WRITE_FSEEK_ERROR   -20
#define       CZMIL_CIF_OPEN_ERROR                 -21
#define       CZMIL_CIF_READ_ERROR                 -22
#define       CZMIL_CIF_READ_FSEEK_ERROR           -23
#define       CZMIL_CIF_REMOVE_ERROR               -24
#define       CZMIL_CIF_RENAME_ERROR               -25
#define       CZMIL_CIF_SETVBUF_ERROR              -26
#define       CZMIL_CIF_WRITE_ERROR                -27
#define       CZMIL_CWF_APPEND_ERROR               -28
#define       CZMIL_CWF_CIF_BUFFER_SIZE_ERROR      -29
#define       CZMIL_CWF_CLOSE_ERROR                -30
#define       CZMIL_CWF_CREATE_CIF_ERROR           -31
#define       CZMIL_CWF_CREATE_ERROR               -32
#define       CZMIL_CWF_EXCEEDED_HEADER_SIZE_ERROR -33
#define       CZMIL_CWF_HEADER_READ_FSEEK_ERROR    -34
#define       CZMIL_CWF_HEADER_TAG_ERROR           -35
#define       CZMIL_CWF_HEADER_WRITE_ERROR         -36
#define       CZMIL_CWF_HEADER_WRITE_FSEEK_ERROR   -37
#define       CZMIL_CWF_INVALID_FILENAME_ERROR     -38
#define       CZMIL_CWF_IO_BUFFER_ALLOCATION_ERROR -39
#define       CZMIL_CWF_NO_CIF_ERROR               -40
#define       CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR  -41
#define       CZMIL_CWF_OPEN_READONLY_ERROR        -42
#define       CZMIL_CWF_OPEN_UPDATE_ERROR          -43
#define       CZMIL_CWF_READ_ERROR                 -44
#define       CZMIL_CWF_READ_FSEEK_ERROR           -45
#define       CZMIL_CWF_TIME_REGRESSION_ERROR      -46  /*  No longer used  */
#define       CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR   -47
#define       CZMIL_CWF_WRITE_ERROR                -48
#define       CZMIL_CWF_WRITE_FSEEK_ERROR          -49
#define       CZMIL_CWF_SETVBUF_ERROR              -50
#define       CZMIL_CPF_APPEND_ERROR               -51
#define       CZMIL_CPF_ABORT_ERROR                -52
#define       CZMIL_CPF_CIF_BUFFER_SIZE_ERROR      -53
#define       CZMIL_CPF_CLOSE_ERROR                -54
#define       CZMIL_CPF_CREATE_CIF_ERROR           -55
#define       CZMIL_CPF_CREATE_ERROR               -56
#define       CZMIL_CPF_EXCEEDED_HEADER_SIZE_ERROR -57
#define       CZMIL_CPF_HEADER_READ_FSEEK_ERROR    -58
#define       CZMIL_CPF_HEADER_TAG_ERROR           -59
#define       CZMIL_CPF_HEADER_WRITE_ERROR         -60
#define       CZMIL_CPF_HEADER_WRITE_FSEEK_ERROR   -61
#define       CZMIL_CPF_INVALID_FILENAME_ERROR     -62
#define       CZMIL_CPF_IO_BUFFER_ALLOCATION_ERROR -63
#define       CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR  -64
#define       CZMIL_CPF_OPEN_READONLY_ERROR        -65
#define       CZMIL_CPF_OPEN_UPDATE_ERROR          -66
#define       CZMIL_CPF_READ_ERROR                 -67
#define       CZMIL_CPF_READ_FSEEK_ERROR           -68
#define       CZMIL_CPF_REMOVE_ERROR               -69
#define       CZMIL_CPF_TIME_REGRESSION_ERROR      -70
#define       CZMIL_CPF_TOO_MANY_RETURNS_ERROR     -71
#define       CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR   -72
#define       CZMIL_CPF_WRITE_ERROR                -73
#define       CZMIL_CPF_WRITE_FSEEK_ERROR          -74
#define       CZMIL_CPF_SETVBUF_ERROR              -75
#define       CZMIL_CSF_APPEND_ERROR               -76
#define       CZMIL_CSF_CLOSE_ERROR                -77
#define       CZMIL_CSF_CREATE_ERROR               -78
#define       CZMIL_CSF_EXCEEDED_HEADER_SIZE_ERROR -79
#define       CZMIL_CSF_HEADER_READ_FSEEK_ERROR    -80
#define       CZMIL_CSF_HEADER_TAG_ERROR           -81
#define       CZMIL_CSF_HEADER_WRITE_ERROR         -82
#define       CZMIL_CSF_HEADER_WRITE_FSEEK_ERROR   -83
#define       CZMIL_CSF_INVALID_FILENAME_ERROR     -84
#define       CZMIL_CSF_IO_BUFFER_ALLOCATION_ERROR -85
#define       CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR  -86
#define       CZMIL_CSF_OPEN_READONLY_ERROR        -87
#define       CZMIL_CSF_OPEN_UPDATE_ERROR          -88
#define       CZMIL_CSF_READ_ERROR                 -89
#define       CZMIL_CSF_READ_FSEEK_ERROR           -90
#define       CZMIL_CSF_TIME_REGRESSION_ERROR      -91
#define       CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR   -92
#define       CZMIL_CSF_WRITE_BUFFER_SIZE_ERROR    -93
#define       CZMIL_CSF_WRITE_ERROR                -94
#define       CZMIL_CSF_WRITE_FSEEK_ERROR          -95
#define       CZMIL_CSF_SETVBUF_ERROR              -96
#define       CZMIL_INVALID_RECORD_NUMBER_ERROR    -97
#define       CZMIL_NEWER_FILE_VERSION_WARNING     -98
#define       CZMIL_NO_MATCHING_TAG_ERROR          -99
#define       CZMIL_NOT_CZMIL_FILE_ERROR           -100
#define       CZMIL_TOO_MANY_OPEN_FILES_ERROR      -101
#define       CZMIL_BAD_TAG_ERROR                  -102
#define       CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR  -103  /*  gcc spits out warnings if you don't check the return value of certain functions.
                                                             Due to this I had to add return error checking on things that should never fail.
							     You should never see this error!  */


  /*  Supported local vertical datums.  These match the vertical datum values used in Generic Sensor Format (GSF).  */

#define       CZMIL_V_DATUM_UNDEFINED              0         /*!<  No vertical datum shift has been applied  */
#define       CZMIL_V_DATUM_UNKNOWN                1         /*!<  Unknown vertical datum  */
#define       CZMIL_V_DATUM_MLLW                   2         /*!<  Mean lower low water  */
#define       CZMIL_V_DATUM_MLW                    3         /*!<  Mean low water  */
#define       CZMIL_V_DATUM_ALAT                   4         /*!<  Approximate Lowest Astronomical Tide  */
#define       CZMIL_V_DATUM_ESLW                   5         /*!<  Equatorial Springs Low Water  */
#define       CZMIL_V_DATUM_ISLW                   6         /*!<  Indian Springs Low Water  */
#define       CZMIL_V_DATUM_LAT                    7         /*!<  Lowest Astronomical Tide  */
#define       CZMIL_V_DATUM_LLW                    8         /*!<  Lowest Low Water  */
#define       CZMIL_V_DATUM_LNLW                   9         /*!<  Lowest Normal Low Water  */
#define       CZMIL_V_DATUM_LWD                    10        /*!<  Low Water Datum  */
#define       CZMIL_V_DATUM_MLHW                   11        /*!<  Mean Lower High Water  */
#define       CZMIL_V_DATUM_MLLWS                  12        /*!<  Mean Lower Low Water Springs  */
#define       CZMIL_V_DATUM_MLWN                   13        /*!<  Mean Low Water Neap  */
#define       CZMIL_V_DATUM_MSL                    14        /*!<  Mean Sea Level  */


#ifdef  __cplusplus
}
#endif


#endif
