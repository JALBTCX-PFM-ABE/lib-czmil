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



/*  Try to handle some things for MSC and Mac OS/X.  */

#if (defined _WIN32) && (defined _MSC_VER)
#undef fseeko64
#undef ftello64
#undef fopen64
#define fseeko64(x, y, z) _fseeki64((x), (y), (z))
#define ftello64(x)       _ftelli64((x))
#define fopen64(x, y)      fopen((x), (y))
#elif defined(__APPLE__)
#define fseeko64(x, y, z) fseek((x), (y), (z))
#define ftello64(x)       ftell((x))
#define fopen64(x, y)     fopen((x), (y))
#endif


#include "czmil.h"
#include "czmil_internals.h"
#include "czmil_version.h"


/*!  This is where we'll store the headers and formatting/usage information of all open CZMIL files (see czmil_internals.h).  */

static INTERNAL_CZMIL_CWF_STRUCT cwf[CZMIL_MAX_FILES];
static INTERNAL_CZMIL_CPF_STRUCT cpf[CZMIL_MAX_FILES];
static INTERNAL_CZMIL_CSF_STRUCT csf[CZMIL_MAX_FILES];
static INTERNAL_CZMIL_CIF_STRUCT cif[CZMIL_MAX_FILES];
static INTERNAL_CZMIL_CAF_STRUCT caf[CZMIL_MAX_FILES];


/*!  This is where we'll store error information in the event of some kind of screwup (see czmil_internals.h).  */

static CZMIL_ERROR_STRUCT czmil_error;


/*!  Startup flag used by either czmil_create_XXX_file or czmil_open_XXX_file to initialize the internal struct arrays and
     set the SIGINT handler.  */

static uint8_t first = 1;


/*!  Timezone flag used by either czmil_cvtime or czmil_inv_cvtime just in case they are used in a standalone manner.  */

static uint8_t tz_set = 0;


/*!  These will never be called by an application program so we're defining them here.  */

static int32_t czmil_write_cif_header (INTERNAL_CZMIL_CIF_STRUCT *cif_struct);
static int32_t czmil_read_cif_header (int32_t hnd);
static int32_t czmil_write_cif_record (INTERNAL_CZMIL_CIF_STRUCT *cif_struct);
static int32_t czmil_create_cif_file (int32_t hnd, char *path);
static int32_t czmil_open_cif_file (const char *path, CZMIL_CIF_Header *cif_header, int32_t mode);
static int32_t czmil_read_cif_record (int32_t hnd, int32_t recnum, CZMIL_CIF_Data *record);
static void czmil_dump_cif_record (CZMIL_CIF_Data *record, FILE *fp);


/*  Insert a bunch of static utility functions that really don't need to live in this file.  */

#include "czmil_functions.h"


static CZMIL_PROGRESS_CALLBACK czmil_progress_callback = NULL;


/********************************************************************************************/
/*!

 - Function:    czmil_register_progress_callback

 - Purpose:     This function is used to register a callback function to keep track of the
                percentage complete when indexing CWF and/or CPF files.  If you do not call
                this function to set your handler, the percentages will be printed to stdout.
                An example of the use of the progress callback function would be to update a
                progress bar in a GUI application.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/22/12

 - Arguments:
                - progressCB     =    The function that will handle the 
                                      percentage information

 - Caveats:     The callback function that you specify must have two int32_t arguments.
                The function should look like this:
                <br>
                <br>
                <pre>
                static void progressCB (int32_t hnd, char *path, int32_t percent)
                {
                  Do something with the handle, path, and percentage
                }
                </pre>
                <br>
                <br>
                The <b>hnd</b> value is the CZMIL API file handle.  The path argument is a
                pointer to the fully qualified file name (CWF or CPF) that is being indexed.
                The <b>percent</b> value is the percentage complete.  After defining the
                above progress callback function you would register it by doing the following:
                <br>
                <br>
                czmil_register_progress_callback (progressCB);

*********************************************************************************************/

CZMIL_DLL void czmil_register_progress_callback (CZMIL_PROGRESS_CALLBACK progressCB)
{
  czmil_progress_callback = progressCB;
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cwf_header

 - Purpose:     Write the CWF ASCII file header to the CZMIL CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_WRITE_FSEEK_ERROR
                - CZMIL_CWF_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_cwf_header (int32_t hnd)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Write the tagged ASCII fields to the ASCII header.  Why not use XML?  Because these files are going to be HUGE!
      The header is only a few tens of thousands of bytes.  If you mistakenly think this is XML and you try to open
      the file with an XML reader it will not be pretty.  The other reason is that XML is designed for MUCH more
      complicated data.  It also usually requires a schema and/or external stuff.  We don't need all that complexity.
      Remember the KISS principle - Keep It Simple Stupid.  */

  fprintf (cwf[hnd].fp, N_("\n[VERSION] = %s\n"), cwf[hnd].header.version);


  /*  Break out the version numbers.  */

  czmil_get_version_numbers (cwf[hnd].header.version, &cwf[hnd].major_version, &cwf[hnd].minor_version);


  fprintf (cwf[hnd].fp, N_("[FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Waveform File\n"));


  /*  Year, month, day, etc. are provided so that you can easily see the times in the ASCII header.  These are not read
      back in when reading the header.  The timestamps are the important parts.  */

  czmil_cvtime (cwf[hnd].header.creation_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cwf[hnd].fp, N_("[CREATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cwf[hnd].fp, N_("[CREATION TIMESTAMP] = %"PRIu64"\n"), cwf[hnd].header.creation_timestamp);


  if (strlen (cwf[hnd].header.creation_software) > 2) fprintf (cwf[hnd].fp, N_("[CREATION SOFTWARE] = %s\n"), cwf[hnd].header.creation_software);

  czmil_cvtime (cwf[hnd].header.modification_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cwf[hnd].fp, N_("[MODIFICATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cwf[hnd].fp, N_("[MODIFICATION TIMESTAMP] = %"PRIu64"\n"), cwf[hnd].header.modification_timestamp);


  if (strlen (cwf[hnd].header.modification_software) > 2) fprintf (cwf[hnd].fp, N_("[MODIFICATION SOFTWARE] = %s\n"),
                                                                   cwf[hnd].header.modification_software);

  fprintf (cwf[hnd].fp, N_("[NUMBER OF RECORDS] = %d\n"), cwf[hnd].header.number_of_records);

  fprintf (cwf[hnd].fp, N_("[HEADER SIZE] = %d\n"), cwf[hnd].header.header_size);

  fprintf (cwf[hnd].fp, N_("[FILE SIZE] = %"PRIu64"\n"), cwf[hnd].header.file_size);


  for (i = 0 ; i < 7 ; i++)
    {
      fprintf (cwf[hnd].fp, N_("[SHALLOW CHANNEL %d VALIDITY] = %d\n"), i + 1, (int32_t) cwf[hnd].header.channel_valid[i]);
    }
  fprintf (cwf[hnd].fp, N_("[IR CHANNEL VALIDITY] = %d\n"), (int32_t) cwf[hnd].header.channel_valid[CZMIL_IR_CHANNEL]);
  fprintf (cwf[hnd].fp, N_("[DEEP CHANNEL VALIDITY] = %d\n"), (int32_t) cwf[hnd].header.channel_valid[CZMIL_DEEP_CHANNEL]);

  fprintf (cwf[hnd].fp, N_("[SYSTEM TYPE] = %d\n"), cwf[hnd].header.system_type);
  fprintf (cwf[hnd].fp, N_("[SYSTEM NUMBER] = %d\n"), cwf[hnd].header.system_number);
  fprintf (cwf[hnd].fp, N_("[SYSTEM REP RATE] = %d\n"), cwf[hnd].header.rep_rate);

  if (strlen (cwf[hnd].header.project) > 2) fprintf (cwf[hnd].fp, N_("[PROJECT] = %s\n"), cwf[hnd].header.project);
  if (strlen (cwf[hnd].header.mission) > 2) fprintf (cwf[hnd].fp, N_("[MISSION] = %s\n"), cwf[hnd].header.mission);
  if (strlen (cwf[hnd].header.dataset) > 2) fprintf (cwf[hnd].fp, N_("[DATASET] = %s\n"), cwf[hnd].header.dataset);
  if (strlen (cwf[hnd].header.flight_id) > 2) fprintf (cwf[hnd].fp, N_("[FLIGHT ID] = %s\n"), cwf[hnd].header.flight_id);


  czmil_cvtime (cwf[hnd].header.flight_start_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cwf[hnd].fp, N_("[FLIGHT START DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cwf[hnd].fp, N_("[FLIGHT START TIMESTAMP] = %"PRIu64"\n"), cwf[hnd].header.flight_start_timestamp);


  czmil_cvtime (cwf[hnd].header.flight_end_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cwf[hnd].fp, N_("[FLIGHT END DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cwf[hnd].fp, N_("[FLIGHT END TIMESTAMP] = %"PRIu64"\n"), cwf[hnd].header.flight_end_timestamp);


  if (strlen (cwf[hnd].header.comments) > 2) fprintf (cwf[hnd].fp, N_("{COMMENTS = \n%s\n}\n"), cwf[hnd].header.comments);


  fprintf (cwf[hnd].fp, N_("\n########## [FORMAT INFORMATION] ##########\n\n"));


  fprintf (cwf[hnd].fp, N_("[BUFFER SIZE BYTES] = %d\n"), cwf[hnd].buffer_size_bytes);
  fprintf (cwf[hnd].fp, N_("[TYPE BITS] = %hd\n"), cwf[hnd].type_bits);
  fprintf (cwf[hnd].fp, N_("[TYPE 1 START BITS] = %hd\n"), cwf[hnd].type_1_start_bits);
  fprintf (cwf[hnd].fp, N_("[TYPE 2 START BITS] = %hd\n"), cwf[hnd].type_2_start_bits);
  fprintf (cwf[hnd].fp, N_("[DELTA BITS] = %hd\n"), cwf[hnd].delta_bits);
  fprintf (cwf[hnd].fp, N_("[CZMIL MAX PACKETS] = %hd\n"), cwf[hnd].czmil_max_packets);
  fprintf (cwf[hnd].fp, N_("[TIME BITS] = %d\n"), cwf[hnd].time_bits);
  fprintf (cwf[hnd].fp, N_("[ANGLE SCALE] = %f\n"), cwf[hnd].angle_scale);
  fprintf (cwf[hnd].fp, N_("[SCAN ANGLE BITS] = %d\n"), cwf[hnd].scan_angle_bits);
  fprintf (cwf[hnd].fp, N_("[PACKET NUMBER BITS] = %d\n"), cwf[hnd].packet_number_bits);
  fprintf (cwf[hnd].fp, N_("[RANGE BITS] = %d\n"), cwf[hnd].range_bits);
  fprintf (cwf[hnd].fp, N_("[RANGE SCALE] = %f\n"), cwf[hnd].range_scale);
  fprintf (cwf[hnd].fp, N_("[SHOT ID BITS] = %d\n"), cwf[hnd].shot_id_bits);


  /****************************************** VERSION CHECK ******************************************

      This field did not exist prior to major version 2.  We don't want confusion so we
      don't add this field to the header of a version 1 file.

  ***************************************************************************************************/

  if (cwf[hnd].major_version >= 2) fprintf (cwf[hnd].fp, N_("[VALIDITY REASON BITS] = %d\n"), cwf[hnd].validity_reason_bits);


  /*  If we have application defined tagged fields we want to write them to the header prior to writing the end of header tag.  */

  if (cwf[hnd].app_tags_pos) fwrite (cwf[hnd].app_tags, cwf[hnd].app_tags_pos, 1, cwf[hnd].fp);


  fprintf (cwf[hnd].fp, N_("\n########## [END OF HEADER] ##########\n"));


  /*  Space fill the rest of the header block.  */

  size = cwf[hnd].header.header_size - ftello64 (cwf[hnd].fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, cwf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_HEADER_WRITE_ERROR);
        }
    }


  cwf[hnd].pos = cwf[hnd].header.header_size;
  cwf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_cwf_header

 - Purpose:     Read the CWF ASCII file header from the CZMIL CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NOT_CZMIL_FILE_ERROR

 - Caveats:     DO NOT put a trailing line-feed (\n) on multi-line fields like the comments
                field.  You may put line-feeds in the body of the field to make it more
                readable but the API will add a final line-feed prior to the trailing
                bracket (}).

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_read_cwf_header (int32_t hnd)
{
  uint16_t major_version, minor_version;
  int32_t i, tmp;
  char varin[8192], info[8192], channel_valid_string[128], *s;
  uint8_t app_end = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      open a binary file.  If we try to use czmil_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault since fgets doesn't check for overrun.  */

  if (!fread (varin, 128, 1, cwf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Check for the CZMIL library string at the beginning of the file.  */

  if (!strstr (varin, N_("CZMIL library")))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number anyway.  */

  fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cwf[hnd].fp))
    {
      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Check for application defined tagged fields.  These aren't used by the API but we want to preserve them.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]")))
        {
          strcpy (&cwf[hnd].app_tags[cwf[hnd].app_tags_pos], varin);
          cwf[hnd].app_tags_pos += strlen (varin);


          /*  Note that we're using fgets because we want the end of line characters.  */

          while (fgets (varin, sizeof (varin), cwf[hnd].fp))
            {
              /*  Check for end of header.  */

              if (strstr (varin, N_("[END OF HEADER]")))
                {
                  app_end = 1;
                  break;
                }

              strcpy (&cwf[hnd].app_tags[cwf[hnd].app_tags_pos], varin);
              cwf[hnd].app_tags_pos += strlen (varin);
            }


          /*  If we encountered the end of header sentinel while reading app tags we need to end.  */

          if (app_end) break;
        }

          
      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {

          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);


          /*  Read the version string and check the major version number against the library major version.  */

          if (strstr (varin, N_("[VERSION]")))
            {
              strcpy (cwf[hnd].header.version, info);
              czmil_get_version_numbers (cwf[hnd].header.version, &cwf[hnd].major_version, &cwf[hnd].minor_version);

              czmil_get_version_numbers (CZMIL_VERSION, &major_version, &minor_version);

              if (cwf[hnd].major_version > major_version)
                {
                  sprintf (czmil_error.info, _("File : %s\nThe file version is newer than the CZMIL library version.\nThis may cause problems.\n"),
                           cpf[hnd].path);
                  czmil_error.czmil = CZMIL_NEWER_FILE_VERSION_WARNING;
                }
            }

          if (strstr (varin, N_("[FILE TYPE]"))) strcpy (cwf[hnd].header.file_type, info);

          if (strstr (varin, N_("[CREATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cwf[hnd].header.creation_timestamp);
          if (strstr (varin, N_("[MODIFICATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cwf[hnd].header.modification_timestamp);


          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &cwf[hnd].header.number_of_records);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &cwf[hnd].header.header_size);

          if (strstr (varin, N_("[FILE SIZE]"))) sscanf (info, "%"PRIu64, &cwf[hnd].header.file_size);


          for (i = 0 ; i < 7 ; i++)
            {
              sprintf (channel_valid_string, N_("[SHALLOW CHANNEL %d VALIDITY]"), i + 1);
              if (strstr (varin, channel_valid_string))
		{
		  sscanf (info, "%d", &tmp);
		  cwf[hnd].header.channel_valid[i] = (uint8_t) tmp;
		}
            }

          if (strstr (varin, N_("[IR CHANNEL VALIDITY]")))
            {
              sscanf (info, "%d", &tmp);
              cwf[hnd].header.channel_valid[CZMIL_IR_CHANNEL] = (uint8_t) tmp;
            }

          if (strstr (varin, N_("[DEEP CHANNEL VALIDITY]")))
            {
              sscanf (info, "%d", &tmp);
              cwf[hnd].header.channel_valid[CZMIL_DEEP_CHANNEL] = (uint8_t) tmp;
            }


          if (strstr (varin, N_("[SYSTEM TYPE]"))) sscanf (info, "%hd", &cwf[hnd].header.system_type);
          if (strstr (varin, N_("[SYSTEM NUMBER]"))) sscanf (info, "%hd", &cwf[hnd].header.system_number);
          if (strstr (varin, N_("[SYSTEM REP RATE]"))) sscanf (info, "%d", &cwf[hnd].header.rep_rate);

          if (strstr (varin, N_("[PROJECT]"))) strcpy (cwf[hnd].header.project, info);
          if (strstr (varin, N_("[MISSION]"))) strcpy (cwf[hnd].header.mission, info);
          if (strstr (varin, N_("[DATASET]"))) strcpy (cwf[hnd].header.dataset, info);
          if (strstr (varin, N_("[FLIGHT ID]"))) strcpy (cwf[hnd].header.flight_id, info);

          if (strstr (varin, N_("[FLIGHT START TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cwf[hnd].header.flight_start_timestamp);
          if (strstr (varin, N_("[FLIGHT END TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cwf[hnd].header.flight_end_timestamp);


          if (strstr (varin, N_("{COMMENTS =")))
            {
              strcpy (cwf[hnd].header.comments, "");
              while (fgets (varin, sizeof (varin), cwf[hnd].fp))
                {
                  if (varin[0] == '}')
                    {
                      //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                      s = cwf[hnd].header.comments;

                      while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                      if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                      break;
                    }

                  strcat (cwf[hnd].header.comments, varin);
                }
            }

          if (strstr (varin, N_("[BUFFER SIZE BYTES]"))) sscanf (info, "%hd", &cwf[hnd].buffer_size_bytes);
          if (strstr (varin, N_("[TYPE BITS]"))) sscanf (info, "%hd", &cwf[hnd].type_bits);
          if (strstr (varin, N_("[TYPE 1 START BITS]"))) sscanf (info, "%hd", &cwf[hnd].type_1_start_bits);
          if (strstr (varin, N_("[TYPE 2 START BITS]"))) sscanf (info, "%hd", &cwf[hnd].type_2_start_bits);
          if (strstr (varin, N_("[DELTA BITS]"))) sscanf (info, "%hd", &cwf[hnd].delta_bits);
          if (strstr (varin, N_("[CZMIL MAX PACKETS]"))) sscanf (info, "%hd", &cwf[hnd].czmil_max_packets);
          if (strstr (varin, N_("[TIME BITS]"))) sscanf (info, "%hd", &cwf[hnd].time_bits);
          if (strstr (varin, N_("[ANGLE SCALE]"))) sscanf (info, "%f", &cwf[hnd].angle_scale);
          if (strstr (varin, N_("[SCAN ANGLE BITS]"))) sscanf (info, "%hd", &cwf[hnd].scan_angle_bits);
          if (strstr (varin, N_("[PACKET NUMBER BITS]"))) sscanf (info, "%hd", &cwf[hnd].packet_number_bits);
          if (strstr (varin, N_("[RANGE BITS]"))) sscanf (info, "%hd", &cwf[hnd].range_bits);
          if (strstr (varin, N_("[RANGE SCALE]"))) sscanf (info, "%f", &cwf[hnd].range_scale);
          if (strstr (varin, N_("[SHOT ID BITS]"))) sscanf (info, "%hd", &cwf[hnd].shot_id_bits);
          if (strstr (varin, N_("[VALIDITY REASON BITS]"))) sscanf (info, "%hd", &cwf[hnd].validity_reason_bits);
        }
    }


  /*  Compute the remaining field definitions from the input header data.  */

  cwf[hnd].type_0_bytes = (cwf[hnd].type_bits + 64 * 10) / 8;
  if ((cwf[hnd].type_bits + 64 * 10) % 8) cwf[hnd].type_0_bytes++;
  cwf[hnd].type_1_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].type_1_offset = (uint16_t) (power2[cwf[hnd].type_1_start_bits] - 1);
  cwf[hnd].type_2_offset_bits = cwf[hnd].type_2_start_bits + 1;
  cwf[hnd].type_2_offset = (uint16_t) (power2[cwf[hnd].type_2_start_bits] - 1);
  cwf[hnd].type_3_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].type_1_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_2_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].type_2_start_bits +
    cwf[hnd].type_2_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_3_header_bits = cwf[hnd].type_bits + cwf[hnd].type_3_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].time_max = (uint32_t) (power2[cwf[hnd].time_bits] - 1);
  cwf[hnd].packet_number_max = (uint32_t) (power2[cwf[hnd].packet_number_bits] - 1);


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  cwf[hnd].num_packets_bits = czmil_short_log2 (cwf[hnd].czmil_max_packets) + 1;

  cwf[hnd].range_max = (uint32_t) power2[cwf[hnd].range_bits];
  cwf[hnd].shot_id_max = (uint32_t) (power2[cwf[hnd].shot_id_bits] - 1);
  cwf[hnd].validity_reason_max = (uint32_t) (power2[cwf[hnd].validity_reason_bits] - 1);


  /*  Seek to the end of the header.  */

  fseeko64 (cwf[hnd].fp, cwf[hnd].header.header_size, SEEK_SET);
  cwf[hnd].pos = cwf[hnd].header.header_size;
  cwf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cpf_header

 - Purpose:     Write the CPF ASCII file header to the CZMIL CPF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_WRITE_FSEEK_ERROR
                - CZMIL_CPF_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_cpf_header (int32_t hnd)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Write the tagged ASCII fields to the ASCII header.  Why not use XML?  Because these files are going to be HUGE!
      The header is only a few tens of thousands of bytes.  If you mistakenly think this is XML and you try to open
      the file with an XML reader it will not be pretty.  The other reason is that XML is designed for MUCH more
      complicated data.  It also usually requires a schema and/or external stuff.  We don't need all that complexity.
      Remember the KISS principle - Keep It Simple Stupid.  */

  fprintf (cpf[hnd].fp, N_("\n[VERSION] = %s\n"), cpf[hnd].header.version);


  /*  Break out the version numbers.  */

  czmil_get_version_numbers (cpf[hnd].header.version, &cpf[hnd].major_version, &cpf[hnd].minor_version);


  fprintf (cpf[hnd].fp, N_("[FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Point File\n"));


  /*  Year, month, day, etc. are provided so that you can easily see the times in the ASCII header.  These are not read
      back in when reading the header.  The timestamps are the important parts.  */

  czmil_cvtime (cpf[hnd].header.creation_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cpf[hnd].fp, N_("[CREATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cpf[hnd].fp, N_("[CREATION TIMESTAMP] = %"PRIu64"\n"), cpf[hnd].header.creation_timestamp);


  if (strlen (cpf[hnd].header.creation_software) > 2) fprintf (cpf[hnd].fp, N_("[CREATION SOFTWARE] = %s\n"), cpf[hnd].header.creation_software);

  czmil_cvtime (cpf[hnd].header.modification_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cpf[hnd].fp, N_("[MODIFICATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cpf[hnd].fp, N_("[MODIFICATION TIMESTAMP] = %"PRIu64"\n"), cpf[hnd].header.modification_timestamp);


  if (strlen (cpf[hnd].header.modification_software) > 2) fprintf (cpf[hnd].fp, N_("[MODIFICATION SOFTWARE] = %s\n"),
                                                                   cpf[hnd].header.modification_software);

  fprintf (cpf[hnd].fp, N_("[MIN LONGITUDE] = %.11f\n"), cpf[hnd].header.min_lon);
  fprintf (cpf[hnd].fp, N_("[MIN LATITUDE] = %.11f\n"), cpf[hnd].header.min_lat);
  fprintf (cpf[hnd].fp, N_("[MAX LONGITUDE] = %.11f\n"), cpf[hnd].header.max_lon);
  fprintf (cpf[hnd].fp, N_("[MAX LATITUDE] = %.11f\n"), cpf[hnd].header.max_lat);

  fprintf (cpf[hnd].fp, N_("[BASE LONGITUDE] = %.11f\n"), cpf[hnd].header.base_lon - 180.0);
  fprintf (cpf[hnd].fp, N_("[BASE LATITUDE] = %.11f\n"), cpf[hnd].header.base_lat - 90.0);

  fprintf (cpf[hnd].fp, N_("[NUMBER OF RECORDS] = %d\n"), cpf[hnd].header.number_of_records);

  fprintf (cpf[hnd].fp, N_("[HEADER SIZE] = %d\n"), cpf[hnd].header.header_size);

  fprintf (cpf[hnd].fp, N_("[FILE SIZE] = %"PRIu64"\n"), cpf[hnd].header.file_size);

  fprintf (cpf[hnd].fp, N_("[SYSTEM TYPE] = %d\n"), cpf[hnd].header.system_type);
  fprintf (cpf[hnd].fp, N_("[SYSTEM NUMBER] = %d\n"), cpf[hnd].header.system_number);
  fprintf (cpf[hnd].fp, N_("[SYSTEM REP RATE] = %d\n"), cpf[hnd].header.rep_rate);

  if (strlen (cpf[hnd].header.project) > 2) fprintf (cpf[hnd].fp, N_("[PROJECT] = %s\n"), cpf[hnd].header.project);
  if (strlen (cpf[hnd].header.mission) > 2) fprintf (cpf[hnd].fp, N_("[MISSION] = %s\n"), cpf[hnd].header.mission);
  if (strlen (cpf[hnd].header.dataset) > 2) fprintf (cpf[hnd].fp, N_("[DATASET] = %s\n"), cpf[hnd].header.dataset);
  if (strlen (cpf[hnd].header.flight_id) > 2) fprintf (cpf[hnd].fp, N_("[FLIGHT ID] = %s\n"), cpf[hnd].header.flight_id);


  /*  Year, month, day, etc. are provided so that you can easily see the start and end times of the flight.  These are not read
      back in when reading the header.  The start and end timestamps are the important parts.  */

  czmil_cvtime (cpf[hnd].header.flight_start_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cpf[hnd].fp, N_("[FLIGHT START DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cpf[hnd].fp, N_("[FLIGHT START TIMESTAMP] = %"PRIu64"\n"), cpf[hnd].header.flight_start_timestamp);


  czmil_cvtime (cpf[hnd].header.flight_end_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cpf[hnd].fp, N_("[FLIGHT END DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cpf[hnd].fp, N_("[FLIGHT END TIMESTAMP] = %"PRIu64"\n"), cpf[hnd].header.flight_end_timestamp);


  fprintf (cpf[hnd].fp, N_("[NULL Z VALUE] = %.5f\n"), cpf[hnd].header.null_z_value);


  /*  If this field is not defined by the calling program we will use the default WGS84/WGS84E definition.  This field allows applications to warp
      the data to different datums (using GDAL/OGR for example).  */

  if (!strstr (cpf[hnd].header.wkt, N_("DATUM")))
    {
      fprintf (cpf[hnd].fp, N_("{WELL-KNOWN TEXT = \nCOMPD_CS[\"WGS84 with WGS84E Z\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9108\"]],AXIS[\"Lat\",NORTH],AXIS[\"Long\",EAST],AUTHORITY[\"EPSG\",\"4326\"]],VERT_CS[\"ellipsoid Z in meters\",VERT_DATUM[\"Ellipsoid\",2002],UNIT[\"metre\",1],AXIS[\"Z\",UP]]]\n}\n"));
    }
  else
    {
      fprintf (cpf[hnd].fp, N_("{WELL-KNOWN TEXT = \n%s\n}\n"), cpf[hnd].header.wkt);
    }


  fprintf (cpf[hnd].fp, N_("[LOCAL VERTICAL DATUM] = %02d\n"), cpf[hnd].header.local_vertical_datum);

  if (strlen (cpf[hnd].header.comments) > 2) fprintf (cpf[hnd].fp, N_("{COMMENTS = \n%s\n}\n"), cpf[hnd].header.comments);

  if (strlen (cpf[hnd].header.user_data_description) > 2) fprintf (cpf[hnd].fp, N_("{USER DATA DESCRIPTION = \n%s\n}\n"),
                                                                   cpf[hnd].header.user_data_description);


  fprintf (cpf[hnd].fp, N_("\n########## [FORMAT INFORMATION] ##########\n\n"));


  fprintf (cpf[hnd].fp, N_("[BUFFER SIZE BYTES] = %d\n"), cpf[hnd].buffer_size_bytes);
  fprintf (cpf[hnd].fp, N_("[CZMIL MAX RETURNS] = %d\n"), cpf[hnd].return_max);
  fprintf (cpf[hnd].fp, N_("[TIME BITS] = %d\n"), cpf[hnd].time_bits);
  fprintf (cpf[hnd].fp, N_("[ANGLE SCALE] = %f\n"), cpf[hnd].angle_scale);
  fprintf (cpf[hnd].fp, N_("[OFF NADIR ANGLE BITS] = %d\n"), cpf[hnd].off_nadir_angle_bits);
  fprintf (cpf[hnd].fp, N_("[LAT BITS] = %d\n"), cpf[hnd].lat_bits);
  fprintf (cpf[hnd].fp, N_("[LON BITS] = %d\n"), cpf[hnd].lon_bits);
  fprintf (cpf[hnd].fp, N_("[LAT SCALE] = %f\n"), cpf[hnd].lat_scale);
  fprintf (cpf[hnd].fp, N_("[LON SCALE] = %f\n"), cpf[hnd].lon_scale);
  fprintf (cpf[hnd].fp, N_("[LAT DIFF SCALE] = %f\n"), cpf[hnd].lat_diff_scale);
  fprintf (cpf[hnd].fp, N_("[LON DIFF SCALE] = %f\n"), cpf[hnd].lon_diff_scale);
  fprintf (cpf[hnd].fp, N_("[LAT DIFF BITS] = %d\n"), cpf[hnd].lat_diff_bits);
  fprintf (cpf[hnd].fp, N_("[LON DIFF BITS] = %d\n"), cpf[hnd].lon_diff_bits);
  fprintf (cpf[hnd].fp, N_("[ELEV BITS] = %d\n"), cpf[hnd].elev_bits);
  fprintf (cpf[hnd].fp, N_("[ELEV SCALE] = %f\n"), cpf[hnd].elev_scale);
  fprintf (cpf[hnd].fp, N_("[UNCERT BITS] = %d\n"), cpf[hnd].uncert_bits);
  fprintf (cpf[hnd].fp, N_("[UNCERT SCALE] = %f\n"), cpf[hnd].uncert_scale);
  fprintf (cpf[hnd].fp, N_("[REFLECTANCE BITS] = %d\n"), cpf[hnd].reflectance_bits);
  fprintf (cpf[hnd].fp, N_("[REFLECTANCE SCALE] = %f\n"), cpf[hnd].reflectance_scale);
  fprintf (cpf[hnd].fp, N_("[RETURN STATUS BITS] = %d\n"), cpf[hnd].return_status_bits);
  fprintf (cpf[hnd].fp, N_("[RETURN CLASS BITS] = %d\n"), cpf[hnd].class_bits);
  fprintf (cpf[hnd].fp, N_("[CZMIL MAX PACKETS] = %hd\n"), cpf[hnd].czmil_max_packets);
  fprintf (cpf[hnd].fp, N_("[INTEREST POINT SCALE] = %f\n"), cpf[hnd].interest_point_scale);
  fprintf (cpf[hnd].fp, N_("[KD BITS] = %d\n"), cpf[hnd].kd_bits);
  fprintf (cpf[hnd].fp, N_("[KD SCALE] = %f\n"), cpf[hnd].kd_scale);
  fprintf (cpf[hnd].fp, N_("[LASER ENERGY BITS] = %d\n"), cpf[hnd].laser_energy_bits);
  fprintf (cpf[hnd].fp, N_("[LASER ENERGY SCALE] = %f\n"), cpf[hnd].laser_energy_scale);


  /****************************************** VERSION CHECK ******************************************

      These fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 3)
    {
      fprintf (cpf[hnd].fp, N_("[IP_RANK BITS] = %d\n"), cpf[hnd].ip_rank_bits);
      fprintf (cpf[hnd].fp, N_("[USER DATA BITS] = %d\n"), cpf[hnd].user_data_bits);
      fprintf (cpf[hnd].fp, N_("[D_INDEX BITS] = %d\n"), cpf[hnd].d_index_bits);
      fprintf (cpf[hnd].fp, N_("[D_INDEX_CUBE BITS] = %d\n"), cpf[hnd].d_index_cube_bits);
    }


  /****************************************** VERSION CHECK ******************************************

      These fields did not exist prior to major version 2.  We don't want confusion so we
      don't add these fields to the header of a version 1 file.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 2)
    {
      fprintf (cpf[hnd].fp, N_("[PROBABILITY SCALE] = %f\n"), cpf[hnd].probability_scale);
      fprintf (cpf[hnd].fp, N_("[PROBABILITY BITS] = %d\n"), cpf[hnd].probability_bits);
      fprintf (cpf[hnd].fp, N_("[RETURN FILTER REASON BITS] = %d\n"), cpf[hnd].return_filter_reason_bits);
      fprintf (cpf[hnd].fp, N_("[OPTECH CLASSIFICATION BITS] = %d\n"), cpf[hnd].optech_classification_bits);


      /*  USER DATA BITS replaced this in version 3 (above).  */

      if (cpf[hnd].major_version == 2) fprintf (cpf[hnd].fp, N_("[SPARE BITS] = %d\n"), cpf[hnd].user_data_bits);
    }


  /*  Unfortunately we changed the name of an existing field when we went to v2.  */

  if (cpf[hnd].major_version == 1)
    {
      fprintf (cpf[hnd].fp, N_("[SHOT STATUS BITS] = %d\n"), cpf[hnd].user_data_bits);
    }


  /*  If we have application defined tagged fields we want to write them to the header prior to writing the end of header tag.  */

  if (cpf[hnd].app_tags_pos) fwrite (cpf[hnd].app_tags, cpf[hnd].app_tags_pos, 1, cpf[hnd].fp);


  fprintf (cpf[hnd].fp, N_("\n########## [END OF HEADER] ##########\n"));


  /*  Space fill the rest.  */

  size = cpf[hnd].header.header_size - ftello64 (cpf[hnd].fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_HEADER_WRITE_ERROR);
        }
    }


  cpf[hnd].pos = cpf[hnd].header.header_size;
  cpf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_cpf_header

 - Purpose:     Read the CPF ASCII file header from the CZMIL CPF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:     - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NOT_CZMIL_FILE_ERROR

 - Caveats:     DO NOT put a trailing line-feed (\n) on multi-line fields like the comments
                field.  You may put line-feeds in the body of the field to make it more
                readable but the API will add a final line-feed prior to the trailing
                bracket (}).

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_read_cpf_header (int32_t hnd)
{
  uint16_t major_version, minor_version;
  char varin[8192], info[8192], *s;
  uint8_t app_end = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      open a binary file.  If we try to use czmil_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault since fgets doesn't check for overrun.  */

  if (!fread (varin, 128, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Check for the CZMIL library string at the beginning of the file.  */

  if (!strstr (varin, N_("CZMIL library")))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number
      anyway.  */

  fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cpf[hnd].fp))
    {
      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Check for application defined tagged fields.  These aren't used by the API but we want to preserve them.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]")))
        {
          strcpy (&cpf[hnd].app_tags[cpf[hnd].app_tags_pos], varin);
          cpf[hnd].app_tags_pos += strlen (varin);


          /*  Note that we're using fgets because we want the end of line characters.  */

          while (fgets (varin, sizeof (varin), cpf[hnd].fp))
            {
              /*  Check for end of header.  */

              if (strstr (varin, N_("[END OF HEADER]")))
                {
                  app_end = 1;
                  break;
                }

              strcpy (&cpf[hnd].app_tags[cpf[hnd].app_tags_pos], varin);
              cpf[hnd].app_tags_pos += strlen (varin);
            }


          /*  If we encountered the end of header sentinel while reading app tags we need to end.  */

          if (app_end) break;
        }

          
      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {

          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);


          /*  Read the version string and check the major version number against the library major version.  */

          if (strstr (varin, N_("[VERSION]")))
            {
              strcpy (cpf[hnd].header.version, info);
              czmil_get_version_numbers (cpf[hnd].header.version, &cpf[hnd].major_version, &cpf[hnd].minor_version);

              czmil_get_version_numbers (CZMIL_VERSION, &major_version, &minor_version);

              if (cpf[hnd].major_version > major_version)
                {
                  sprintf (czmil_error.info, _("File : %s\nThe file version is newer than the CZMIL library version.\nThis may cause problems.\n"),
                           cpf[hnd].path);
                  czmil_error.czmil = CZMIL_NEWER_FILE_VERSION_WARNING;
                }
            }

          if (strstr (varin, N_("[FILE TYPE]"))) strcpy (cpf[hnd].header.file_type, info);

          if (strstr (varin, N_("[CREATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cpf[hnd].header.creation_timestamp);
          if (strstr (varin, N_("[MODIFICATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cpf[hnd].header.modification_timestamp);


          if (strstr (varin, N_("[MIN LONGITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.min_lon);
          if (strstr (varin, N_("[MIN LATITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.min_lat);
          if (strstr (varin, N_("[MAX LONGITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.max_lon);
          if (strstr (varin, N_("[MAX LATITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.max_lat);

          if (strstr (varin, N_("[BASE LONGITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.base_lon);
          if (strstr (varin, N_("[BASE LATITUDE]"))) sscanf (info, "%lf", &cpf[hnd].header.base_lat);

          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &cpf[hnd].header.number_of_records);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &cpf[hnd].header.header_size);

          if (strstr (varin, N_("[FILE SIZE]"))) sscanf (info, "%"PRIu64, &cpf[hnd].header.file_size);

          if (strstr (varin, N_("[SYSTEM TYPE]"))) sscanf (info, "%hd", &cpf[hnd].header.system_type);
          if (strstr (varin, N_("[SYSTEM NUMBER]"))) sscanf (info, "%hd", &cpf[hnd].header.system_number);
          if (strstr (varin, N_("[SYSTEM REP RATE]"))) sscanf (info, "%d", &cpf[hnd].header.rep_rate);

          if (strstr (varin, N_("[PROJECT]"))) strcpy (cpf[hnd].header.project, info);
          if (strstr (varin, N_("[MISSION]"))) strcpy (cpf[hnd].header.mission, info);
          if (strstr (varin, N_("[DATASET]"))) strcpy (cpf[hnd].header.dataset, info);
          if (strstr (varin, N_("[FLIGHT ID]"))) strcpy (cpf[hnd].header.flight_id, info);


          if (strstr (varin, N_("[FLIGHT START TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cpf[hnd].header.flight_start_timestamp);
          if (strstr (varin, N_("[FLIGHT END TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cpf[hnd].header.flight_end_timestamp);


          if (strstr (varin, N_("[NULL Z VALUE]"))) sscanf (info, "%f", &cpf[hnd].header.null_z_value);

          if (strstr (varin, N_("{WELL-KNOWN TEXT =")))
            {
              strcpy (cpf[hnd].header.wkt, "");
              while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                {
                  if (varin[0] == '}')
                    {
                      //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                      s = cpf[hnd].header.wkt;

                      while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                      if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                      break;
                    }

                  strcat (cpf[hnd].header.wkt, varin);
                }


              /************************************************** IMPORTANT NOTE *******************************************************/

              /*  At one point (version 3.00 to be specific) I decided that, because QGIS couldn't handle a composite coordinate system,
                  COMPD_CS was incorrect.  When I did that I changed the code to strip off the COMPD_CS section and the trailing bracket
                  (]) but leave the VERT_CS section.  I now have a much better handle on Well-known Text (WKT) Coordinate Reference
                  Systems (CRS) and have determined that COMPD_CS is perfectly acceptable.  Since CZMIL 3.00 and/or 3.01 may have removed
                  the COMPD_CS verbiage from existing files I've added this little piece of code to make sure that they get restored to
                  what they were supposed to be.  Since 3.00/3.01 aren't really in production yet this probably will only affect test
                  files.  Sorry about that.  JCD 10/17/2016  */

              if (!strncmp (cpf[hnd].header.wkt, "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]]", 98) &&
                  strstr (cpf[hnd].header.wkt, "VERT_CS"))
                {
                  strcpy (cpf[hnd].header.wkt, N_("{WELL-KNOWN TEXT = \nCOMPD_CS[\"WGS84 with WGS84E Z\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9108\"]],AXIS[\"Lat\",NORTH],AXIS[\"Long\",EAST],AUTHORITY[\"EPSG\",\"4326\"]],VERT_CS[\"ellipsoid Z in meters\",VERT_DATUM[\"Ellipsoid\",2002],UNIT[\"metre\",1],AXIS[\"Z\",UP]]]\n}\n"));
                }
            }

          if (strstr (varin, N_("[LOCAL VERTICAL DATUM]"))) sscanf (info, "%hd", &cpf[hnd].header.local_vertical_datum);

          if (strstr (varin, N_("{COMMENTS =")))
            {
              strcpy (cpf[hnd].header.comments, "");
              while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                {
                  if (varin[0] == '}')
                    {
                      //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                      s = cpf[hnd].header.comments;

                      while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                      if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                      break;
                    }

                  strcat (cpf[hnd].header.comments, varin);
                }
            }

          if (strstr (varin, N_("{USER DATA DESCRIPTION =")))
            {
              strcpy (cpf[hnd].header.user_data_description, "");
              while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                {
                  if (varin[0] == '}')
                    {
                      //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                      s = cpf[hnd].header.user_data_description;

                      while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                      if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                      break;
                    }

                  strcat (cpf[hnd].header.user_data_description, varin);
                }
            }

          if (strstr (varin, N_("[BUFFER SIZE BYTES]"))) sscanf (info, "%hd", &cpf[hnd].buffer_size_bytes);
          if (strstr (varin, N_("[CZMIL MAX RETURNS]"))) sscanf (info, "%hd", &cpf[hnd].return_max);
          if (strstr (varin, N_("[TIME BITS]"))) sscanf (info, "%hd", &cpf[hnd].time_bits);
          if (strstr (varin, N_("[ANGLE SCALE]"))) sscanf (info, "%f", &cpf[hnd].angle_scale);
          if (strstr (varin, N_("[OFF NADIR ANGLE BITS]"))) sscanf (info, "%hd", &cpf[hnd].off_nadir_angle_bits);
          if (strstr (varin, N_("[LAT BITS]"))) sscanf (info, "%hd", &cpf[hnd].lat_bits);
          if (strstr (varin, N_("[LON BITS]"))) sscanf (info, "%hd", &cpf[hnd].lon_bits);
          if (strstr (varin, N_("[LAT SCALE]"))) sscanf (info, "%lf", &cpf[hnd].lat_scale);
          if (strstr (varin, N_("[LON SCALE]"))) sscanf (info, "%lf", &cpf[hnd].lon_scale);
          if (strstr (varin, N_("[LAT DIFF SCALE]"))) sscanf (info, "%lf", &cpf[hnd].lat_diff_scale);
          if (strstr (varin, N_("[LON DIFF SCALE]"))) sscanf (info, "%lf", &cpf[hnd].lon_diff_scale);
          if (strstr (varin, N_("[LAT DIFF BITS]"))) sscanf (info, "%hd", &cpf[hnd].lat_diff_bits);
          if (strstr (varin, N_("[LON DIFF BITS]"))) sscanf (info, "%hd", &cpf[hnd].lon_diff_bits);
          if (strstr (varin, N_("[ELEV BITS]"))) sscanf (info, "%hd", &cpf[hnd].elev_bits);
          if (strstr (varin, N_("[ELEV SCALE]"))) sscanf (info, "%f", &cpf[hnd].elev_scale);
          if (strstr (varin, N_("[UNCERT BITS]"))) sscanf (info, "%hd", &cpf[hnd].uncert_bits);
          if (strstr (varin, N_("[UNCERT SCALE]"))) sscanf (info, "%f", &cpf[hnd].uncert_scale);
          if (strstr (varin, N_("[REFLECTANCE BITS]"))) sscanf (info, "%hd", &cpf[hnd].reflectance_bits);
          if (strstr (varin, N_("[REFLECTANCE SCALE]"))) sscanf (info, "%f", &cpf[hnd].reflectance_scale);
          if (strstr (varin, N_("[RETURN STATUS BITS]"))) sscanf (info, "%hd", &cpf[hnd].return_status_bits);


          /*  In v1 files the user_data bit size field was called SHOT STATUS BITS or VALIDITY REASON BITS in some test files.  The field was never 
              used so any of these get translated to user_data_bits.  */

          if (strstr (varin, N_("[SHOT STATUS BITS]"))) sscanf (info, "%hd", &cpf[hnd].user_data_bits);
          if (strstr (varin, N_("[VALIDITY REASON BITS]"))) sscanf (info, "%hd", &cpf[hnd].user_data_bits);
          if (strstr (varin, N_("[SPARE BITS]"))) sscanf (info, "%hd", &cpf[hnd].user_data_bits);
          if (strstr (varin, N_("[USER DATA BITS]"))) sscanf (info, "%hd", &cpf[hnd].user_data_bits);


          if (strstr (varin, N_("[RETURN CLASS BITS]"))) sscanf (info, "%hd", &cpf[hnd].class_bits);
          if (strstr (varin, N_("[CZMIL MAX PACKETS]"))) sscanf (info, "%hd", &cpf[hnd].czmil_max_packets);
          if (strstr (varin, N_("[INTEREST POINT SCALE]"))) sscanf (info, "%f", &cpf[hnd].interest_point_scale);
          if (strstr (varin, N_("[KD BITS]"))) sscanf (info, "%hd", &cpf[hnd].kd_bits);
          if (strstr (varin, N_("[KD SCALE]"))) sscanf (info, "%f", &cpf[hnd].kd_scale);
          if (strstr (varin, N_("[LASER ENERGY BITS]"))) sscanf (info, "%hd", &cpf[hnd].laser_energy_bits);
          if (strstr (varin, N_("[LASER ENERGY SCALE]"))) sscanf (info, "%f", &cpf[hnd].laser_energy_scale);
          if (strstr (varin, N_("[PROBABILITY BITS]"))) sscanf (info, "%hd", &cpf[hnd].probability_bits);
          if (strstr (varin, N_("[PROBABILITY SCALE]"))) sscanf (info, "%f", &cpf[hnd].probability_scale);
          if (strstr (varin, N_("[RETURN FILTER REASON BITS]"))) sscanf (info, "%hd", &cpf[hnd].return_filter_reason_bits);
          if (strstr (varin, N_("[OPTECH CLASSIFICATION BITS]"))) sscanf (info, "%hd", &cpf[hnd].optech_classification_bits);
          if (strstr (varin, N_("[IP_RANK BITS]"))) sscanf (info, "%hd", &cpf[hnd].ip_rank_bits);
          if (strstr (varin, N_("[D_INDEX BITS]"))) sscanf (info, "%hd", &cpf[hnd].d_index_bits);
          if (strstr (varin, N_("[D_INDEX_CUBE BITS]"))) sscanf (info, "%hd", &cpf[hnd].d_index_cube_bits);
        }
    }


  /*  Compute the rest of the needed fields.  */

  cpf[hnd].lat_max = (uint32_t) (power2[cpf[hnd].lat_bits] - 1);
  cpf[hnd].lat_offset = cpf[hnd].lat_max / 2;
  cpf[hnd].lon_max = (uint32_t) (power2[cpf[hnd].lon_bits] - 1);
  cpf[hnd].lon_offset = cpf[hnd].lon_max / 2;
  cpf[hnd].lat_diff_max = (uint32_t) (power2[cpf[hnd].lat_diff_bits] - 1);
  cpf[hnd].lat_diff_offset = cpf[hnd].lat_diff_max / 2;
  cpf[hnd].lon_diff_max = (uint32_t) (power2[cpf[hnd].lon_diff_bits] - 1);
  cpf[hnd].lon_diff_offset = cpf[hnd].lon_diff_max / 2;
  cpf[hnd].elev_max = (uint32_t) (power2[cpf[hnd].elev_bits] - 1);
  cpf[hnd].elev_offset = cpf[hnd].elev_max / 2;
  cpf[hnd].uncert_max = (uint32_t) (power2[cpf[hnd].uncert_bits] - 1);
  cpf[hnd].reflectance_max = (uint32_t) (power2[cpf[hnd].reflectance_bits] - 1);
  cpf[hnd].class_max = (uint32_t) (power2[cpf[hnd].class_bits] - 1);
  cpf[hnd].off_nadir_angle_max = (uint32_t) (power2[cpf[hnd].off_nadir_angle_bits] - 1);
  cpf[hnd].off_nadir_angle_offset = cpf[hnd].off_nadir_angle_max / 2;
  cpf[hnd].time_max = (uint32_t) (power2[cpf[hnd].time_bits] - 1);
  cpf[hnd].return_status_max = (uint32_t) (power2[cpf[hnd].return_status_bits] - 1);


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  cpf[hnd].return_bits = czmil_short_log2 (cpf[hnd].return_max) + 1;

  cpf[hnd].interest_point_bits = (uint16_t) czmil_int_log2 (NINT ((float) (cpf[hnd].czmil_max_packets * 64) * cpf[hnd].interest_point_scale)) + 1;
  cpf[hnd].interest_point_max = (uint32_t) (power2[cpf[hnd].interest_point_bits] - 1);
  cpf[hnd].laser_energy_max = (uint32_t) (power2[cpf[hnd].laser_energy_bits] - 1);
  cpf[hnd].probability_max = (uint32_t) (power2[cpf[hnd].probability_bits] - 1);
  cpf[hnd].return_filter_reason_max = (uint32_t) (power2[cpf[hnd].return_filter_reason_bits] - 1);
  cpf[hnd].optech_classification_max = (uint32_t) (power2[cpf[hnd].optech_classification_bits] - 1);
  cpf[hnd].d_index_max = (uint32_t) (power2[cpf[hnd].d_index_bits] - 1);
  cpf[hnd].d_index_cube_max = (uint32_t) (power2[cpf[hnd].d_index_cube_bits] - 1);


  /****************************************** VERSION CHECK ******************************************

      These fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version < 3)
    {
      /*  In previous versions, the ip_rank field required the same number of bits as the return number.  */

      cpf[hnd].ip_rank_bits = cpf[hnd].return_bits;
    }


  /*  Bias the latitude and longitude after reading them from the file header.  Internally they will be biased
      by 90 and 180 respectively.  */

  cpf[hnd].header.base_lat += 90.0;
  cpf[hnd].header.base_lon += 180.0;


  /*  Seek to the end of the header.  */

  fseeko64 (cpf[hnd].fp, cpf[hnd].header.header_size, SEEK_SET);
  cpf[hnd].pos = cpf[hnd].header.header_size;
  cpf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_csf_header

 - Purpose:     Write the CSF ASCII file header to the CZMIL CSF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_WRITE_FSEEK_ERROR
                - CZMIL_CSF_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_csf_header (int32_t hnd)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Write the tagged ASCII fields to the ASCII header.  Why not use XML?  Because these files are going to be HUGE!
      The header is only a few tens of thousands of bytes.  If you mistakenly think this is XML and you try to open
      the file with an XML reader it will not be pretty.  The other reason is that XML is designed for MUCH more
      complicated data.  It also usually requires a schema and/or external stuff.  We don't need all that complexity.
      Remember the KISS principle - Keep It Simple Stupid.  */

  fprintf (csf[hnd].fp, N_("\n[VERSION] = %s\n"), csf[hnd].header.version);


  /*  Break out the version numbers.  */

  czmil_get_version_numbers (csf[hnd].header.version, &csf[hnd].major_version, &csf[hnd].minor_version);


  fprintf (csf[hnd].fp, N_("[FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) SBET File\n"));


  /*  Year, month, day, etc. are provided so that you can easily see the times in the ASCII header.  These are not read
      back in when reading the header.  The timestamps are the important parts.  */

  czmil_cvtime (csf[hnd].header.creation_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (csf[hnd].fp, N_("[CREATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (csf[hnd].fp, N_("[CREATION TIMESTAMP] = %"PRIu64"\n"), csf[hnd].header.creation_timestamp);


  if (strlen (csf[hnd].header.creation_software) > 2) fprintf (csf[hnd].fp, N_("[CREATION SOFTWARE] = %s\n"),
                                                                     csf[hnd].header.creation_software);

  czmil_cvtime (csf[hnd].header.modification_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (csf[hnd].fp, N_("[MODIFICATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (csf[hnd].fp, N_("[MODIFICATION TIMESTAMP] = %"PRIu64"\n"), csf[hnd].header.modification_timestamp);


  if (strlen (csf[hnd].header.modification_software) > 2) fprintf (csf[hnd].fp, N_("[MODIFICATION SOFTWARE] = %s\n"),
                                                                             csf[hnd].header.modification_software);

  fprintf (csf[hnd].fp, N_("[NUMBER OF RECORDS] = %d\n"), csf[hnd].header.number_of_records);

  fprintf (csf[hnd].fp, N_("[HEADER SIZE] = %d\n"), csf[hnd].header.header_size);

  fprintf (csf[hnd].fp, N_("[FILE SIZE] = %"PRIu64"\n"), csf[hnd].header.file_size);

  if (strlen (csf[hnd].header.project) > 2) fprintf (csf[hnd].fp, N_("[PROJECT] = %s\n"), csf[hnd].header.project);
  if (strlen (csf[hnd].header.mission) > 2) fprintf (csf[hnd].fp, N_("[MISSION] = %s\n"), csf[hnd].header.mission);
  if (strlen (csf[hnd].header.dataset) > 2) fprintf (csf[hnd].fp, N_("[DATASET] = %s\n"), csf[hnd].header.dataset);
  if (strlen (csf[hnd].header.flight_id) > 2) fprintf (csf[hnd].fp, N_("[FLIGHT ID] = %s\n"), csf[hnd].header.flight_id);


  /*  Year, month, day, etc. are provided so that you can easily see the start and end times of the flight.  These are not read
      back in when reading the header.  The start and end timestamps are the important parts.  */

  czmil_cvtime (csf[hnd].header.flight_start_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (csf[hnd].fp, N_("[FLIGHT START DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (csf[hnd].fp, N_("[FLIGHT START TIMESTAMP] = %"PRIu64"\n"), csf[hnd].header.flight_start_timestamp);


  czmil_cvtime (csf[hnd].header.flight_end_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (csf[hnd].fp, N_("[FLIGHT END DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (csf[hnd].fp, N_("[FLIGHT END TIMESTAMP] = %"PRIu64"\n"), csf[hnd].header.flight_end_timestamp);


  fprintf (csf[hnd].fp, N_("[MIN LONGITUDE] = %.11f\n"), csf[hnd].header.min_lon);
  fprintf (csf[hnd].fp, N_("[MIN LATITUDE] = %.11f\n"), csf[hnd].header.min_lat);
  fprintf (csf[hnd].fp, N_("[MAX LONGITUDE] = %.11f\n"), csf[hnd].header.max_lon);
  fprintf (csf[hnd].fp, N_("[MAX LATITUDE] = %.11f\n"), csf[hnd].header.max_lat);

  fprintf (csf[hnd].fp, N_("[BASE LONGITUDE] = %.11f\n"), csf[hnd].header.base_lon - 180.0);
  fprintf (csf[hnd].fp, N_("[BASE LATITUDE] = %.11f\n"), csf[hnd].header.base_lat - 90.0);


  fprintf (csf[hnd].fp, N_("\n########## [FORMAT INFORMATION] ##########\n\n"));


  fprintf (csf[hnd].fp, N_("[TIME BITS] = %d\n"), csf[hnd].time_bits);
  fprintf (csf[hnd].fp, N_("[LAT BITS] = %d\n"), csf[hnd].lat_bits);
  fprintf (csf[hnd].fp, N_("[LON BITS] = %d\n"), csf[hnd].lon_bits);
  fprintf (csf[hnd].fp, N_("[LAT SCALE] = %f\n"), csf[hnd].lat_scale);
  fprintf (csf[hnd].fp, N_("[LON SCALE] = %f\n"), csf[hnd].lon_scale);
  fprintf (csf[hnd].fp, N_("[ALTITUDE SCALE] = %f\n"), csf[hnd].alt_scale);
  fprintf (csf[hnd].fp, N_("[ALTITUDE BITS] = %d\n"), csf[hnd].alt_bits);
  fprintf (csf[hnd].fp, N_("[ANGLE SCALE] = %f\n"), csf[hnd].angle_scale);
  fprintf (csf[hnd].fp, N_("[SCAN ANGLE BITS] = %d\n"), csf[hnd].scan_angle_bits);
  fprintf (csf[hnd].fp, N_("[ROLL AND PITCH BITS] = %d\n"), csf[hnd].roll_pitch_bits);
  fprintf (csf[hnd].fp, N_("[HEADING BITS] = %d\n"), csf[hnd].heading_bits);
  fprintf (csf[hnd].fp, N_("[RANGE BITS] = %d\n"), csf[hnd].range_bits);
  fprintf (csf[hnd].fp, N_("[RANGE SCALE] = %f\n"), csf[hnd].range_scale);


  /****************************************** VERSION CHECK ******************************************

      These fields did not exist prior to major version 2.  We don't want confusion so we
      don't add these fields to the header of a version 1 file.

  ***************************************************************************************************/

  if (csf[hnd].major_version >= 2)
    {
      fprintf (csf[hnd].fp, N_("[INTENSITY BITS] = %d\n"), csf[hnd].intensity_bits);
      fprintf (csf[hnd].fp, N_("[INTENSITY SCALE] = %f\n"), csf[hnd].intensity_scale);
    }


  /*  If we have application defined tagged fields we want to write them to the header prior to writing the end of header tag.  */

  if (csf[hnd].app_tags_pos) fwrite (csf[hnd].app_tags, csf[hnd].app_tags_pos, 1, csf[hnd].fp);


  fprintf (csf[hnd].fp, N_("\n########## [END OF HEADER] ##########\n"));


  /*  Space fill the rest.  */

  size = csf[hnd].header.header_size - ftello64 (csf[hnd].fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, csf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_HEADER_WRITE_ERROR);
        }
    }


  csf[hnd].pos = csf[hnd].header.header_size;
  csf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_csf_header

 - Purpose:     Read the CSF ASCII file header from the CZMIL CSF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:     - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NOT_CZMIL_FILE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_read_csf_header (int32_t hnd)
{
  int32_t total_bits;
  uint16_t major_version, minor_version;
  char varin[8192], info[8192];
  uint8_t app_end = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      open a binary file.  If we try to use czmil_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault since fgets doesn't check for overrun.  */

  if (!fread (varin, 128, 1, csf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Check for the CZMIL library string at the beginning of the file.  */

  if (!strstr (varin, N_("CZMIL library")))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number
      anyway.  */

  fseeko64 (csf[hnd].fp, 0LL, SEEK_SET);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), csf[hnd].fp))
    {
      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Check for application defined tagged fields.  These aren't used by the API but we want to preserve them.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]")))
        {
          strcpy (&csf[hnd].app_tags[csf[hnd].app_tags_pos], varin);
          csf[hnd].app_tags_pos += strlen (varin);


          /*  Note that we're using fgets because we want the end of line characters.  */

          while (fgets (varin, sizeof (varin), csf[hnd].fp))
            {
              /*  Check for end of header.  */

              if (strstr (varin, N_("[END OF HEADER]")))
                {
                  app_end = 1;
                  break;
                }

              strcpy (&csf[hnd].app_tags[csf[hnd].app_tags_pos], varin);
              csf[hnd].app_tags_pos += strlen (varin);
            }


          /*  If we encountered the end of header sentinel while reading app tags we need to end.  */

          if (app_end) break;
        }

          
      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {

          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);


          /*  Read the version string and check the major version number against the library major version.  */

          if (strstr (varin, N_("[VERSION]")))
            {
              strcpy (csf[hnd].header.version, info);
              czmil_get_version_numbers (csf[hnd].header.version, &csf[hnd].major_version, &csf[hnd].minor_version);

              czmil_get_version_numbers (CZMIL_VERSION, &major_version, &minor_version);

              if (csf[hnd].major_version > major_version)
                {
                  sprintf (czmil_error.info, _("File : %s\nThe file version is newer than the CZMIL library version.\nThis may cause problems.\n"),
                           csf[hnd].path);
                  czmil_error.czmil = CZMIL_NEWER_FILE_VERSION_WARNING;
                }
            }

          if (strstr (varin, N_("[FILE TYPE]"))) strcpy (csf[hnd].header.file_type, info);

          if (strstr (varin, N_("[CREATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &csf[hnd].header.creation_timestamp);
          if (strstr (varin, N_("[MODIFICATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &csf[hnd].header.modification_timestamp);


          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &csf[hnd].header.number_of_records);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &csf[hnd].header.header_size);

          if (strstr (varin, N_("[FILE SIZE]"))) sscanf (info, "%"PRIu64, &csf[hnd].header.file_size);

          if (strstr (varin, N_("[PROJECT]"))) strcpy (csf[hnd].header.project, info);
          if (strstr (varin, N_("[MISSION]"))) strcpy (csf[hnd].header.mission, info);
          if (strstr (varin, N_("[DATASET]"))) strcpy (csf[hnd].header.dataset, info);
          if (strstr (varin, N_("[FLIGHT ID]"))) strcpy (csf[hnd].header.flight_id, info);

          if (strstr (varin, N_("[FLIGHT START TIMESTAMP]"))) sscanf (info, "%"PRIu64, &csf[hnd].header.flight_start_timestamp);
          if (strstr (varin, N_("[FLIGHT END TIMESTAMP]"))) sscanf (info, "%"PRIu64, &csf[hnd].header.flight_end_timestamp);

          if (strstr (varin, N_("[MIN LONGITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.min_lon);
          if (strstr (varin, N_("[MIN LATITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.min_lat);
          if (strstr (varin, N_("[MAX LONGITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.max_lon);
          if (strstr (varin, N_("[MAX LATITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.max_lat);

          if (strstr (varin, N_("[BASE LONGITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.base_lon);
          if (strstr (varin, N_("[BASE LATITUDE]"))) sscanf (info, "%lf", &csf[hnd].header.base_lat);

          if (strstr (varin, N_("[TIME BITS]"))) sscanf (info, "%hd", &csf[hnd].time_bits);
          if (strstr (varin, N_("[LAT BITS]"))) sscanf (info, "%hd", &csf[hnd].lat_bits);
          if (strstr (varin, N_("[LON BITS]"))) sscanf (info, "%hd", &csf[hnd].lon_bits);
          if (strstr (varin, N_("[LAT SCALE]"))) sscanf (info, "%lf", &csf[hnd].lat_scale);
          if (strstr (varin, N_("[LON SCALE]"))) sscanf (info, "%lf", &csf[hnd].lon_scale);
          if (strstr (varin, N_("[ALTITUDE SCALE]"))) sscanf (info, "%f", &csf[hnd].alt_scale);
          if (strstr (varin, N_("[ALTITUDE BITS]"))) sscanf (info, "%hd", &csf[hnd].alt_bits);
          if (strstr (varin, N_("[ANGLE SCALE]"))) sscanf (info, "%f", &csf[hnd].angle_scale);
          if (strstr (varin, N_("[SCAN ANGLE BITS]"))) sscanf (info, "%hd", &csf[hnd].scan_angle_bits);
          if (strstr (varin, N_("[ROLL AND PITCH BITS]"))) sscanf (info, "%hd", &csf[hnd].roll_pitch_bits);
          if (strstr (varin, N_("[HEADING BITS]"))) sscanf (info, "%hd", &csf[hnd].heading_bits);
          if (strstr (varin, N_("[RANGE BITS]"))) sscanf (info, "%hd", &csf[hnd].range_bits);
          if (strstr (varin, N_("[RANGE SCALE]"))) sscanf (info, "%f", &csf[hnd].range_scale);
          if (strstr (varin, N_("[INTENSITY BITS]"))) sscanf (info, "%hd", &csf[hnd].intensity_bits);
          if (strstr (varin, N_("[INTENSITY SCALE]"))) sscanf (info, "%f", &csf[hnd].intensity_scale);
        }
    }


  /*  Compute the remaining field definitions from the input header data.  */

  csf[hnd].lat_max = (uint32_t) (power2[csf[hnd].lat_bits] - 1);
  csf[hnd].lat_offset = csf[hnd].lat_max / 2;
  csf[hnd].lon_max = (uint32_t) (power2[csf[hnd].lon_bits] - 1);
  csf[hnd].lon_offset = csf[hnd].lon_max / 2;
  csf[hnd].roll_pitch_max = (uint32_t) (power2[csf[hnd].roll_pitch_bits] - 1);
  csf[hnd].roll_pitch_offset = csf[hnd].roll_pitch_max / 2;
  csf[hnd].time_max = (uint32_t) (power2[csf[hnd].time_bits] - 1);
  csf[hnd].alt_max = (uint32_t) (power2[csf[hnd].alt_bits] - 1);
  csf[hnd].alt_offset = csf[hnd].alt_max / 2;
  csf[hnd].range_max = (uint32_t) power2[csf[hnd].range_bits];
  csf[hnd].intensity_max = (uint32_t) power2[csf[hnd].intensity_bits];


  /****************************************** VERSION CHECK ******************************************

      The range_in_water, intensity, and intensity_in_water fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (csf[hnd].major_version >= 2)
    {
      total_bits = csf[hnd].time_bits + csf[hnd].lat_bits + csf[hnd].lon_bits + csf[hnd].alt_bits + csf[hnd].scan_angle_bits +
        csf[hnd].heading_bits + 2 * csf[hnd].roll_pitch_bits + 9 * csf[hnd].range_bits + 9 * csf[hnd].range_bits + 9 * csf[hnd].intensity_bits +
        9 * csf[hnd].intensity_bits;
    }
  else
    {
      total_bits = csf[hnd].time_bits + csf[hnd].lat_bits + csf[hnd].lon_bits + csf[hnd].alt_bits + csf[hnd].scan_angle_bits +
        csf[hnd].heading_bits + 2 * csf[hnd].roll_pitch_bits + 9 * csf[hnd].range_bits;
    }

  csf[hnd].buffer_size = total_bits / 8;
  if (total_bits % 8) csf[hnd].buffer_size++;


  /*  Bias the latitude and longitude after reading them from the file header.  Internally they will be biased
      by 90 and 180 respectively.  */

  csf[hnd].header.base_lat += 90.0;
  csf[hnd].header.base_lon += 180.0;


  /*  Seek to the end of the header.  */

  fseeko64 (csf[hnd].fp, csf[hnd].header.header_size, SEEK_SET);
  csf[hnd].pos = csf[hnd].header.header_size;
  csf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cif_header

 - Purpose:     Write the CIF ASCII file header to the CZMIL CIF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/14/12

 - Arguments:
                - cif_struct     =    The internal CZMIL CIF structure

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CIF_HEADER_WRITE_FSEEK_ERROR
                - CZMIL_CIF_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_cif_header (INTERNAL_CZMIL_CIF_STRUCT *cif_struct)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cif_struct->fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CIF header :\n%s\n"), cif_struct->path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Write the tagged ASCII fields to the ASCII header.  Why not use XML?  Because these files are going to be HUGE!
      The header is only a few tens of thousands of bytes.  If you mistakenly think this is XML and you try to open
      the file with an XML reader it will not be pretty.  The other reason is that XML is designed for MUCH more
      complicated data.  It also usually requires a schema and/or external stuff.  We don't need all that complexity.
      Remember the KISS principle - Keep It Simple Stupid.  */

  fprintf (cif_struct->fp, N_("\n[VERSION] = %s\n"), cif_struct->header.version);


  /*  Break out the version numbers.  */

  czmil_get_version_numbers (cif_struct->header.version, &cif_struct->major_version, &cif_struct->minor_version);


  fprintf (cif_struct->fp, N_("[FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Index File\n"));


  /*  Year, month, day, etc. are provided so that you can easily see the times in the ASCII header.  These are not read
      back in when reading the header.  The timestamps are the important parts.  */

  czmil_cvtime (cif_struct->header.creation_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (cif_struct->fp, N_("[CREATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (cif_struct->fp, N_("[CREATION TIMESTAMP] = %"PRIu64"\n"), cif_struct->header.creation_timestamp);


  fprintf (cif_struct->fp, N_("[NUMBER OF RECORDS] = %d\n"), cif_struct->header.number_of_records);

  fprintf (cif_struct->fp, N_("[HEADER SIZE] = %d\n"), cif_struct->header.header_size);


  fprintf (cif_struct->fp, N_("\n########## [FORMAT INFORMATION] ##########\n\n"));


  fprintf (cif_struct->fp, N_("[CPF ADDRESS BITS] = %d\n"), cif_struct->header.cpf_address_bits);
  fprintf (cif_struct->fp, N_("[CWF ADDRESS BITS] = %d\n"), cif_struct->header.cwf_address_bits);
  fprintf (cif_struct->fp, N_("[CPF BUFFER SIZE BITS] = %d\n"), cif_struct->header.cpf_buffer_size_bits);
  fprintf (cif_struct->fp, N_("[CWF BUFFER SIZE BITS] = %d\n"), cif_struct->header.cwf_buffer_size_bits);


  fprintf (cif_struct->fp, N_("\n########## [END OF HEADER] ##########\n"));


  /*  Space fill the rest.  */

  size = cif_struct->header.header_size - ftello64 (cif_struct->fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, cif_struct->fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CIF header :\n%s\n"), cif_struct->path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_HEADER_WRITE_ERROR);
        }
    }


  cif_struct->pos = cif_struct->header.header_size;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_cif_header

 - Purpose:     Read the CIF ASCII file header from the CZMIL CIF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/14/12

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CIF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NOT_CZMIL_FILE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_read_cif_header (int32_t hnd)
{
  uint16_t major_version, minor_version;
  char varin[8192], info[8192];


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cif[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CIF header :\n%s\n"), cif[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_HEADER_READ_FSEEK_ERROR);
    }


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      open a binary file.  If we try to use czmil_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault since fgets doesn't check for overrun.  */

  if (!fread (varin, 128, 1, cif[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cif[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Check for the CZMIL library string at the beginning of the file.  */

  if (!strstr (varin, N_("CZMIL library")))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), cif[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number
      anyway.  */

  fseeko64 (cif[hnd].fp, 0LL, SEEK_SET);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cif[hnd].fp))
    {
      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {

          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);


          /*  Read the version string and check the major version number against the library major version.  */

          if (strstr (varin, N_("[VERSION]")))
            {
              strcpy (cif[hnd].header.version, info);
              czmil_get_version_numbers (cif[hnd].header.version, &cif[hnd].major_version, &cif[hnd].minor_version);

              czmil_get_version_numbers (CZMIL_VERSION, &major_version, &minor_version);

              if (cif[hnd].major_version > major_version)
                {
                  sprintf (czmil_error.info, _("File : %s\nThe file version is newer than the CZMIL library version.\nThis may cause problems.\n"),
                           cpf[hnd].path);
                  czmil_error.czmil = CZMIL_NEWER_FILE_VERSION_WARNING;
                }
            }

          if (strstr (varin, N_("[FILE TYPE]"))) strcpy (cif[hnd].header.file_type, info);

          if (strstr (varin, N_("[CREATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &cif[hnd].header.creation_timestamp);

          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &cif[hnd].header.number_of_records);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &cif[hnd].header.header_size);

          if (strstr (varin, N_("[CPF ADDRESS BITS]"))) sscanf (info, "%hd", &cif[hnd].header.cpf_address_bits);
          if (strstr (varin, N_("[CWF ADDRESS BITS]"))) sscanf (info, "%hd", &cif[hnd].header.cwf_address_bits);
          if (strstr (varin, N_("[CPF BUFFER SIZE BITS]"))) sscanf (info, "%hd", &cif[hnd].header.cpf_buffer_size_bits);
          if (strstr (varin, N_("[CWF BUFFER SIZE BITS]"))) sscanf (info, "%hd", &cif[hnd].header.cwf_buffer_size_bits);
        }
    }


  /*  Compute the remaining field definitions from the input header data.  */

  cif[hnd].header.record_size_bytes = (cif[hnd].header.cpf_address_bits + cif[hnd].header.cwf_address_bits + cif[hnd].header.cpf_buffer_size_bits +
                                       cif[hnd].header.cwf_buffer_size_bits) / 8;


  /*  We don't want to seek to the end of the header like we would normally do because czmil_read_cif_record would
      think that we just read the first record if we were to ask to read the first record.  The czmil_read_cif_record
      function tries to avoid re-reading a record if you just read it (as in the case of reading a CPF record followed
      by the associated CWF record).  */

  fseeko64 (cif[hnd].fp, 0LL, SEEK_SET);
  cif[hnd].pos = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_caf_header

 - Purpose:     Write the CAF ASCII file header to the CZMIL CAF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CAF_HEADER_WRITE_FSEEK_ERROR
                - CZMIL_CAF_HEADER_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_caf_header (int32_t hnd)
{
  char space = ' ';
  int32_t i, size, year, jday, hour, minute, month, day;
  float second;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (caf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CAF header :\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_HEADER_WRITE_FSEEK_ERROR);
    }


  /*  Write the tagged ASCII fields to the ASCII header.  */

  fprintf (caf[hnd].fp, N_("\n[VERSION] = %s\n"), caf[hnd].header.version);


  /*  Break out the version numbers.  */

  czmil_get_version_numbers (caf[hnd].header.version, &caf[hnd].major_version, &caf[hnd].minor_version);


  fprintf (caf[hnd].fp, N_("[FILE TYPE] = Optech Coastal Zone Mapping and Imaging LiDAR (CZMIL) Audit File\n"));


  /*  Year, month, day, etc. are provided so that you can easily see the times in the ASCII header.  These are not read
      back in when reading the header.  The timestamps are the important parts.  */

  czmil_cvtime (caf[hnd].header.creation_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (caf[hnd].fp, N_("[CREATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (caf[hnd].fp, N_("[CREATION TIMESTAMP] = %"PRIu64"\n"), caf[hnd].header.creation_timestamp);


  czmil_cvtime (caf[hnd].header.application_timestamp, &year, &jday, &hour, &minute, &second);
  czmil_jday2mday (year, jday, &month, &day);
  month++;

  fprintf (caf[hnd].fp, N_("[APPLICATION DATE] = %d %02d %02d (%03d) %02d:%02d:%02d\n"), year + 1900, month, day, jday, hour, minute, NINT (second));
  fprintf (caf[hnd].fp, N_("[APPLICATION TIMESTAMP] = %"PRIu64"\n"), caf[hnd].header.application_timestamp);


  fprintf (caf[hnd].fp, N_("[NUMBER OF RECORDS] = %d\n"), caf[hnd].header.number_of_records);

  fprintf (caf[hnd].fp, N_("[HEADER SIZE] = %d\n"), caf[hnd].header.header_size);


  fprintf (caf[hnd].fp, N_("\n########## [FORMAT INFORMATION] ##########\n\n"));

  fprintf (caf[hnd].fp, N_("[SHOT ID BITS] = %d\n"), caf[hnd].shot_id_bits);
  fprintf (caf[hnd].fp, N_("[CHANNEL NUMBER BITS] = %d\n"), caf[hnd].channel_number_bits);
  fprintf (caf[hnd].fp, N_("[OPTECH CLASSIFICATION BITS] = %d\n"), caf[hnd].optech_classification_bits);
  fprintf (caf[hnd].fp, N_("[INTEREST POINT SCALE] = %f\n"), caf[hnd].interest_point_scale);
  fprintf (caf[hnd].fp, N_("[CZMIL MAX RETURNS] = %d\n"), caf[hnd].return_max);
  fprintf (caf[hnd].fp, N_("[CZMIL MAX PACKETS] = %hd\n"), caf[hnd].czmil_max_packets);


  fprintf (caf[hnd].fp, N_("\n########## [END OF HEADER] ##########\n"));


  /*  Space fill the rest.  */

  size = caf[hnd].header.header_size - ftello64 (caf[hnd].fp);


  for (i = 0 ; i < size ; i++)
    {
      if (!fwrite (&space, 1, 1, caf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CAF header :\n%s\n"), caf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CAF_HEADER_WRITE_ERROR);
        }
    }


  caf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_caf_header

 - Purpose:     Read the CAF ASCII file header from the CZMIL CAF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The CZMIL file handle

 - Returns:     - CZMIL_SUCCESS
                - CZMIL_CAF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NOT_CZMIL_FILE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_read_caf_header (int32_t hnd)
{
  uint16_t major_version, minor_version;
  char varin[8192], info[8192];


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Position to the beginning of the file.  */

  if (fseeko64 (caf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CAF header :\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_HEADER_READ_FSEEK_ERROR);
    }


  /*  We want to try to read the first line (version info) with an fread in case we mistakenly asked to
      open a binary file.  If we try to use czmil_ngets to read a binary file and there are no line feeds in 
      the first sizeof (varin) characters we would segfault since fgets doesn't check for overrun.  */

  if (!fread (varin, 128, 1, caf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), caf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Check for the CZMIL library string at the beginning of the file.  */

  if (!strstr (varin, N_("CZMIL library")))
    {
      sprintf (czmil_error.info, _("File : %s\nThe file version string is corrupt or indicates that this is not a CZMIL file.\n"), caf[hnd].path);
      return (czmil_error.czmil = CZMIL_NOT_CZMIL_FILE_ERROR);
    }


  /*  Rewind to the beginning of the file.  Yes, we'll read the version again but we need to check the version number
      anyway.  */

  fseeko64 (caf[hnd].fp, 0LL, SEEK_SET);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), caf[hnd].fp))
    {
      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {

          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);


          /*  Read the version string and check the major version number against the library major version.  */

          if (strstr (varin, N_("[VERSION]")))
            {
              strcpy (caf[hnd].header.version, info);
              czmil_get_version_numbers (caf[hnd].header.version, &caf[hnd].major_version, &caf[hnd].minor_version);

              czmil_get_version_numbers (CZMIL_VERSION, &major_version, &minor_version);

              if (caf[hnd].major_version > major_version)
                {
                  sprintf (czmil_error.info, _("File : %s\nThe file version is newer than the CZMIL library version.\nThis may cause problems.\n"),
                           caf[hnd].path);
                  czmil_error.czmil = CZMIL_NEWER_FILE_VERSION_WARNING;
                }
            }

          if (strstr (varin, N_("[FILE TYPE]"))) strcpy (caf[hnd].header.file_type, info);

          if (strstr (varin, N_("[CREATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &caf[hnd].header.creation_timestamp);
          if (strstr (varin, N_("[APPLICATION TIMESTAMP]"))) sscanf (info, "%"PRIu64, &caf[hnd].header.application_timestamp);


          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &caf[hnd].header.number_of_records);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &caf[hnd].header.header_size);

          if (strstr (varin, N_("[SHOT ID BITS]"))) sscanf (info, "%hd", &caf[hnd].shot_id_bits);
          if (strstr (varin, N_("[CHANNEL NUMBER BITS]"))) sscanf (info, "%hd", &caf[hnd].channel_number_bits);
          if (strstr (varin, N_("[OPTECH CLASSIFICATION BITS]"))) sscanf (info, "%hd", &caf[hnd].optech_classification_bits);
          if (strstr (varin, N_("[INTEREST POINT SCALE]"))) sscanf (info, "%f", &caf[hnd].interest_point_scale);
          if (strstr (varin, N_("[CZMIL MAX RETURNS]"))) sscanf (info, "%hd", &caf[hnd].return_max);
          if (strstr (varin, N_("[CZMIL MAX PACKETS]"))) sscanf (info, "%hd", &caf[hnd].czmil_max_packets);
        }
    }


  /*  Compute the rest of the needed fields.  */

  caf[hnd].interest_point_bits = (uint16_t) czmil_int_log2 (NINT ((float) (caf[hnd].czmil_max_packets * 64) * caf[hnd].interest_point_scale)) + 1;
  caf[hnd].interest_point_max = (uint32_t) (power2[caf[hnd].interest_point_bits] - 1);
  caf[hnd].shot_id_max = (uint32_t) (power2[caf[hnd].shot_id_bits] - 1);
  caf[hnd].channel_number_max = 8;


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  caf[hnd].return_bits = czmil_short_log2 (caf[hnd].return_max) + 1;

  caf[hnd].optech_classification_max = (uint32_t) (power2[caf[hnd].optech_classification_bits] - 1);


  /*  Seek to the end of the header.  */

  fseeko64 (caf[hnd].fp, caf[hnd].header.header_size, SEEK_SET);
  caf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_create_cwf_file

 - Purpose:     Create CZMIL CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - cwf_header     =    CZMIL_CWF_Header structure to be written to the file
                - io_buffer_size =    CWF I/O buffer size in bytes.  If set to 0 the default
                                      value of CZMIL_CWF_IO_BUFFER_SIZE will be used.
                                      Since the actual record size is completely variable there
                                      is no way to determine the exact number of records the
                                      io_buffer will hold.  On average the waveforms take about
                                      1800 bytes so a value of 36,000,000 would give you
                                      about 20,000 records in cache prior to it being
                                      written to disk.  This is about 2 seconds of data.

 - NOTE:        Block buffering on creation increases speed significantly.  Creating and using
                my own output buffer seems to be marginally faster than creating the buffer and using
                setvbuf.  In addition, I know exactly what it's doing on every system (as opposed to
                setvbuf).

 - Returns:     
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CWF_CREATE_ERROR
                - Error value from czmil_cwf_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Please look at the CZMIL_CWF_Header structure in the czmil.h file to determine which
                fields must be set by your application in the cwf_header structure prior to creating
                the CWF file.

                This function will probably only be called from IDL by the HydroFusion package.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_create_cwf_file (char *idl_path, int32_t path_length, CZMIL_CWF_Header *cwf_header, int32_t io_buffer_size)
{
  int32_t                    i, hnd;
  char                       path[1024];
  time_t                     t;
  struct tm                  *cur_tm;


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


#ifdef IDL_LOG

      /*  Create an error log file for IDL.  */

      if (log_fp == NULL)
        {
          strcpy (log_path, path);
          sprintf (&log_path[strlen (log_path) - 4], "_API.log");

          if ((log_fp = fopen (log_path, "w+")) == NULL) exit (-1);
        }
#endif

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cwf[i].fp == NULL)
        {
          memset (&cwf[i], 0, sizeof (INTERNAL_CZMIL_CWF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (cwf[hnd].path, path);


  /*  Make sure that the file has a .cwf extension since we will be using the extension to check associated files.  */

  if (strcmp (&path[strlen (path) - 4], ".cwf"))
    {
      sprintf (czmil_error.info, _("File : %s\nInvalid file extension for CZMIL CWF file (must be .cwf)\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_INVALID_FILENAME_ERROR);
    }


  /*  Open the file and write the header.  */

  if ((cwf[hnd].fp = fopen64 (cwf[hnd].path, "wb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL CWF file :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_CREATE_ERROR);
    }


  /*  Make sure that the start and end timestamps are 0 so we can compute them as we build the file.  */

  cwf_header->flight_start_timestamp = cwf_header->flight_end_timestamp = 0;


  cwf[hnd].header = *cwf_header;


  /*  Put the version info into the header.  */

  strcpy (cwf[hnd].header.version, CZMIL_VERSION);


  /*  Check for io_buffer_size being set in the arguments.  */

  if (!io_buffer_size)
    {
      cwf[hnd].io_buffer_size = CZMIL_CWF_IO_BUFFER_SIZE;
    }
  else
    {
      cwf[hnd].io_buffer_size = io_buffer_size;
    }


  /*  Set the default CWF compression bit sizes.  */

  cwf[hnd].buffer_size_bytes = CWF_BUFFER_SIZE_BYTES;
  cwf[hnd].type_bits = CWF_TYPE_BITS;
  cwf[hnd].type_0_bytes = (cwf[hnd].type_bits + 64 * 10) / 8;
  if ((cwf[hnd].type_bits + 64 * 10) % 8) cwf[hnd].type_0_bytes++;
  cwf[hnd].type_1_start_bits = CWF_TYPE_1_START_BITS;
  cwf[hnd].type_1_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].type_1_offset = (uint16_t) (power2[cwf[hnd].type_1_start_bits] - 1);
  cwf[hnd].type_2_start_bits = CWF_TYPE_2_START_BITS;
  cwf[hnd].type_2_offset_bits = cwf[hnd].type_2_start_bits + 1;
  cwf[hnd].type_2_offset = (uint16_t) (power2[cwf[hnd].type_2_start_bits] - 1);
  cwf[hnd].type_3_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].delta_bits = CWF_DELTA_BITS;
  cwf[hnd].time_bits = TIME_BITS;
  cwf[hnd].angle_scale = ANGLE_SCALE;
  cwf[hnd].scan_angle_bits = SCAN_ANGLE_BITS;
  cwf[hnd].type_1_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_2_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].type_2_start_bits +
    cwf[hnd].type_2_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_3_header_bits = cwf[hnd].type_bits + cwf[hnd].type_3_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].time_max = (uint32_t) (power2[cwf[hnd].time_bits] - 1);
  cwf[hnd].packet_number_bits = CWF_PACKET_NUMBER_BITS;
  cwf[hnd].czmil_max_packets = CZMIL_MAX_PACKETS;
  cwf[hnd].packet_number_max = (uint16_t) (power2[cwf[hnd].packet_number_bits] - 1);


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  cwf[hnd].num_packets_bits = czmil_short_log2 (cwf[hnd].czmil_max_packets) + 1;

  cwf[hnd].range_bits = CWF_RANGE_BITS;
  cwf[hnd].range_scale = CWF_RANGE_SCALE;
  cwf[hnd].range_max = (uint32_t) power2[cwf[hnd].range_bits];
  cwf[hnd].shot_id_bits = CWF_SHOT_ID_BITS;
  cwf[hnd].shot_id_max = (uint32_t) (power2[cwf[hnd].shot_id_bits] - 1);
  cwf[hnd].validity_reason_bits = CWF_VALIDITY_REASON_BITS;
  cwf[hnd].validity_reason_max = (uint32_t) (power2[cwf[hnd].validity_reason_bits] - 1);


  /*  Set the header size from the default.  */

  cwf[hnd].header.header_size = CZMIL_CWF_HEADER_SIZE;


  /*  Write the CWF header.  */

  if (czmil_write_cwf_header (hnd) < 0)
    {
      fclose (cwf[hnd].fp);
      cwf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  /*  Allocate the CWF I/O buffer memory.  */

  cwf[hnd].io_buffer = (uint8_t *) malloc (cwf[hnd].io_buffer_size);
  if (cwf[hnd].io_buffer == NULL)
    {
      fclose (cwf[hnd].fp);
      cwf[hnd].fp = NULL;

      sprintf (czmil_error.info, _("Failure allocating CWF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_IO_BUFFER_ALLOCATION_ERROR);
    }
  cwf[hnd].io_buffer_address = 0;

  cwf[hnd].header.number_of_records = 0;


  /*  Now, since we're creating the CWF file we need to create the CIF file.  In this case it will eventually be
      called *.cwi for CZMIL waveform index because we don't have a CPF file when we create the CWF file.  That
      happens in level 2 of HydroFusion.  At that point we'll read the *.cwi file and create a *.cif file.
      Until we finish writing this file it will be called *.cwi.tmp so that if we don't finish normally it won't
      get used improperly.  */

  memset (&cwf[hnd].cif, 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));


  strcpy (cwf[hnd].cif.path, path);
  sprintf (&cwf[hnd].cif.path[strlen (cwf[hnd].cif.path) - 4], ".cwi.tmp");


  /*  Try to create the CIF file.  */

  if ((cwf[hnd].cif.fp = fopen64 (cwf[hnd].cif.path, "wb+")) == NULL)
    {
      free (cwf[hnd].io_buffer);
      fclose (cwf[hnd].fp);
      cwf[hnd].fp = NULL;

      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL TMP CIF file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_CREATE_ERROR);
    }


  /*  Allocate the CIF I/O buffer memory.  */

  cwf[hnd].cif.io_buffer_size = CZMIL_CIF_IO_BUFFER_SIZE;
  cwf[hnd].cif.io_buffer = (uint8_t *) malloc (cwf[hnd].cif.io_buffer_size);
  if (cwf[hnd].cif.io_buffer == NULL)
    {
      free (cwf[hnd].io_buffer);
      fclose (cwf[hnd].fp);
      cwf[hnd].fp = NULL;

      sprintf (czmil_error.info, _("Failure allocating CIF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_IO_BUFFER_ALLOCATION_ERROR);
    }
  cwf[hnd].cif.io_buffer_address = 0;


  cwf[hnd].cif.created = 1;


  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &cwf[hnd].cif.header.creation_timestamp);


  /*  Put the version info into the header.  */

  strcpy (cwf[hnd].cif.header.version, CZMIL_VERSION);


  /*  Set the record field sizes.  */

  cwf[hnd].cif.header.cwf_address_bits = CIF_CWF_ADDRESS_BITS;
  cwf[hnd].cif.header.cpf_address_bits = CIF_CPF_ADDRESS_BITS;
  cwf[hnd].cif.header.cwf_buffer_size_bits = CIF_CWF_BUFFER_SIZE_BITS;
  cwf[hnd].cif.header.cpf_buffer_size_bits = CIF_CPF_BUFFER_SIZE_BITS;
  cwf[hnd].cif.header.record_size_bytes = (cwf[hnd].cif.header.cwf_address_bits + cwf[hnd].cif.header.cpf_address_bits +
                                           cwf[hnd].cif.header.cwf_buffer_size_bits + cwf[hnd].cif.header.cpf_buffer_size_bits) / 8;


  cwf[hnd].cif.header.header_size = CZMIL_CIF_HEADER_SIZE;


  /*  This field will always be the same as the field from the CWF header (which is now 0).  */

  cwf[hnd].cif.header.number_of_records = cwf[hnd].header.number_of_records;


  /*  Write the header to the CIF file (this is placeholder until we're finished).  */

  if (czmil_write_cif_header (&cwf[hnd].cif) < 0) return (czmil_error.czmil);


  /*  We have to set the apparent file position in the CWF file.  */

  cwf[hnd].create_file_pos = cwf[hnd].header.header_size;


  cwf[hnd].at_end = 1;
  cwf[hnd].modified = 1;
  cwf[hnd].created = 1;
  cwf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}


/********************************************************************************************/
/*!

 - Function:    czmil_create_cpf_file

 - Purpose:     Create CZMIL CPF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - cpf_header     =    CZMIL_CPF_Header structure to be written to the file
                - io_buffer_size =    CPF I/O buffer size in bytes.  If set to 0 the
                                      default value of CZMIL_CPF_IO_BUFFER_SIZE will be used.
                                      Since the actual record size is completely variable there
                                      is no way to determine the exact number of records the
                                      I/O buffer will hold.

 - NOTE:        Block buffering on creation increases speed significantly.  Creating and using
                my own output buffer seems to be marginally faster than creating the buffer and using
                setvbuf.  In addition, I know exactly what it's doing on every system (as opposed to
                setvbuf).

 - Returns:     
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CPF_CREATE_ERROR
                - Error value from czmil_cpf_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Please look at the CZMIL_CPF_Header structure in the czmil.h file to determine which
                fields must be set by your application in the cpf_header structure prior to creating
                the CPF file.

                This function will probably only be called from IDL by the HydroFusion package.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_create_cpf_file (char *idl_path, int32_t path_length, CZMIL_CPF_Header *cpf_header, int32_t io_buffer_size)
{
  int32_t i, hnd;
  char path[1024];


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cpf[i].fp == NULL)
        {
          memset (&cpf[i], 0, sizeof (INTERNAL_CZMIL_CPF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (cpf[hnd].path, path);


  /*  Make sure that the file has a .cpf extension since we will be using the extension to check associated files.  */

  if (strcmp (&path[strlen (path) - 4], ".cpf"))
    {
      sprintf (czmil_error.info, _("File : %s\nInvalid file extension for CZMIL CPF file (must be .cpf)\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_INVALID_FILENAME_ERROR);
    }


  /*  Open the file and write the header.  */

  if ((cpf[hnd].fp = fopen64 (cpf[hnd].path, "wb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL CPF file :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_CREATE_ERROR);
    }


  /*  Check for io_buffer_size being set in the arguments.  */

  if (!io_buffer_size)
    {
      cpf[hnd].io_buffer_size = CZMIL_CPF_IO_BUFFER_SIZE;
    }
  else
    {
      cpf[hnd].io_buffer_size = io_buffer_size;
    }


  /*  Make sure that the start and end timestamps are 0 so we can compute them as we build the file.  */

  cpf_header->flight_start_timestamp = cpf_header->flight_end_timestamp = 0;


  cpf[hnd].header = *cpf_header;


  /*  Put the version info into the header.  */

  strcpy (cpf[hnd].header.version, CZMIL_VERSION);


  /*  Set the default packed record bit field sizes and scale factors.  */

  cpf[hnd].buffer_size_bytes = CPF_BUFFER_SIZE_BYTES;
  cpf[hnd].return_max = CZMIL_MAX_RETURNS;
  cpf[hnd].time_bits = TIME_BITS;
  cpf[hnd].angle_scale = ANGLE_SCALE;
  cpf[hnd].off_nadir_angle_bits = CPF_OFF_NADIR_ANGLE_BITS;
  cpf[hnd].lat_bits = CPF_LAT_BITS;
  cpf[hnd].lon_bits = CPF_LAT_BITS;
  cpf[hnd].lat_scale = CPF_LATITUDE_SCALE;
  cpf[hnd].lon_scale = CPF_LONGITUDE_SCALE;
  cpf[hnd].lat_diff_scale = CPF_LAT_DIFF_SCALE;
  cpf[hnd].lon_diff_scale = CPF_LON_DIFF_SCALE;
  cpf[hnd].lat_diff_bits = CPF_LAT_DIFF_BITS;
  cpf[hnd].lon_diff_bits = CPF_LAT_DIFF_BITS;
  cpf[hnd].elev_bits = CPF_ELEV_BITS;
  cpf[hnd].elev_scale = CPF_ELEV_SCALE;
  cpf[hnd].uncert_bits = CPF_UNCERT_BITS;
  cpf[hnd].uncert_scale = CPF_UNCERT_SCALE;
  cpf[hnd].reflectance_bits = CPF_REFLECTANCE_BITS;
  cpf[hnd].reflectance_scale = CPF_REFLECTANCE_SCALE;
  cpf[hnd].return_status_bits = CPF_RETURN_STATUS_BITS;
  cpf[hnd].user_data_bits = CPF_USER_DATA_BITS;
  cpf[hnd].class_bits = CPF_CLASS_BITS;
  cpf[hnd].czmil_max_packets = CZMIL_MAX_PACKETS;
  cpf[hnd].interest_point_scale = CPF_INTEREST_POINT_SCALE;
  cpf[hnd].kd_bits = CPF_KD_BITS;
  cpf[hnd].kd_scale = CPF_KD_SCALE;
  cpf[hnd].laser_energy_bits = CPF_LASER_ENERGY_BITS;
  cpf[hnd].laser_energy_scale = CPF_LASER_ENERGY_SCALE;
  cpf[hnd].probability_bits = CPF_PROBABILITY_BITS;
  cpf[hnd].probability_scale = CPF_PROBABILITY_SCALE;
  cpf[hnd].return_filter_reason_bits = CPF_RETURN_FILTER_REASON_BITS;
  cpf[hnd].optech_classification_bits = CPF_OPTECH_CLASSIFICATION_BITS;
  cpf[hnd].ip_rank_bits = CPF_IP_RANK_BITS;
  cpf[hnd].d_index_bits = CPF_D_INDEX_BITS;
  cpf[hnd].d_index_cube_bits = CPF_D_INDEX_CUBE_BITS;


  /*  Compute the rest of the field size information from the defaults.  */

  cpf[hnd].lat_max = (uint32_t) (power2[cpf[hnd].lat_bits] - 1);
  cpf[hnd].lat_offset = cpf[hnd].lat_max / 2;
  cpf[hnd].lon_max = (uint32_t) (power2[cpf[hnd].lon_bits] - 1);
  cpf[hnd].lon_offset = cpf[hnd].lon_max / 2;
  cpf[hnd].lat_diff_max = (uint32_t) (power2[cpf[hnd].lat_diff_bits] - 1);
  cpf[hnd].lat_diff_offset = cpf[hnd].lat_diff_max / 2;
  cpf[hnd].lon_diff_max = (uint32_t) (power2[cpf[hnd].lon_diff_bits] - 1);
  cpf[hnd].lon_diff_offset = cpf[hnd].lon_diff_max / 2;
  cpf[hnd].elev_max = (uint32_t) (power2[cpf[hnd].elev_bits] - 1);
  cpf[hnd].elev_offset = cpf[hnd].elev_max / 2;
  cpf[hnd].uncert_max = (uint32_t) (power2[cpf[hnd].uncert_bits] - 1);
  cpf[hnd].reflectance_max = (uint32_t) (power2[cpf[hnd].reflectance_bits] - 1);
  cpf[hnd].class_max = (uint32_t) (power2[cpf[hnd].class_bits] - 1);
  cpf[hnd].off_nadir_angle_max = (uint32_t) (power2[cpf[hnd].off_nadir_angle_bits] - 1);
  cpf[hnd].off_nadir_angle_offset = cpf[hnd].off_nadir_angle_max / 2;
  cpf[hnd].time_max = (uint32_t) (power2[cpf[hnd].time_bits] - 1);
  cpf[hnd].return_status_max = (uint32_t) (power2[cpf[hnd].return_status_bits] - 1);


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  cpf[hnd].return_bits = czmil_short_log2 (cpf[hnd].return_max) + 1;

  cpf[hnd].interest_point_bits = (uint16_t) czmil_int_log2 (NINT ((float) (cpf[hnd].czmil_max_packets * 64) * cpf[hnd].interest_point_scale)) + 1;
  cpf[hnd].interest_point_max = (uint32_t) (power2[cpf[hnd].interest_point_bits] - 1);
  cpf[hnd].laser_energy_max = (uint32_t) (power2[cpf[hnd].laser_energy_bits] - 1);
  cpf[hnd].probability_max = (uint32_t) (power2[cpf[hnd].probability_bits] - 1);
  cpf[hnd].return_filter_reason_max = (uint32_t) (power2[cpf[hnd].return_filter_reason_bits] - 1);
  cpf[hnd].optech_classification_max = (uint32_t) (power2[cpf[hnd].optech_classification_bits] - 1);
  cpf[hnd].d_index_max = (uint32_t) (power2[cpf[hnd].d_index_bits] - 1);
  cpf[hnd].d_index_cube_max = (uint32_t) (power2[cpf[hnd].d_index_cube_bits] - 1);


  /*  Initialize the record count.  */

  cpf[hnd].header.number_of_records = 0;


  /*  Set the header size from the defaults.  */

  cpf[hnd].header.header_size = CZMIL_CPF_HEADER_SIZE;


  /*  Set the CPF observed min and max values to some ridiculous number so they'll get replaced immediately.  */

  cpf[hnd].header.min_lon = cpf[hnd].header.min_lat = 999.0;
  cpf[hnd].header.max_lon = cpf[hnd].header.max_lat = -999.0;


  /*  Bias the latitude and longitude prior to writing them to the file header.  Internally they will be biased
      by 90 and 180 respectively.  The header writing routine will unbias them prior to writing.  */

  cpf[hnd].header.base_lat += 90.0;
  cpf[hnd].header.base_lon += 180.0;


  /*  Write the CPF header.  */

  if (czmil_write_cpf_header (hnd) < 0)
    {
      fclose (cpf[hnd].fp);
      cpf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  /*  Set the apparent file position for use when creating the new CIF file (which was, or will be, setup in
      czmil_open_cwf_file if the *.cwi file was/is available at the time the CWF file was/is opened).  */

  cpf[hnd].create_file_pos = ftello64 (cpf[hnd].fp);


  /*  Allocate the CPF I/O buffer memory.  */

  cpf[hnd].io_buffer = (uint8_t *) malloc (cpf[hnd].io_buffer_size);
  if (cpf[hnd].io_buffer == NULL)
    {
      fclose (cpf[hnd].fp);
      cpf[hnd].fp = NULL;

      sprintf (czmil_error.info, _("Failure allocating CPF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_IO_BUFFER_ALLOCATION_ERROR);
    }


  cpf[hnd].header.number_of_records = 0;

  cpf[hnd].at_end = 1;
  cpf[hnd].modified = 1;
  cpf[hnd].created = 1;
  cpf[hnd].write = 1;


  /*  Un-bias the latitude and longitude before we return.  We only wanted to use this biased internally and don't want
      to pass a modified base lat/lon back to the caller.  */

  cpf_header->base_lat -= 90.0;
  cpf_header->base_lon -= 180.0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}


/********************************************************************************************/
/*!

 - Function:    czmil_create_csf_file

 - Purpose:     Create CZMIL CSF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - csf_header     =    CZMIL_CSF_Header structure to be written to the file
                - io_buffer_size =    CSF I/O buffer size in bytes.  If set to 0 the
                                      default value of CZMIL_CSF_IO_BUFFER_SIZE will be used.
                                      The CSF records are fixed length and are, at present
                                      (July 2012) about 25 bytes in length.  So a buffer size
                                      of 15,000,000 would hold one minute of CSF data.

 - NOTE:        Block buffering on creation increases speed significantly.  Creating and using
                my own output buffer seems to be marginally faster than creating the buffer and using
                setvbuf.  In addition, I know exactly what it's doing on every system (as opposed to
                setvbuf).

 - Returns:     
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CSF_CREATE_ERROR
                - Error value from czmil_csf_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Please look at the CZMIL_CSF_Header structure in the czmil.h file to determine which
                fields must be set by your application in the csf_header structure prior to creating
                the CSF file.

                This function will probably only be called from IDL by the HydroFusion package.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_create_csf_file (char *idl_path, int32_t path_length, CZMIL_CSF_Header *csf_header, int32_t io_buffer_size)
{
  int32_t i, hnd, total_bits;
  char path[1024];


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (csf[i].fp == NULL)
        {
          memset (&csf[i], 0, sizeof (INTERNAL_CZMIL_CSF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (csf[hnd].path, path);


  /*  Make sure that the file has a .csf extension since we will be using the extension to check associated files.  */

  if (strcmp (&path[strlen (path) - 4], ".csf"))
    {
      sprintf (czmil_error.info, _("File : %s\nInvalid file extension for CZMIL CSF file (must be .csf)\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_INVALID_FILENAME_ERROR);
    }


  /*  Open the file and write the header.  */

  if ((csf[hnd].fp = fopen64 (csf[hnd].path, "wb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL CSF file :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_CREATE_ERROR);
    }


  /*  Check for io_buffer_size being set in the arguments.  */

  if (!io_buffer_size)
    {
      csf[hnd].io_buffer_size = CZMIL_CSF_IO_BUFFER_SIZE;
    }
  else
    {
      csf[hnd].io_buffer_size = io_buffer_size;
    }


  /*  Make sure that the start and end timestamps are 0 so we can compute them as we build the file.  */

  csf_header->flight_start_timestamp = csf_header->flight_end_timestamp = 0;


  csf[hnd].header = *csf_header;


  /*  Put the version info into the header.  */

  strcpy (csf[hnd].header.version, CZMIL_VERSION);


  /*  Set the default packed record bit field sizes and scale factors.  */

  csf[hnd].angle_scale = ANGLE_SCALE;
  csf[hnd].scan_angle_bits = SCAN_ANGLE_BITS;
  csf[hnd].roll_pitch_bits = CSF_ROLL_PITCH_BITS;
  csf[hnd].heading_bits = CSF_HEADING_BITS;
  csf[hnd].time_bits = TIME_BITS;
  csf[hnd].lat_bits = CSF_LAT_BITS;
  csf[hnd].lon_bits = CSF_LAT_BITS;
  csf[hnd].lat_scale = CSF_LATITUDE_SCALE;
  csf[hnd].lon_scale = CSF_LONGITUDE_SCALE;
  csf[hnd].alt_scale = CSF_ALTITUDE_SCALE;
  csf[hnd].alt_bits = CSF_ALTITUDE_BITS;
  csf[hnd].range_bits = CSF_RANGE_BITS;
  csf[hnd].range_scale = CSF_RANGE_SCALE;
  csf[hnd].intensity_bits = CSF_INTENSITY_BITS;
  csf[hnd].intensity_scale = CSF_INTENSITY_SCALE;


  /*  Compute the rest of the field size information from the defaults.  */

  csf[hnd].lat_max = (uint32_t) (power2[csf[hnd].lat_bits] - 1);
  csf[hnd].lat_offset = csf[hnd].lat_max / 2;
  csf[hnd].lon_max = (uint32_t) (power2[csf[hnd].lon_bits] - 1);
  csf[hnd].lon_offset = csf[hnd].lon_max / 2;
  csf[hnd].alt_max = (uint32_t) (power2[csf[hnd].alt_bits] - 1);
  csf[hnd].alt_offset = csf[hnd].alt_max / 2;
  csf[hnd].roll_pitch_max = (uint32_t) (power2[csf[hnd].roll_pitch_bits] - 1);
  csf[hnd].roll_pitch_offset = csf[hnd].roll_pitch_max / 2;
  csf[hnd].time_max = (uint32_t) (power2[csf[hnd].time_bits] - 1);
  csf[hnd].range_max = (uint32_t) power2[csf[hnd].range_bits];
  csf[hnd].intensity_max = (uint32_t) power2[csf[hnd].intensity_bits];


  /*  Set the header size from the defaults.  */

  csf[hnd].header.header_size = CZMIL_CSF_HEADER_SIZE;


  /*  Set the CSF observed min and max values to some ridiculous number so they'll get replaced immediately.  */

  csf[hnd].header.min_lon = csf[hnd].header.min_lat = 999.0;
  csf[hnd].header.max_lon = csf[hnd].header.max_lat = -999.0;


  /*  Bias the latitude and longitude prior to writing them to the file header.  Internally they will be biased
      by 90 and 180 respectively.  The header writing routine will unbias them prior to writing.  */

  csf[hnd].header.base_lat += 90.0;
  csf[hnd].header.base_lon += 180.0;


  /*  Write the CSF header (which, by the way, populates the major and minor version numbers in the header).  */

  if (czmil_write_csf_header (hnd) < 0)
    {
      fclose (csf[hnd].fp);
      csf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  /****************************************** VERSION CHECK ******************************************

      The range_in_water, intensity, and intensity_in_water fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (csf[hnd].major_version >= 2)
    {
      total_bits = csf[hnd].time_bits + csf[hnd].lat_bits + csf[hnd].lon_bits + csf[hnd].alt_bits + csf[hnd].scan_angle_bits +
        csf[hnd].heading_bits + 2 * csf[hnd].roll_pitch_bits + 9 * csf[hnd].range_bits + 9 * csf[hnd].range_bits + 9 * csf[hnd].intensity_bits +
        9 * csf[hnd].intensity_bits;
    }
  else
    {
      total_bits = csf[hnd].time_bits + csf[hnd].lat_bits + csf[hnd].lon_bits + csf[hnd].alt_bits + csf[hnd].scan_angle_bits +
        csf[hnd].heading_bits + 2 * csf[hnd].roll_pitch_bits + 9 * csf[hnd].range_bits;
    }


  csf[hnd].buffer_size = total_bits / 8;
  if (total_bits % 8) csf[hnd].buffer_size++;


  /*  Allocate the CSF I/O buffer memory.  */

  csf[hnd].io_buffer = (uint8_t *) malloc (csf[hnd].io_buffer_size);
  if (csf[hnd].io_buffer == NULL)
    {
      sprintf (czmil_error.info, _("Failure allocating CSF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_IO_BUFFER_ALLOCATION_ERROR);
    }


  csf[hnd].header.number_of_records = 0;

  csf[hnd].at_end = 1;
  csf[hnd].modified = 1;
  csf[hnd].created = 1;
  csf[hnd].write = 1;


  /*  Un-bias the latitude and longitude before we return.  We only wanted to use this biased internally and don't want
      to pass a modified base lat/lon back to the caller.  */

  csf_header->base_lat -= 90.0;
  csf_header->base_lon -= 180.0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_create_cif_file

 - Purpose:     Create and populate a CZMIL CIF index file using information from the
                associated CWF or CPF file(s).  This function is only called in the
                event that the original CIF file, created when the CWF/CPF files are
                created, gets deleted somehow.  Hopefully, that doesn't happen.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The handle of the CZMIL CWF or CPF file
                - path           =    The CZMIL CPF or CWF path name

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CIF_CREATE_ERROR
                - CZMIL_CWF_CREATE_CIF_ERROR
                - Error value from czmil_csf_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_create_cif_file (int32_t hnd, char *path)
{
  int32_t                    i, percent = 0, old_percent = -1, cwf_header_size, cpf_header_size, cwf_number_of_records, cpf_number_of_records, cwf_size,
                             cpf_size;
  int16_t                    cwf_buffer_size_bytes, cpf_buffer_size_bytes;
  INTERNAL_CZMIL_CIF_STRUCT  cif_struct;
  FILE                       *cwf_fp, *cpf_fp;
  char                       cwf_path[1024], cpf_path[1024], varin[8192], info[8192];
  uint8_t                    buffer[sizeof (CZMIL_CIF_Data)], cpf_read_buf[sizeof (CZMIL_CPF_Data)], cwf_read_buf[sizeof (CZMIL_CWF_Data)];
  char                       *cpf_buffer = NULL, *cwf_buffer = NULL;
  time_t                     t;
  struct tm                  *cur_tm;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Path = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Compute the CWF and CPF setvbuf buffer sizes.  According to everything I can find, multiples of 512 with an additional 8 bytes
      works best.  At the moment these are about 9MB and 8MB respectively.  */

  cwf_size = 512 * sizeof (CZMIL_CWF_Data) + 8;
  cpf_size = 512 * sizeof (CZMIL_CPF_Data) + 8;


  memset (&cif_struct, 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));

  cif_struct.created = 1;

  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &cif_struct.header.creation_timestamp);


  /*  Create the CIF file path from the input file path.  */

  strcpy (cif_struct.path, path);
  sprintf (&cif_struct.path[strlen (cif_struct.path) - 4], ".cif");


  /*  Put the version info into the header.  */

  strcpy (cif_struct.header.version, CZMIL_VERSION);


  /*  Set the record field sizes.  */

  cif_struct.header.cwf_address_bits = CIF_CWF_ADDRESS_BITS;
  cif_struct.header.cpf_address_bits = CIF_CPF_ADDRESS_BITS;
  cif_struct.header.cwf_buffer_size_bits = CIF_CWF_BUFFER_SIZE_BITS;
  cif_struct.header.cpf_buffer_size_bits = CIF_CPF_BUFFER_SIZE_BITS;
  cif_struct.header.record_size_bytes = (cif_struct.header.cwf_address_bits + cif_struct.header.cpf_address_bits + cif_struct.header.cwf_buffer_size_bits +
                                         cif_struct.header.cpf_buffer_size_bits) / 8;


  cif_struct.header.header_size = CZMIL_CIF_HEADER_SIZE;


  /*  Initialize the percent spinner or progress callback.  */

  percent = 0;
  old_percent = -1;
  if (!czmil_progress_callback) fprintf (stdout, "\n");


  /*  If it's a CWF file, make the CPF file name, or vice versa.  */

  if (!strcmp (&path[strlen (path) - 4], ".cwf"))
    {
      strcpy (cwf_path, path);
      strcpy (cpf_path, path);
      sprintf (&cpf_path[strlen (cpf_path) - 4], ".cpf");
    }
  else
    {
      strcpy (cpf_path, path);
      strcpy (cwf_path, path);
      sprintf (&cwf_path[strlen (cwf_path) - 4], ".cwf");
    }


  if ((cwf_fp = fopen64 (cwf_path, "rb")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError opening CWF file to create CIF file :\n%s\n"), cwf_path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_CREATE_CIF_ERROR);
    }


  /*  Create and set the CWF vbuf read buffer.  This will make this function run about twice as fast (on Linux, I'm not sure about Windows).  */

  cwf_buffer = (char *) malloc (cwf_size);
  if (cwf_buffer == NULL)
    {
      perror ("Allocating cwf_buffer memory in czmil_create_cif_file");
      exit (-1);
    }

  if (setvbuf (cwf_fp, cwf_buffer, _IOFBF, cwf_size))
    {
      perror ("Setting cwf vbuf size in czmil_create_cif_file");
      exit (-1);
    }


  /*  Read the tagged ASCII header data.  We only need three things from the header.  The header size, the size of the
      buffer size holder, and the number of records.  Note that we read the number of records from the CWF file.  This is 
      because, sometimes, there are more CWF records than CPF (at least for the moment).    */

  while (czmil_ngets (varin, sizeof (varin), cwf_fp))
    {
      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {
          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &cwf_header_size);
          if (strstr (varin, N_("[BUFFER SIZE BYTES]"))) sscanf (info, "%hd", &cwf_buffer_size_bytes);
          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &cwf_number_of_records);


          /*  Deal with the multi-line tagged info.  */

          if (varin[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cwf_fp))
                {
                  if (varin[0] == '}') break;
                }
            }
        }
    }


  /*  Try to create the CIF file.  */

  if ((cif_struct.fp = fopen64 (cif_struct.path, "wb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL CIF file :\n%s\n"), cif_struct.path, strerror (errno));
      free (cwf_buffer);
      fclose (cwf_fp);
      return (czmil_error.czmil = CZMIL_CIF_CREATE_ERROR);
    }


  /*  Allocate the CIF I/O buffer memory.  */

  cif_struct.io_buffer_size = CZMIL_CIF_IO_BUFFER_SIZE;
  cif_struct.io_buffer = (uint8_t *) malloc (cif_struct.io_buffer_size);
  if (cif_struct.io_buffer == NULL)
    {
      free (cwf_buffer);
      fclose (cwf_fp);

      sprintf (czmil_error.info, _("Failure allocating CIF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_IO_BUFFER_ALLOCATION_ERROR);
    }
  cif_struct.io_buffer_address = 0;


  /*  Open the CPF file.  */

  if ((cpf_fp = fopen64 (cpf_path, "rb")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError opening CPF file to create CIF file :\n%s\n"), cpf_path, strerror (errno));
      free (cwf_buffer);
      free (cif_struct.io_buffer);
      fclose (cwf_fp);
      return (czmil_error.czmil = CZMIL_CPF_CREATE_CIF_ERROR);
    }


  /*  Create and set the CPF vbuf read buffer.  This will make this function run about twice as fast (on Linux, I'm not sure about Windows).  */

  cpf_buffer = (char *) malloc (cpf_size);
  if (cpf_buffer == NULL)
    {
      perror ("Allocating cpf_buffer memory in czmil_create_cif_file");
      exit (-1);
    }

  if (setvbuf (cpf_fp, cpf_buffer, _IOFBF, cpf_size))
    {
      perror ("Setting cpf vbuf size in czmil_create_cif_file");
      exit (-1);
    }


  /*  Read the tagged ASCII header data.  We only need two things from the header.  The header size and the size of the
      buffer size holder.  */

  while (czmil_ngets (varin, sizeof (varin), cpf_fp))
    {
      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {
          /*  Put everything to the right of the equals sign into 'info'.   */

          czmil_get_string (varin, info);

          if (strstr (varin, N_("[HEADER SIZE]"))) sscanf (info, "%d", &cpf_header_size);
          if (strstr (varin, N_("[BUFFER SIZE BYTES]"))) sscanf (info, "%hd", &cpf_buffer_size_bytes);
          if (strstr (varin, N_("[NUMBER OF RECORDS]"))) sscanf (info, "%d", &cpf_number_of_records);


          /*  Deal with the multi-line tagged info.  */

          if (varin[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cpf_fp))
                {
                  if (varin[0] == '}') break;
                }
            }
        }
    }

          
  /*  This field will always be the same as the field from the CWF header (not necessarily the same as the one from the CPF file).  */

  cif_struct.header.number_of_records = cwf_number_of_records;


  /*  Write the header to the CIF file.  */

  if (czmil_write_cif_header (&cif_struct) < 0)
    {
      free (cpf_buffer);
      free (cwf_buffer);
      free (cif_struct.io_buffer);
      fclose (cpf_fp);
      fclose (cwf_fp);
      return (czmil_error.czmil);
    }


  /*  Position the CPF file to the start of the data.  */

  fseeko64 (cpf_fp, cpf_header_size, SEEK_SET);


  /*  Position the CWF file to the start of the data.  */

  fseeko64 (cwf_fp, cwf_header_size, SEEK_SET);


  /*  Loop through the CWF and CPF files and re-create the CIF file.  */

  for (i = 0 ; i < cwf_number_of_records ; i++)
    {
      /*  There may be fewer CPF records than CWF records.  In that case we want to set everything to 0 for the 
          CPF record.  */

      cif_struct.record.cpf_address = cif_struct.record.cpf_buffer_size = 0;
      if (i < cpf_number_of_records)
        {
          /*  Read the buffer size and then the rest of the buffer.  This uses vbuf buffering.  */

          cif_struct.record.cpf_address = ftello64 (cpf_fp);
          if (!fread (buffer, cpf_buffer_size_bytes, 1, cpf_fp))
            {
              sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
              free (cwf_buffer);
              free (cpf_buffer);
              free (cif_struct.io_buffer);
              fclose (cpf_fp);
              fclose (cwf_fp);
              return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
            }
          cif_struct.record.cpf_buffer_size = czmil_bit_unpack (buffer, 0, cif_struct.header.cpf_buffer_size_bits);

          if (!fread (cpf_read_buf, cif_struct.record.cpf_buffer_size - cpf_buffer_size_bytes, 1, cpf_fp))
            {
              sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
              free (cwf_buffer);
              free (cpf_buffer);
              free (cif_struct.io_buffer);
              fclose (cpf_fp);
              fclose (cwf_fp);
              return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
            }
        }


      cif_struct.record.cwf_address = ftello64 (cwf_fp);
      if (!fread (buffer, cwf_buffer_size_bytes, 1, cwf_fp))
        {
          sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
          free (cwf_buffer);
          free (cpf_buffer);
          free (cif_struct.io_buffer);
          fclose (cpf_fp);
          fclose (cwf_fp);
          return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
        }
      cif_struct.record.cwf_buffer_size = czmil_bit_unpack (buffer, 0, cif_struct.header.cwf_buffer_size_bits);

      if (!fread (cwf_read_buf, cif_struct.record.cwf_buffer_size - cwf_buffer_size_bytes, 1, cwf_fp))
        {
          sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
          free (cwf_buffer);
          free (cpf_buffer);
          free (cif_struct.io_buffer);
          fclose (cpf_fp);
          fclose (cwf_fp);
          return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
        }

      if (czmil_write_cif_record (&cif_struct) < 0)
        {
          free (cpf_buffer);
          free (cwf_buffer);
          free (cif_struct.io_buffer);
          fclose (cpf_fp);
          fclose (cwf_fp);
          return (czmil_error.czmil);
        }


      percent = NINT (((float) i / (float) cwf_number_of_records) * 100.0);
      if (percent != old_percent)
        {
          /*  If the callback has been registered, call it.  Otherwise, just print the info to stdout.  */

          if (czmil_progress_callback)
            {
              (*czmil_progress_callback) (hnd, path, percent);
            }
          else
            {
              fprintf (stdout, "Indexing the CWF and CPF files : %03d%% processed\r", percent);
              fflush (stdout);
            }
          old_percent = percent;
        }
    }


  fclose (cwf_fp);
  fclose (cpf_fp);


  /*  Flush the last buffer.  */

  if (cif_struct.io_buffer_address)
    {
      /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
          of the next available byte.  Therefor, it is also the size of the data already written into the 
          output buffer.  */

      if (!fwrite (cif_struct.io_buffer, cif_struct.io_buffer_address, 1, cif_struct.fp))
        {
          free (cpf_buffer);
          free (cwf_buffer);
          free (cif_struct.io_buffer);
          sprintf (czmil_error.info, _("File : %s\nError writing CIF data :\n%s\n"), cif_struct.path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_WRITE_ERROR);
        }
    }


  fclose (cif_struct.fp);


  /*  Make sure we get to 100%.  */

  if (czmil_progress_callback)
    {
      (*czmil_progress_callback) (hnd, path, 100);
    }
  else
    {
      fprintf (stdout, "Indexing the CWF and CPF files : 100%% processed\n");
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  free (cwf_buffer);
  free (cpf_buffer);
  free (cif_struct.io_buffer);


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_create_caf_file

 - Purpose:     Create CZMIL CAF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - path           =    Path name
                - caf_header     =    CZMIL_CAF_Header structure to be written to the file

 - NOTE:        Block buffering on creation increases speed significantly.  Creating and using
                my own output buffer seems to be marginally faster than creating the buffer and using
                setvbuf.  In addition, I know exactly what it's doing on every system (as opposed to
                setvbuf).

 - Returns:     
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CAF_CREATE_ERROR
                - Error value from czmil_caf_write_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Please look at the CZMIL_CAF_Header structure in the czmil.h file to determine which
                fields must be set by your application in the caf_header structure prior to creating
                the CAF file.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_create_caf_file (char *path, CZMIL_CAF_Header *caf_header)
{
  int32_t i, hnd, total_bits;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (caf[i].fp == NULL)
        {
          memset (&caf[i], 0, sizeof (INTERNAL_CZMIL_CAF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Save the file name for error messages.  */

  strcpy (caf[hnd].path, path);


  /*  Make sure that the file has a .caf extension since we will be using the extension to check associated files.  */

  if (strcmp (&path[strlen (path) - 4], ".caf"))
    {
      sprintf (czmil_error.info, _("File : %s\nInvalid file extension for CZMIL CAF file (must be .caf)\n"), caf[hnd].path);
      return (czmil_error.czmil = CZMIL_CAF_INVALID_FILENAME_ERROR);
    }


  /*  Open the file and write the header.  */

  if ((caf[hnd].fp = fopen64 (caf[hnd].path, "wb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError creating CZMIL CAF file :\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_CREATE_ERROR);
    }


  /*  Set the io_buffer_size.  */

  caf[hnd].io_buffer_size = CZMIL_CAF_IO_BUFFER_SIZE;


  caf[hnd].header = *caf_header;


  /*  Put the version info into the header.  */

  strcpy (caf[hnd].header.version, CZMIL_VERSION);


  /*  Set the default packed record bit field sizes and scale factors.  */

  caf[hnd].shot_id_bits = CWF_SHOT_ID_BITS;
  caf[hnd].channel_number_bits = CAF_CHANNEL_NUMBER_BITS;
  caf[hnd].optech_classification_bits = CPF_OPTECH_CLASSIFICATION_BITS;
  caf[hnd].interest_point_scale = CPF_INTEREST_POINT_SCALE;
  caf[hnd].return_max = CZMIL_MAX_RETURNS;
  caf[hnd].czmil_max_packets = CZMIL_MAX_PACKETS;


  /*  Compute the rest of the field size information from the defaults.  */

  caf[hnd].shot_id_max = (uint32_t) (power2[caf[hnd].shot_id_bits] - 1);
  caf[hnd].channel_number_max = 8;


  /*  When using czmil_short_log2 always make sure that the computed value on the right will always be less than 32768 - or else!  */

  caf[hnd].return_bits = czmil_short_log2 (caf[hnd].return_max) + 1;

  caf[hnd].interest_point_bits = (uint16_t) czmil_int_log2 (NINT ((float) (caf[hnd].czmil_max_packets * 64) * caf[hnd].interest_point_scale)) + 1;
  caf[hnd].interest_point_max = (uint32_t) (power2[caf[hnd].interest_point_bits] - 1);
  caf[hnd].optech_classification_max = (uint32_t) (power2[caf[hnd].optech_classification_bits] - 1);


  /*  Set the header size from the defaults.  */

  caf[hnd].header.header_size = CZMIL_CAF_HEADER_SIZE;


  /*  Write the CAF header (which, by the way, populates the major and minor version numbers in the header).  */

  if (czmil_write_caf_header (hnd) < 0)
    {
      fclose (caf[hnd].fp);
      caf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  total_bits = caf[hnd].shot_id_bits + caf[hnd].channel_number_bits + caf[hnd].interest_point_bits + caf[hnd].optech_classification_bits + 2 * caf[hnd].return_bits;

  caf[hnd].buffer_size = total_bits / 8;
  if (total_bits % 8) caf[hnd].buffer_size++;


  /*  Allocate the CAF I/O buffer memory.  */

  caf[hnd].io_buffer = (uint8_t *) malloc (caf[hnd].io_buffer_size);
  if (caf[hnd].io_buffer == NULL)
    {
      sprintf (czmil_error.info, _("Failure allocating CAF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_IO_BUFFER_ALLOCATION_ERROR);
    }


  caf[hnd].header.number_of_records = 0;

  caf[hnd].at_end = 1;
  caf[hnd].modified = 1;
  caf[hnd].created = 1;
  caf[hnd].write = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_open_cwf_file

 - Purpose:     Open a CZMIL CWF file (called from IDL).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/26/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - cwf_header     =    CZMIL_CWF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, CZMIL_READONLY_SEQUENTIAL,
                                      or (HydroFusion only) CZMIL_CWF_PROCESS_WAVEFORMS

 - Returns:
                - The file handle (0 or positive)
                - Error value from czmil_open_cwf_file

 - Caveats:     This is a wrapper function that can be called from IDL that calls
                czmil_open_cwf_file.  We're just null terminating the path name.
                This function should only be used by HydroFusion.

                This function will probably only be called from IDL by the HydroFusion package.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_open_cwf_file (char *idl_path, int32_t path_length, CZMIL_CWF_Header *cwf_header, int32_t mode)
{
  char path[1024];


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


  /*  Call the normal CWF file open function.  */

  return (czmil_open_cwf_file (path, cwf_header, mode));
}



/********************************************************************************************/
/*!

 - Function:    czmil_open_cwf_file

 - Purpose:     Open a CZMIL CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - path           =    The CZMIL CWF file path
                - cwf_header     =    CZMIL_CWF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, CZMIL_READONLY_SEQUENTIAL,
                                      or (HydroFusion only) CZMIL_CWF_PROCESS_WAVEFORMS

 - Returns:
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CWF_NO_CIF_ERROR
                - CZMIL_CIF_RENAME_ERROR
                - CZMIL_CWF_OPEN_UPDATE_ERROR
                - CZMIL_CWF_OPEN_READONLY_ERROR
                - Error value from czmil_create_cif_file
                - Error value from czmil_read_cwf_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_open_cwf_file (const char *path, CZMIL_CWF_Header *cwf_header, int32_t mode)
{
  int32_t i, hnd, cif_mode = -1, orig_mode = -1;
  char cpf_path[1024], cif_path[1024], new_cif_name[1024];
  CZMIL_CIF_Header cif_header;
  FILE *tmp_fp;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


#ifdef IDL_LOG

      /*  Create an error log file for IDL.  */

      if (log_fp == NULL)
        {
          strcpy (log_path, path);
          sprintf (&log_path[strlen (log_path) - 4], "_API.log");

          if ((log_fp = fopen (log_path, "w+")) == NULL) exit (-1);
        }
#endif

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cwf[i].fp == NULL)
        {
          memset (&cwf[i], 0, sizeof (INTERNAL_CZMIL_CWF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Set the CIF mode and save the original open mode (since it may be reset below).  */

  cif_mode = CZMIL_READONLY;
  orig_mode = mode;


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */


  /*  Save the file name for error messages.  */

  strcpy (cwf[hnd].path, path);


  /*  If the mode is CZMIL_CWF_PROCESS_WAVEFORMS, check for CWI (initial processing) or CIF (reprocessing or
      initial processing with older API).  Granted, this is sort of a kludge.  It could have been handled
      more elegantly in the CIF/CWI checking below but this method had the advantage of causing no code
      changes other than this simple addition.  */

  if (mode == CZMIL_CWF_PROCESS_WAVEFORMS)
    {
      /*  Set the mode to CZMIL_READONLY_SEQUENTIAL.  */

      mode = CZMIL_READONLY_SEQUENTIAL;


      /*  First check for the *.cwi file.  */

      strcpy (cif_path, path);
      sprintf (&cif_path[strlen (cif_path) - 4], ".cwi");

      if ((tmp_fp = fopen (cif_path, "rb")) == NULL)
        {
          /*  Next, check for the *.cif file.  */

          strcpy (cif_path, path);
          sprintf (&cif_path[strlen (cif_path) - 4], ".cif");

          if ((tmp_fp = fopen (cif_path, "rb")) == NULL)
            {
              sprintf (czmil_error.info, _("File : %s\nNo index file available for processing CWF waveforms.\n"), cwf[hnd].path);
              return (czmil_error.czmil = CZMIL_CWF_NO_CIF_ERROR);
            }


          fclose (tmp_fp);


          /*  Rename the CIF file to CWI so we can handle reprocessing or CIF files generated by an older version of the API.  */

          strcpy (new_cif_name, cif_path);
          sprintf (&new_cif_name[strlen (new_cif_name) - 4], ".cwi");

          if (rename (cif_path, new_cif_name))
            {
              sprintf (czmil_error.info, _("Old file : %s\nNew file : %s\nError renaming CZMIL CIF file :\n%s\n"), cif_path, new_cif_name, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_RENAME_ERROR);
            }
        }
      else
        {
          fclose (tmp_fp);
        }
    }


  /*  Open the file and read the header.  */

  switch (mode)
    {
    case CZMIL_UPDATE:
      if ((cwf[hnd].fp = fopen64 (path, "rb+")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CWF file for update :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_OPEN_UPDATE_ERROR);
        }

      break;


    case CZMIL_READONLY:

      if ((cwf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CWF file read-only :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_OPEN_READONLY_ERROR);
        }

      break;


    case CZMIL_READONLY_SEQUENTIAL:

      if ((cwf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CWF file read-only :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_OPEN_READONLY_ERROR);
        }


      /*  Compute the CWF setvbuf buffer size.  According to everything I can find, multiples of 512 with an additional 8 bytes
          works best.  At the moment this is about 9MB.  */

      cwf[hnd].io_buffer_size = 512 * sizeof (CZMIL_CWF_Data) + 8;


      /*  Allocate the CWF I/O buffer memory.  */

      cwf[hnd].io_buffer = (uint8_t *) malloc (cwf[hnd].io_buffer_size);
      if (cwf[hnd].io_buffer == NULL)
        {
          fclose (cwf[hnd].fp);
          cwf[hnd].fp = NULL;

          sprintf (czmil_error.info, _("Failure allocating CWF I/O buffer : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_IO_BUFFER_ALLOCATION_ERROR);
        }

      if (setvbuf (cwf[hnd].fp, (char *) cwf[hnd].io_buffer, _IOFBF, cwf[hnd].io_buffer_size))
        {
          fclose (cwf[hnd].fp);
          cwf[hnd].fp = NULL;

          free (cwf[hnd].io_buffer);
          cwf[hnd].io_buffer_size = 0;

          sprintf (czmil_error.info, _("Failure using setvbuf : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_SETVBUF_ERROR);
        }

      cif_mode = CZMIL_READONLY_SEQUENTIAL;

      break;
    }


  /*  Save the open mode.  */

  cwf[hnd].mode = mode;


  /*  Read the header.  */

  if (czmil_read_cwf_header (hnd))
    {
      fclose (cwf[hnd].fp);
      cwf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  *cwf_header = cwf[hnd].header;


  /*  Compute the needed fields from the header information.  */

  cwf[hnd].type_0_bytes = (cwf[hnd].type_bits + 64 * 10) / 8;
  if ((cwf[hnd].type_bits + 64 * 10) % 8) cwf[hnd].type_0_bytes++;
  cwf[hnd].type_1_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].type_1_offset = (uint16_t) (power2[cwf[hnd].type_1_start_bits] - 1);
  cwf[hnd].type_2_offset_bits = cwf[hnd].type_2_start_bits + 1;
  cwf[hnd].type_2_offset = (uint16_t) (power2[cwf[hnd].type_2_start_bits] - 1);
  cwf[hnd].type_3_offset_bits = cwf[hnd].type_1_start_bits + 1;
  cwf[hnd].type_1_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_2_header_bits = cwf[hnd].type_bits + cwf[hnd].type_1_start_bits + cwf[hnd].type_1_offset_bits + cwf[hnd].type_2_start_bits +
    cwf[hnd].type_2_offset_bits + cwf[hnd].delta_bits;
  cwf[hnd].type_3_header_bits = cwf[hnd].type_bits + cwf[hnd].type_3_offset_bits + cwf[hnd].delta_bits;


  /*  Loop through all possible open CPF files to see if we have the associated CPF opened.  If we do then the CIF file has already
      been opened and all we have to do is steal the handle from the CPF record.  */

  strcpy (cpf_path, path);
  sprintf (&cpf_path[strlen (cpf_path) - 4], ".cpf");
  cwf[hnd].cif_hnd = -1;

  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cpf[i].fp != NULL)
        {
          /*  Make sure the CPF file isn't being created.  If it is, we don't have a CIF file reference.  */

          if (!cpf[hnd].created)
            {
              if (!strcmp (cpf_path, cpf[hnd].path))
                {
                  cwf[hnd].cif_hnd = cpf[hnd].cif_hnd;
                  break;
                }
            }
        }
    }


  /*  If we didn't have the CPF file opened (after the check, above), open the associated CIF file directly.  If there
      is an associated *.cwi file then we may be creating a CPF file.  In that case, we're going to read the *.cwi
      file that was created when we created the CWF file and make a new CIF file (that starts out as *.cif.tmp).  */

  if (cwf[hnd].cif_hnd < 0)
    {
      /*  First check for the *.cwi file.  */

      strcpy (cif_path, path);
      sprintf (&cif_path[strlen (cif_path) - 4], ".cwi");


      /*  If we find the CWI (temporary CZMIL Waveform Index - *.cwi) file, we need to open it and create the new CIF file (which will be called *.cif.tmp
          for the time being).  We only want to create the new CIF file if the original mode was CZMIL_CWF_PROCESS_WAVEFORMS.  If a .cwi file exists and
	  any other open mode was used it means that HydroFusion is doing some preliminary checking of waveform records.  */

      if ((cwf[hnd].cif_hnd = czmil_open_cif_file (cif_path, &cwf[hnd].cif.header, CZMIL_READONLY_SEQUENTIAL)) >= 0)
        {
          strcpy (cwf[hnd].cif.path, path);
          sprintf (&cwf[hnd].cif.path[strlen (cwf[hnd].cif.path) - 4], ".cif.tmp");


	  /*  Only try to create the CIF file if the original open mode was CZMIL_CWF_PROCESS_WAVEFORMS.  */

	  if (orig_mode == CZMIL_CWF_PROCESS_WAVEFORMS)
	    {
	      /*  Try to create the CIF (*.tmp.cif) file.  */

	      if ((cwf[hnd].cif.fp = fopen64 (cwf[hnd].cif.path, "wb+")) == NULL)
		{
		  sprintf (czmil_error.info, _("File : %s\nError creating CZMIL TMP CIF file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
		  return (czmil_error.czmil = CZMIL_CIF_CREATE_ERROR);
		}


	      /*  Allocate the CIF I/O buffer memory.  */

	      cwf[hnd].cif.io_buffer_size = CZMIL_CIF_IO_BUFFER_SIZE;
	      cwf[hnd].cif.io_buffer = (uint8_t *) malloc (cwf[hnd].cif.io_buffer_size);
	      if (cwf[hnd].cif.io_buffer == NULL)
		{
		  fclose (cwf[hnd].fp);
		  cwf[hnd].fp = NULL;

		  sprintf (czmil_error.info, _("Failure allocating CIF I/O buffer : %s\n"), strerror (errno));
		  return (czmil_error.czmil = CZMIL_CIF_IO_BUFFER_ALLOCATION_ERROR);
		}
	      cwf[hnd].cif.io_buffer_address = 0;


	      /*  Write the header from the *.cwi file to the *.cif.tmp file.  */

	      if (czmil_write_cif_header (&cwf[hnd].cif) < 0) return (czmil_error.czmil);
	    }
        }
      else
        {
          strcpy (cif_path, path);
          sprintf (&cif_path[strlen (cif_path) - 4], ".cif");


          /*  If we can't find the CIF file, or it is corrupt, we have to regenerate it.  */

          if ((cwf[hnd].cif_hnd = czmil_open_cif_file (cif_path, &cif_header, cif_mode)) < 0)
            {
              /*  Always use the CPF file name (generated above) so that czmil_create_cif file will generate addresses for
                  both the CWF and CPF files (unless the CPF file is being created).  */

              if (!cpf[hnd].created)
                {
                  if (czmil_create_cif_file (hnd, cpf_path) < 0)
                    {
                      fclose (cwf[hnd].fp);
                      cwf[hnd].fp = NULL;

                      return (czmil_error.czmil);
                    }
                }
              else
                {
                  if (czmil_create_cif_file (hnd, cwf[hnd].path) < 0)
                    {
                      fclose (cwf[hnd].fp);
                      cwf[hnd].fp = NULL;

                      return (czmil_error.czmil);
                    }
                }


              /*  Now that we've created it we have to open it.  We don't have to check for error on opening since 
                  we just created it without returning an error.  */

              cwf[hnd].cif_hnd = czmil_open_cif_file (cif_path, &cif_header, cif_mode);
            }
        }
    }


  cwf[hnd].at_end = 0;
  cwf[hnd].modified = 0;
  cwf[hnd].created = 0;
  cwf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_open_cpf_file

 - Purpose:     Open a CZMIL CPF file (called from IDL).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/26/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - cpf_header     =    CZMIL_CPF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL

 - Returns:
                - The file handle (0 or positive)
                - Error value from czmil_open_cpf_file

 - Caveats:     This is a wrapper function that can be called from IDL that calls
                czmil_open_cpf_file.  We're just null terminating the path name.
                This function should only be used by HydroFusion.

                This function will probably only be called from IDL by the HydroFusion package.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_open_cpf_file (char *idl_path, int32_t path_length, CZMIL_CPF_Header *cpf_header, int32_t mode)
{
  char path[1024];


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


  /*  Call the normal CPF file open function.  */

  return (czmil_open_cpf_file (path, cpf_header, mode));
}



/********************************************************************************************/
/*!

 - Function:    czmil_open_cpf_file

 - Purpose:     Open a CZMIL CPF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - path           =    The CZMIL file path
                - cpf_header     =    CZMIL_CPF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL

 - Returns:
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CPF_OPEN_UPDATE_ERROR
                - CZMIL_CPF_OPEN_READONLY_ERROR
                - Error value from czmil_create_cif_file
                - Error value from czmil_read_cpf_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_open_cpf_file (const char *path, CZMIL_CPF_Header *cpf_header, int32_t mode)
{
  int32_t i, hnd, cif_mode = -1;
  char cwf_path[1024], cif_path[1024];
  CZMIL_CIF_Header cif_header;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


#ifdef IDL_LOG

      /*  Create an error log file for IDL.  */

      if (log_fp == NULL)
        {
          strcpy (log_path, path);
          sprintf (&log_path[strlen (log_path) - 4], "_API.log");

          if ((log_fp = fopen (log_path, "w+")) == NULL) exit (-1);
        }
#endif

      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cpf[i].fp == NULL)
        {
          memset (&cpf[i], 0, sizeof (INTERNAL_CZMIL_CPF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  cif_mode = CZMIL_READONLY;


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */

  /*  Save the file name for error messages.  */

  strcpy (cpf[hnd].path, path);


  /*  Open the file and read the header.  */

  switch (mode)
    {
    case CZMIL_UPDATE:
      if ((cpf[hnd].fp = fopen64 (path, "rb+")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CPF file for update :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_OPEN_UPDATE_ERROR);
        }

      break;


    case CZMIL_READONLY:
      if ((cpf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CPF file read-only :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_OPEN_READONLY_ERROR);
        }

      break;


    case CZMIL_READONLY_SEQUENTIAL:
      if ((cpf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CPF file read-only :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_OPEN_READONLY_ERROR);
        }


      /*  Compute the CPF setvbuf buffer size.  According to everything I can find, multiples of 512 with an additional 8 bytes
          works best.  At the moment this is about 8MB.  */

      cpf[hnd].io_buffer_size = 512 * sizeof (CZMIL_CPF_Data) + 8;


      /*  Allocate the CPF I/O buffer memory.  */

      cpf[hnd].io_buffer = (uint8_t *) malloc (cpf[hnd].io_buffer_size);
      if (cpf[hnd].io_buffer == NULL)
        {
          fclose (cpf[hnd].fp);
          cpf[hnd].fp = NULL;

          sprintf (czmil_error.info, _("Failure allocating CPF I/O buffer : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_IO_BUFFER_ALLOCATION_ERROR);
        }

      if (setvbuf (cpf[hnd].fp, (char *) cpf[hnd].io_buffer, _IOFBF, cpf[hnd].io_buffer_size))
        {
          fclose (cpf[hnd].fp);
          cpf[hnd].fp = NULL;

          free (cpf[hnd].io_buffer);
          cpf[hnd].io_buffer_size = 0;

          sprintf (czmil_error.info, _("Failure using setvbuf : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_SETVBUF_ERROR);
        }

      cif_mode = CZMIL_READONLY_SEQUENTIAL;

      break;
    }


  /*  Save the open mode.  */

  cpf[hnd].mode = mode;


  /*  Read the header.  */

  if (czmil_read_cpf_header (hnd))
    {
      fclose (cpf[hnd].fp);
      cpf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  *cpf_header = cpf[hnd].header;


  /*  Compute the needed fields from the header information.  */

  cpf[hnd].lat_max = (uint32_t) (power2[cpf[hnd].lat_bits] - 1);
  cpf[hnd].lat_offset = cpf[hnd].lat_max / 2;
  cpf[hnd].lon_max = (uint32_t) (power2[cpf[hnd].lon_bits] - 1);
  cpf[hnd].lon_offset = cpf[hnd].lon_max / 2;
  cpf[hnd].lat_diff_max = (uint32_t) (power2[cpf[hnd].lat_diff_bits] - 1);
  cpf[hnd].lat_diff_offset = cpf[hnd].lat_diff_max / 2;
  cpf[hnd].lon_diff_max = (uint32_t) (power2[cpf[hnd].lon_diff_bits] - 1);
  cpf[hnd].lon_diff_offset = cpf[hnd].lon_diff_max / 2;
  cpf[hnd].elev_max = (uint32_t) (power2[cpf[hnd].elev_bits] - 1);
  cpf[hnd].elev_offset = cpf[hnd].elev_max / 2;
  cpf[hnd].reflectance_max = (uint32_t) (power2[cpf[hnd].reflectance_bits] - 1);
  cpf[hnd].class_max = (uint32_t) (power2[cpf[hnd].class_bits] - 1);
  cpf[hnd].uncert_max = (uint32_t) (power2[cpf[hnd].uncert_bits] - 1);
  cpf[hnd].off_nadir_angle_max = (uint32_t) (power2[cpf[hnd].off_nadir_angle_bits] - 1);
  cpf[hnd].off_nadir_angle_offset = cpf[hnd].off_nadir_angle_max / 2;
  cpf[hnd].laser_energy_max = (uint32_t) (power2[cpf[hnd].laser_energy_bits] - 1);
  cpf[hnd].probability_max = (uint32_t) (power2[cpf[hnd].probability_bits] - 1);
  cpf[hnd].d_index_max = (uint32_t) (power2[cpf[hnd].d_index_bits] - 1);
  cpf[hnd].d_index_cube_max = (uint32_t) (power2[cpf[hnd].d_index_cube_bits] - 1);


  /*  Loop through all possible open CWF files to see if we have the associated CWF opened.  If we do
      then the CIF file has already been opened and all we have to do is steal the handle from the CWF 
      record.  */

  strcpy (cwf_path, path);
  sprintf (&cwf_path[strlen (cwf_path) - 4], ".cwf");
  cpf[hnd].cif_hnd = -1;

  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cwf[i].fp != NULL)
        {
          if (!strcmp (cwf_path, cwf[hnd].path))
            {
              cpf[hnd].cif_hnd = cwf[hnd].cif_hnd;
              break;
            }
        }
    }


  /*  If we didn't have the CWF file opened (after the check, above), open the associated CIF file directly.  */

  if (cpf[hnd].cif_hnd < 0)
    {
      strcpy (cif_path, path);
      sprintf (&cif_path[strlen (cif_path) - 4], ".cif");


      /*  If we can't find the CIF file, or it is corrupt, we have to regenerate it.  */

      if ((cpf[hnd].cif_hnd = czmil_open_cif_file (cif_path, &cif_header, cif_mode)) < 0)
        {
          if (czmil_create_cif_file (hnd, cpf[hnd].path) < 0)
            {
              fclose (cpf[hnd].fp);
              cpf[hnd].fp = NULL;

              return (czmil_error.czmil);
            }


          /*  Now that we've created it we have to open it.  We don't have to check for error on opening since 
              we just created it without returning an error.  */

          cpf[hnd].cif_hnd = czmil_open_cif_file (cif_path, &cif_header, cif_mode);
        }
    }


  cpf[hnd].at_end = 0;
  cpf[hnd].modified = 0;
  cpf[hnd].created = 0;
  cpf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_open_csf_file

 - Purpose:     Open a CZMIL CSF file (called from IDL).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/26/12

 - Arguments:
                - idl_path       =    Unterminated path name from IDL (HydroFusion)
		- path_length    =    Length of unterminated path name
                - csf_header     =    CZMIL_CSF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL

 - Returns:
                - The file handle (0 or positive)
                - Error value from czmil_open_csf_file

 - Caveats:     This is a wrapper function that can be called from IDL that calls
                czmil_open_csf_file.  We're just null terminating the path name.
                This function should only be used by HydroFusion.

                This function will probably only be called from IDL by the HydroFusion package.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_open_csf_file (char *idl_path, int32_t path_length, CZMIL_CSF_Header *csf_header, int32_t mode)
{
  char path[1024];


  /*  Null terminate the unterminated IDL path name.  */

  strncpy (path, idl_path, path_length);
  path[path_length] = 0;


  /*  Call the normal CSF file open function.  */

  return (czmil_open_csf_file (path, csf_header, mode));
}



/********************************************************************************************/
/*!

 - Function:    czmil_open_csf_file

 - Purpose:     Open a CZMIL CSF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - path           =    The CZMIL CSF file path
                - csf_header     =    CZMIL_CSF_HEADER structure to be populated
                - mode           =    CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL

 - Returns:
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CSF_OPEN_UPDATE_ERROR
                - CZMIL_CSF_OPEN_READONLY_ERROR
                - Error value from czmil_read_csf_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                DO NOT use CZMIL_READONLY_SEQUENTIAL unless you are reading the entire file
                from beginning to end in sequential order.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_open_csf_file (const char *path, CZMIL_CSF_Header *csf_header, int32_t mode)
{
  int32_t i, hnd;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++) 
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          csf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (csf[i].fp == NULL)
        {
          memset (&csf[i], 0, sizeof (INTERNAL_CZMIL_CSF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */


  /*  Save the file name for error messages.  */

  strcpy (csf[hnd].path, path);


  /*  Open the file and read the header.  */

  switch (mode)
    {
    case CZMIL_UPDATE:
      if ((csf[hnd].fp = fopen64 (path, "rb+")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CSF file for update :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_OPEN_UPDATE_ERROR);
        }
      break;


    case CZMIL_READONLY:
      if ((csf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CSF file read-only :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_OPEN_READONLY_ERROR);
        }

      break;


    case CZMIL_READONLY_SEQUENTIAL:

      if ((csf[hnd].fp = fopen64 (path, "rb")) == NULL)
        {
          sprintf (czmil_error.info, _("File : %s\nError opening CSF file read-only :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_OPEN_READONLY_ERROR);
        }


      /*  Compute the CSF setvbuf buffer size.  According to everything I can find, multiples of 512 with an additional 8 bytes
          works best.  At the moment this is about 96KB.  */

      csf[hnd].io_buffer_size = 512 * sizeof (CZMIL_CSF_Data) + 8;


      /*  Allocate the CSF I/O buffer memory.  */

      csf[hnd].io_buffer = (uint8_t *) malloc (csf[hnd].io_buffer_size);
      if (csf[hnd].io_buffer == NULL)
        {
          fclose (csf[hnd].fp);
          csf[hnd].fp = NULL;

          sprintf (czmil_error.info, _("Failure allocating CSF I/O buffer : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_IO_BUFFER_ALLOCATION_ERROR);
        }

      if (setvbuf (csf[hnd].fp, (char *) csf[hnd].io_buffer, _IOFBF, csf[hnd].io_buffer_size))
        {
          fclose (csf[hnd].fp);
          csf[hnd].fp = NULL;

          free (csf[hnd].io_buffer);
          csf[hnd].io_buffer_size = 0;

          sprintf (czmil_error.info, _("Failure using setvbuf : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_SETVBUF_ERROR);
        }

      break;
    }


  /*  Save the open mode.  */

  csf[hnd].mode = mode;


  /*  Read the header.  */

  if (czmil_read_csf_header (hnd))
    {
      fclose (csf[hnd].fp);
      csf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  *csf_header = csf[hnd].header;


  csf[hnd].at_end = 0;
  csf[hnd].modified = 0;
  csf[hnd].created = 0;
  csf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_open_cif_file

 - Purpose:     Open a CZMIL CIF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/19/12

 - Arguments:
                - path           =    The CZMIL CIF file path
                - cif_header     =    CZMIL_CIF_HEADER structure to be populated
                - mode           =    CZMIL_READONLY or CZMIL_READONLY_SEQUENTIAL

 - Returns:
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CIF_OPEN_ERROR
                - Error value from czmil_read_cif_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_open_cif_file (const char *path, CZMIL_CIF_Header *cif_header, int32_t mode)
{
  int32_t i, hnd;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (cif[i].fp == NULL)
        {
          memset (&cif[i], 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */


  /*  Save the file name for error messages.  */

  strcpy (cif[hnd].path, path);


  /*  Open the file.  */

  if ((cif[hnd].fp = fopen64 (path, "rb")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError opening CIF file :\n%s\n"), cif[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_OPEN_ERROR);
    }


  /*  Set the buffer if we are opening in CZMIL_READONLY_SEQUENTIAL mode.  */

  if (mode == CZMIL_READONLY_SEQUENTIAL)
    {
      /*  Compute the CIF setvbuf buffer size.  According to everything I can find, multiples of 512 with an additional 8 bytes
          works best.  This is overkill since the CIF record as stored on disc is not as big as the CIF structure but it's close
          enough and this way we never have to worry about the size on disc changing.  */

      cif[hnd].io_buffer_size = 512 * sizeof (CZMIL_CIF_Data) + 8;


      /*  Allocate the CIF I/O buffer memory.  */

      cif[hnd].io_buffer = (uint8_t *) malloc (cif[hnd].io_buffer_size);
      if (cif[hnd].io_buffer == NULL)
        {
          fclose (cif[hnd].fp);
          cif[hnd].fp = NULL;

          sprintf (czmil_error.info, _("Failure allocating CIF I/O buffer : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_IO_BUFFER_ALLOCATION_ERROR);
        }

      if (setvbuf (cif[hnd].fp, (char *) cif[hnd].io_buffer, _IOFBF, cif[hnd].io_buffer_size))
        {
          fclose (cif[hnd].fp);
          cif[hnd].fp = NULL;

          free (cif[hnd].io_buffer);
          cif[hnd].io_buffer_size = 0;

          sprintf (czmil_error.info, _("Failure using setvbuf : %s\n"), strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_SETVBUF_ERROR);
        }
    }


  /*  Read the header.  */

  if (czmil_read_cif_header (hnd))
    {
      fclose (cif[hnd].fp);
      cif[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  *cif_header = cif[hnd].header;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_open_caf_file

 - Purpose:     Open a CZMIL CAF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - path           =    The CZMIL CAF file path
                - caf_header     =    CZMIL_CAF_HEADER structure to be populated

 - Returns:
                - The file handle (0 or positive)
                - CZMIL_TOO_MANY_OPEN_FILES_ERROR
                - CZMIL_CAF_OPEN_ERROR
                - Error value from czmil_read_caf_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_open_caf_file (const char *path, CZMIL_CAF_Header *caf_header)
{
  int32_t i, hnd, total_bits;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Path = %s\n", __FILE__, __FUNCTION__, __LINE__, path);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  The first time through we want to initialize all of the CZMIL file pointers.  */

  if (first)
    {
      for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
        {
          cpf[i].fp = NULL;
          cwf[i].fp = NULL;
          cif[i].fp = NULL;
          caf[i].fp = NULL;
          caf[i].fp = NULL;
        }


      /*  Set up the SIGINT handler.  */

      signal (SIGINT, czmil_sigint_handler);


      /*  Set the timezone to GMT.  */

#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif


      first = 0;
    }


  /*  Find the next available handle and make sure we haven't opened too many.  Also, zero the internal record structure.  */

  hnd = CZMIL_MAX_FILES;
  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      if (caf[i].fp == NULL)
        {
          memset (&caf[i], 0, sizeof (INTERNAL_CZMIL_CAF_STRUCT));

          hnd = i;
          break;
        }
    }


  if (hnd == CZMIL_MAX_FILES)
    {
      sprintf (czmil_error.info, _("Too many CZMIL files are already open.\n"));
      return (czmil_error.czmil = CZMIL_TOO_MANY_OPEN_FILES_ERROR);
    }


  /*  Internal structs are zeroed above and on close of file so we don't have to do it here.  */


  /*  Save the file name for error messages.  */

  strcpy (caf[hnd].path, path);


  /*  Open the file and read the header.  Note that we always open this for update since the only reason to 
      read this file is to apply the audits to the CPF file.  In that case we will need to update the 
      application time in the header.  */

  if ((caf[hnd].fp = fopen64 (path, "rb+")) == NULL)
    {
      sprintf (czmil_error.info, _("File : %s\nError opening CAF file :\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_OPEN_ERROR);
    }


  /*  Compute the CAF setvbuf buffer size.  According to everything I can find, multiples of 512 with an additional 8 bytes
      works best.  At the moment this is about 96KB.  */

  caf[hnd].io_buffer_size = 512 * sizeof (CZMIL_CAF_Data) + 8;


  /*  Allocate the CAF I/O buffer memory.  */

  caf[hnd].io_buffer = (uint8_t *) malloc (caf[hnd].io_buffer_size);
  if (caf[hnd].io_buffer == NULL)
    {
      fclose (caf[hnd].fp);
      caf[hnd].fp = NULL;

      sprintf (czmil_error.info, _("Failure allocating CAF I/O buffer : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_IO_BUFFER_ALLOCATION_ERROR);
    }

  if (setvbuf (caf[hnd].fp, (char *) caf[hnd].io_buffer, _IOFBF, caf[hnd].io_buffer_size))
    {
      fclose (caf[hnd].fp);
      caf[hnd].fp = NULL;

      free (caf[hnd].io_buffer);
      caf[hnd].io_buffer_size = 0;

      sprintf (czmil_error.info, _("Failure using setvbuf : %s\n"), strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_SETVBUF_ERROR);
    }


  /*  Read the header.  */

  if (czmil_read_caf_header (hnd))
    {
      fclose (caf[hnd].fp);
      caf[hnd].fp = NULL;

      return (czmil_error.czmil);
    }


  total_bits = caf[hnd].shot_id_bits + caf[hnd].channel_number_bits + caf[hnd].interest_point_bits + caf[hnd].optech_classification_bits + 2 * caf[hnd].return_bits;

  caf[hnd].buffer_size = total_bits / 8;
  if (total_bits % 8) caf[hnd].buffer_size++;


  *caf_header = caf[hnd].header;


  caf[hnd].at_end = 0;
  caf[hnd].modified = 0;
  caf[hnd].created = 0;
  caf[hnd].write = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (hnd);
}



/********************************************************************************************/
/*!

 - Function:    czmil_flush_cwf_io_buffer

 - Purpose:     When creating a CWF file, the data is stored to an I/O buffer.  When the 
                buffer becomes close to full this function is called to flush the buffer to
                the disk.  This is also called upon closing the newly created file if there
                is still some data left in the buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/26/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_flush_cwf_io_buffer (int32_t hnd)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
      of the next available byte.  Therefor, it is also the size of the data already written into the 
      output buffer.  */

  if (!fwrite (cwf[hnd].io_buffer, cwf[hnd].io_buffer_address, 1, cwf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CWF data :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_WRITE_ERROR);
    }

  cwf[hnd].pos += cwf[hnd].io_buffer_address;
  cwf[hnd].modified = 1;
  cwf[hnd].write = 1;


  /*  Reset the address to the beginning of the buffer.  */

  cwf[hnd].io_buffer_address = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_flush_cpf_io_buffer

 - Purpose:     When creating a CPF file, the data is stored to an I/O buffer.  When the 
                buffer becomes close to full this function is called to flush the buffer to
                the disk.  This is also called upon closing the newly created file if there
                is still some data left in the buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/26/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_flush_cpf_io_buffer (int32_t hnd)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
      of the next available byte.  Therefor, it is also the size of the data already written into the 
      output buffer.  */

  if (!fwrite (cpf[hnd].io_buffer, cpf[hnd].io_buffer_address, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CPF data :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_WRITE_ERROR);
    }

  cpf[hnd].pos += cpf[hnd].io_buffer_address;
  cpf[hnd].modified = 1;
  cpf[hnd].write = 1;


  /*  Reset the address to the beginning of the buffer.  */

  cpf[hnd].io_buffer_address = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_flush_csf_io_buffer

 - Purpose:     When creating a CSF file, the data is stored to an I/O buffer.  When the 
                buffer becomes close to full this function is called to flush the buffer to
                the disk.  This is also called upon closing the newly created file if there
                is still some data left in the buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/26/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_flush_csf_io_buffer (int32_t hnd)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
      of the next available byte.  Therefor, it is also the size of the data already written into the 
      output buffer.  */

  if (!fwrite (csf[hnd].io_buffer, csf[hnd].io_buffer_address, 1, csf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CSF data :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_WRITE_ERROR);
    }

  csf[hnd].pos += csf[hnd].io_buffer_address;
  csf[hnd].modified = 1;
  csf[hnd].write = 1;


  /*  Reset the address to the beginning of the buffer.  */

  csf[hnd].io_buffer_address = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_flush_caf_io_buffer

 - Purpose:     When creating a CAF file, the data is stored to an I/O buffer.  When the 
                buffer becomes close to full this function is called to flush the buffer to
                the disk.  This is also called upon closing the newly created file if there
                is still some data left in the buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CAF_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_flush_caf_io_buffer (int32_t hnd)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
      of the next available byte.  Therefor, it is also the size of the data already written into the 
      output buffer.  */

  if (!fwrite (caf[hnd].io_buffer, caf[hnd].io_buffer_address, 1, caf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CAF data :\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_WRITE_ERROR);
    }

  caf[hnd].modified = 1;
  caf[hnd].write = 1;


  /*  Reset the address to the beginning of the buffer.  */

  caf[hnd].io_buffer_address = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_close_cwf_file

 - Purpose:     Close a CZMIL CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_create_cif_file
                - Error value from czmil_write_cwf_header
                - CZMIL_CWF_CLOSE_ERROR
                - CZMIL_CIF_CLOSE_ERROR
                - CZMIL_CIF_RENAME_ERROR
                - Error value from czmil_flush_cwf_io_buffer
                - Error value from czmil_write_cif_header

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                IMPORTANT CAVEAT : If you are creating a CPF file from a CWF file you
                ***MUST*** close the CPF file prior to closing the CWF file!!!!

*********************************************************************************************/

CZMIL_DLL int32_t czmil_close_cwf_file (int32_t hnd)
{
  time_t t;
  struct tm *cur_tm;
  uint64_t timestamp;
  char new_cif_name[1024];


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Just in case someone tries to close a file more than once... */

  if (cwf[hnd].fp == NULL) return (czmil_error.czmil = CZMIL_SUCCESS);


  /*  Get the current time for creation and/or modification times.  */

  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &timestamp);


  /*  If we created a CWF file we need to save the file size and we may have to flush the I/O buffer.  */

  if (cwf[hnd].created)
    {
      /*  Flush the buffer if needed.  */

      if (cwf[hnd].io_buffer_address != 0)
        {
          if (czmil_flush_cwf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Get the file size.  */

      cwf[hnd].header.file_size = ftello64 (cwf[hnd].fp);


      /*  If we created the CWF file we have to update the number of records in the CIF file that we also created.
          In addition, we have to close the CIF file and change the name from .cwi.tmp to .cwi.  */

      cwf[hnd].cif.header.number_of_records = cwf[hnd].header.number_of_records;


      /*  Flush the last CIF buffer.  */

      if (cwf[hnd].cif.io_buffer_address)
        {
          /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
              of the next available byte.  Therefor, it is also the size of the data already written into the 
              output buffer.  */

          if (!fwrite (cwf[hnd].cif.io_buffer, cwf[hnd].cif.io_buffer_address, 1, cwf[hnd].cif.fp))
            {
              sprintf (czmil_error.info, _("File : %s\nError writing CIF data :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_WRITE_ERROR);
            }
        }

      free (cwf[hnd].cif.io_buffer);
      cwf[hnd].cif.io_buffer_address = 0;


      /*  Write the header to the CIF file.  */

      if (czmil_write_cif_header (&cwf[hnd].cif) < 0) return (czmil_error.czmil);


      /*  Now close the file and change the name from *.cwi.tmp to *.cwi.  */

      if (fclose (cwf[hnd].cif.fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
        }


      strcpy (new_cif_name, cwf[hnd].cif.path);
      sprintf (&new_cif_name[strlen (new_cif_name) - 8], ".cwi");

      if (rename (cwf[hnd].cif.path, new_cif_name))
        {
          sprintf (czmil_error.info, _("Old file : %s\nNew file : %s\nError renaming CZMIL CIF file :\n%s\n"), cwf[hnd].cif.path, new_cif_name,
                   strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_RENAME_ERROR);
        }
    }


  /*  If the CWF file was created or we modified the header we need to update the header.  */

  if (cwf[hnd].created || cwf[hnd].modified)
    {
      /*  Save the modification time to the header.  */

      cwf[hnd].header.modification_timestamp = timestamp;


      /*  Save the creation time to the header.  */

      if (cwf[hnd].created) cwf[hnd].header.creation_timestamp = timestamp;

      if (czmil_write_cwf_header (hnd) < 0) return (czmil_error.czmil);
    }


  /*  Close the file.  */

  if (cwf[hnd].fp != NULL)
    {
      if (fclose (cwf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError fclosing CZMIL CWF file :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_CLOSE_ERROR);
        }
    }


  /*  Free the local I/O buffer.  */

  if (cwf[hnd].io_buffer_size) free (cwf[hnd].io_buffer);


  /*  Make sure we close the index file if it was opened (but only if we weren't creating the CWF file).  */

  if (!cwf[hnd].created && cwf[hnd].cif_hnd >= 0)
    {
      if (cif[cwf[hnd].cif_hnd].fp != NULL)
        {
          /*  Free the local CIF I/O read buffer.  */

          if (cif[cwf[hnd].cif_hnd].io_buffer_size) free (cif[cwf[hnd].cif_hnd].io_buffer);


          if (fclose (cif[cwf[hnd].cif_hnd].fp))
            {
              sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF file :\n%s\n"), cif[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
            }
        }
    }


  /*  Clear the internal CIF structure.  */

  memset (&cif[cwf[hnd].cif_hnd], 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));


  /*  We do this just on the off chance that someday NULL won't be 0.  */

  cif[cwf[hnd].cif_hnd].fp = NULL;


  /*  Set the file pointer to NULL so we can reuse the structure the next create/open.  */

  cwf[hnd].fp = NULL;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_close_cpf_file

 - Purpose:     Close a CZMIL CPF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_create_cif_file
                - Error value from czmil_write_cpf_header
                - CZMIL_CPF_CLOSE_ERROR
                - CZMIL_CIF_CLOSE_ERROR
                - CZMIL_CIF_RENAME_ERROR
                - CZMIL_CIF_REMOVE_ERROR
                - Error value from czmil_flush_cpf_io_buffer

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                IMPORTANT CAVEAT : If you are creating a CPF file from a CWF file you
                ***MUST*** close the CPF file prior to closing the CWF file!!!!

*********************************************************************************************/

CZMIL_DLL int32_t czmil_close_cpf_file (int32_t hnd)
{
  time_t t;
  struct tm *cur_tm;
  uint64_t timestamp;
  char new_cif_name[1024];


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Just in case someone tries to close a file more than once... */

  if (cpf[hnd].fp == NULL) return (czmil_error.czmil = CZMIL_SUCCESS);


  /*  Get the current time for creation and/or modification times.  */

  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &timestamp);


  /*  If we created a CPF file we need to save the file size and we may have to flush the I/O buffer.  */

  if (cpf[hnd].created)
    {
      /*  Flush the buffer if needed.  */

      if (cpf[hnd].io_buffer_address != 0)
        {
          if (czmil_flush_cpf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Get the file size.  */

      cpf[hnd].header.file_size = ftello64 (cpf[hnd].fp);


      /*  If we created the CPF file we have to update the number of records in the CIF file that we also created.
          In addition, we have to close the CIF file and change the name from .cif.tmp to .cif.

          This is a little weird.  The CWF file may have more records than the CPF file.  In that case we don't really care about 
          the CWF records that were not turned into CPF data so we are going to set the number of records in the CIF header to the
          number of records in the CPF file not the number of records in the CWF file.  */

      cwf[hnd].cif.header.number_of_records = cpf[hnd].header.number_of_records;


      /*  Flush the last CIF buffer.  */

      if (cwf[hnd].cif.io_buffer_address)
        {
          /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
              of the next available byte.  Therefor, it is also the size of the data already written into the 
              output buffer.  */

          if (!fwrite (cwf[hnd].cif.io_buffer, cwf[hnd].cif.io_buffer_address, 1, cwf[hnd].cif.fp))
            {
              sprintf (czmil_error.info, _("File : %s\nError writing CIF data :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_WRITE_ERROR);
            }
        }

      free (cwf[hnd].cif.io_buffer);
      cwf[hnd].cif.io_buffer_address = 0;


      /*  Write the header to the CIF file.  */

      if (czmil_write_cif_header (&cwf[hnd].cif) < 0) return (czmil_error.czmil);


      /*  We also need to close the new CIF file (*.cif.tmp), rename it to *.cif, close the interim *.cwi file, and
          then delete it.  */

      if (fclose (cwf[hnd].cif.fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF TMP file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
        }


      strcpy (new_cif_name, cwf[hnd].cif.path);
      sprintf (&new_cif_name[strlen (new_cif_name) - 8], ".cif");

      if (rename (cwf[hnd].cif.path, new_cif_name))
        {
          sprintf (czmil_error.info, _("Old file : %s\nNew file : %s\nError renaming CZMIL CIF file :\n%s\n"), cwf[hnd].cif.path, new_cif_name, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_RENAME_ERROR);
        }


      /*  Make sure we close the index file if it was opened.  This may have already been closed in czmil_close_cwf_file but we need to make sure
          so we can delete it.  */

      if (cwf[hnd].cif_hnd >= 0)
        {
          if (cif[cwf[hnd].cif_hnd].fp != NULL)
            {
              /*  Free the local CIF I/O read buffer.  */

              if (cif[hnd].io_buffer_size) free (cif[hnd].io_buffer);


              if (fclose (cif[cwf[hnd].cif_hnd].fp))
                {
                  sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF file :\n%s\n"), cif[hnd].path, strerror (errno));
                  return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
                }

              cif[cwf[hnd].cif_hnd].fp = NULL;
            }
        }


      sprintf (&new_cif_name[strlen (new_cif_name) - 4], ".cwi");

      if (remove (new_cif_name))
        {
          sprintf (czmil_error.info, _("File : %s\nError removing interim CIF file :\n%s\n"), new_cif_name, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_REMOVE_ERROR);
        }
    }


  /*  If the CPF file was created or modified we need to update the header.  */

  if (cpf[hnd].created || cpf[hnd].modified)
    {
      /*  Save the modification time to the header.  */

      cpf[hnd].header.modification_timestamp = timestamp;


      /*  Save the creation time to the header.  */

      if (cpf[hnd].created) cpf[hnd].header.creation_timestamp = timestamp;


      if (czmil_write_cpf_header (hnd) < 0) return (czmil_error.czmil);
    }


  /*  Close the file.  */

  if (cpf[hnd].fp != NULL)
    {
      if (fclose (cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CPF file :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_CLOSE_ERROR);
        }
    }


  /*  Free the local I/O buffer.  */

  if (cpf[hnd].io_buffer_size) free (cpf[hnd].io_buffer);


  /*  Make sure we close the index file if it was opened (but only if we weren't creating the CPF file).  */

  if (!cpf[hnd].created && cpf[hnd].cif_hnd >= 0)
    {
      if (cif[cpf[hnd].cif_hnd].fp != NULL)
        {
          /*  Free the local CIF I/O read buffer.  */

          if (cif[cpf[hnd].cif_hnd].io_buffer_size) free (cif[cpf[hnd].cif_hnd].io_buffer);


          if (fclose (cif[cpf[hnd].cif_hnd].fp))
            {
              sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF file :\n%s\n"), cif[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
            }
        }
    }


  /*  Clear the internal CIF structure.  */

  memset (&cif[cpf[hnd].cif_hnd], 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));


  /*  We do this just on the off chance that someday NULL won't be 0.  */

  cif[cpf[hnd].cif_hnd].fp = NULL;


  /*  Set the file pointer to NULL so we can reuse the structure on the next create/open.  */

  cpf[hnd].fp = NULL;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_abort_cpf_file

 - Purpose:     Closes/deletes a CZMIL CPF file when an error is encountered during
                HydroFusion processing of the CWF file to create the CPF.  It will also close
                and delete the interim .cif.tmp file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        11/10/14

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_CLOSE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                IMPORTANT CAVEAT : This function should only be used by HydroFusion when
                an unexpected error condition is encountered during processing of the CWF
                file to create the CPF file.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_abort_cpf_file (int32_t hnd)
{

#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  If we aren't trying to create a CPF file then this call is not to be used.  */

  if (!cpf[hnd].created)
    {
      sprintf (czmil_error.info, _("File : %s\nCannot use czmil_abort_cpf_file unless you are creating a new CPF file."), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_ABORT_ERROR);
    }


  /*  Close the CPF file.  */

  if (cpf[hnd].fp != NULL)
    {
      if (fclose (cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CPF file :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_CLOSE_ERROR);
        }
    }


  /*  Delete the CPF file.  */

  if (remove (cpf[hnd].path))
    {
      sprintf (czmil_error.info, _("File : %s\nError removing CPF file :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_REMOVE_ERROR);
    }


  /*  We also need to close the interim CIF file (*.cif.tmp) and then delete it.  */

  if (fclose (cwf[hnd].cif.fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF TMP file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
    }


  if (remove (cwf[hnd].cif.path))
    {
      sprintf (czmil_error.info, _("File : %s\nError removing interim CIF file :\n%s\n"), cwf[hnd].cif.path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CIF_REMOVE_ERROR);
    }


  /*  Free the local CPF I/O buffer if it was allocated.  */

  if (cpf[hnd].io_buffer_size) free (cpf[hnd].io_buffer);


  /*  Make sure we close the CWI index file if it was opened.  */

  if (cwf[hnd].cif_hnd >= 0)
    {
      if (cif[cwf[hnd].cif_hnd].fp != NULL)
        {
          /*  Free the local CIF I/O read buffer.  */

          if (cif[hnd].io_buffer_size) free (cif[hnd].io_buffer);


          if (fclose (cif[cwf[hnd].cif_hnd].fp))
            {
              sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CIF file :\n%s\n"), cif[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_CLOSE_ERROR);
            }

          cif[cwf[hnd].cif_hnd].fp = NULL;
        }
    }


  /*  Clear the internal CIF structure.  */

  memset (&cif[cpf[hnd].cif_hnd], 0, sizeof (INTERNAL_CZMIL_CIF_STRUCT));


  /*  We do this just on the off chance that someday NULL won't be 0.  */

  cif[cpf[hnd].cif_hnd].fp = NULL;


  /*  Set the file pointer to NULL so we can reuse the structure on the next create/open.  */

  cpf[hnd].fp = NULL;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_close_csf_file

 - Purpose:     Close a CZMIL CSF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/14/12

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_write_csf_header
                - CZMIL_CSF_CLOSE_ERROR
                - Error value from czmil_flush_csf_io_buffer

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_close_csf_file (int32_t hnd)
{
  time_t t;
  struct tm *cur_tm;
  uint64_t timestamp;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Just in case someone tries to close a file more than once... */

  if (csf[hnd].fp == NULL) return (czmil_error.czmil = CZMIL_SUCCESS);


  /*  Get the current time for creation and/or modification times.  */

  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &timestamp);


  /*  If we created a CSF file we need to save the file size and we may have to flush the I/O buffer.  */

  if (csf[hnd].created)
    {
      /*  Flush the buffer if needed.  */

      if (csf[hnd].io_buffer_address != 0)
        {
          if (czmil_flush_csf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Get the file size.  */

      csf[hnd].header.file_size = ftello64 (csf[hnd].fp);
    }


  /*  If we created a CSF file we have to save the creation time.  */

  if (csf[hnd].created)
    {
      /*  Save the creation time to the header.  */

      csf[hnd].header.creation_timestamp = timestamp;
    }


  /*  If the CSF file was created or we modified the header we need to update the header.  */

  if (csf[hnd].created || csf[hnd].modified)
    {
      /*  Save the modification time to the header.  */

      csf[hnd].header.modification_timestamp = timestamp;

      if (czmil_write_csf_header (hnd) < 0) return (czmil_error.czmil);
    }


  /*  Close the file.  */

  if (csf[hnd].fp != NULL)
    {
      if (fclose (csf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CSF file :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_CLOSE_ERROR);
        }
    }


  /*  Free the local I/O buffer.  */

  if (csf[hnd].io_buffer_size) free (csf[hnd].io_buffer);


  /*  Set the file pointer to NULL so we can reuse the structure on the next create/open.  */

  csf[hnd].fp = NULL;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_close_caf_file

 - Purpose:     Close a CZMIL CAF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The file handle

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_write_caf_header
                - CZMIL_CAF_CLOSE_ERROR
                - Error value from czmil_flush_caf_io_buffer

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_close_caf_file (int32_t hnd)
{
  time_t t;
  struct tm *cur_tm;
  uint64_t timestamp;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Just in case someone tries to close a file more than once... */

  if (caf[hnd].fp == NULL) return (czmil_error.czmil = CZMIL_SUCCESS);


  /*  Get the current time for creation time.  */

  t = time (&t);
  cur_tm = gmtime (&t);
  czmil_inv_cvtime (cur_tm->tm_year, cur_tm->tm_yday + 1, cur_tm->tm_hour, cur_tm->tm_min, cur_tm->tm_sec, &timestamp);


  /*  If we created a CAF file we may have to flush the I/O buffer and/or save the creation time.  */

  if (caf[hnd].created)
    {
      /*  Flush the buffer if needed.  */

      if (caf[hnd].io_buffer_address != 0)
        {
          if (czmil_flush_caf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Save the creation time to the header.  */

      caf[hnd].header.creation_timestamp = timestamp;
    }


  /*  If we didn't create it we must have applied the audits to the CPF file (there's no other reason for reading this file)
      so we need to set the application time.  */

  else
    {
      /*  Save the application time to the header.  */

      caf[hnd].header.application_timestamp = timestamp;
    }


  /*  No matter what we did we now need to update the header.  */

  if (czmil_write_caf_header (hnd) < 0) return (czmil_error.czmil);


  /*  Close the file.  */

  if (caf[hnd].fp != NULL)
    {
      if (fclose (caf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError closing CZMIL CAF file :\n%s\n"), caf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CAF_CLOSE_ERROR);
        }
    }


  /*  Free the local I/O buffer.  */

  if (caf[hnd].io_buffer_size) free (caf[hnd].io_buffer);


  /*  Set the file pointer to NULL so we can reuse the structure on the next create/open.  */

  caf[hnd].fp = NULL;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_compress_cwf_record

 - Purpose:     Compress and bit pack a CZMIL CWF waveform record into an unsigned byte buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/02/11

 - Arguments:
                - hnd            =    The file handle
                - record         =    CZMIL CWF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR
                - Error values returned from czmil_flush_cwf_io_buffer
                - Error values returned from czmil_write_cif_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the compress and uncompress 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CWF:3])to the beginning of each section so that you can search
                from the compress to uncompress or vice versa.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_compress_cwf_record (int32_t hnd, CZMIL_CWF_Data *record)
{
  uint16_t start[4], delta_bits[4], size[4], buffer_size = 0;
  int16_t i, j, k, m, min_delta[4], max_value[4], previous, delta[63], delta2[62], delta3[64], type, offset[4];
  uint16_t *packet = NULL, *shallow_central = NULL;
  int32_t bpos, num_bits, i32value;
  uint32_t ui32value;
  uint8_t *buffer;


  /*  If there is ANY chance at all that we might overrun our I/O buffer on this write, we need to 
      flush the buffer.  Since we're compressing our records they should not be anywhere near the
      size of a CZMIL_CWF_Data structure.  But, also since we're compressing the records, the actual size
      of the next record is unknown so we are just making sure that we're not within sizeof (CZMIL_CWF_Data)
      bytes of the end of the output buffer.  That way we're taking no chances of exceeding our output
      buffer size.  */

  if ((cwf[hnd].io_buffer_size - cwf[hnd].io_buffer_address) < sizeof (CZMIL_CWF_Data))
    {
      if (czmil_flush_cwf_io_buffer (hnd) < 0) return (czmil_error.czmil);
    }


  /*  Set the buffer pointer to the correct location within the I/O buffer.  */

  buffer = &cwf[hnd].io_buffer[cwf[hnd].io_buffer_address];


  /*  [CWF:0]  We need to skip the buffer size at the beginning of the buffer.  The buffer size will be stored in 
      cwf[hnd].buffer_size_bytes bytes.  We multiply by 8 to get the number of bits to skip.  */

  bpos = cwf[hnd].buffer_size_bytes * 8;


  /*  [CWF:1]  Loop through each of the 9 channels and compress all of the available 64 sample packets.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  [CWF 1-0]  First pack the number of packets for this channel into the buffer.  */

      if (record->number_of_packets[i] > cwf[hnd].czmil_max_packets)
        {
          sprintf (czmil_error.info, N_("In CWF file %s, Record %d :\nCWF number of packets %d exceeds max packets %d.\n"), cwf[hnd].path,
                   cwf[hnd].header.number_of_records, record->number_of_packets[i], cwf[hnd].czmil_max_packets);
          return (czmil_error.czmil = CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR);
        }
      czmil_bit_pack (buffer, bpos, cwf[hnd].num_packets_bits, record->number_of_packets[i]);
      bpos += cwf[hnd].num_packets_bits;


      /*  [CWF:1-1]  Next, pack the channel packet numbers into the buffer.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {
          if (record->channel_ndx[i][j] > cwf[hnd].packet_number_max)
            {
              sprintf (czmil_error.info, N_("In CWF file %s, Record %d :\nCWF packet number %d exceeds max packet number %d.\n"), cwf[hnd].path,
                       cwf[hnd].header.number_of_records, record->channel_ndx[i][j], cwf[hnd].packet_number_max);
              return (czmil_error.czmil = CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cwf[hnd].packet_number_bits, record->channel_ndx[i][j]);
          bpos += cwf[hnd].packet_number_bits;
        }


      /*  [CWF:1-2]  Then, pack the MCWP ranges into the buffer.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {
          ui32value = NINT (record->range[i][j] * cwf[hnd].range_scale);


          /*  This shouldn't happen but, apparently, some of the ranges being read from the original .cwf file (Optech's
              version) have wild values when there is no return found.  Set them to cwf[hnd].range_max and return -1.0 on read.  */

          if (ui32value > cwf[hnd].range_max) ui32value = cwf[hnd].range_max;
          czmil_bit_pack (buffer, bpos, cwf[hnd].range_bits, ui32value);
          bpos += cwf[hnd].range_bits;
        }


      /*  Now loop through all of the possible packets and compress them.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {

          /*  Set the packet pointer to point to the correct channel and packet.  */

          packet = &record->channel[i][j * 64];


          /*  This is the uncompressed (i.e. 10 bit, bit packed values) buffer size for comparison.  */

          size[0] = cwf[hnd].type_0_bytes;


          /************** First difference computations ******************/


          /*  Compute the first differences and figure out how big the buffer will be for compression type 1.  
              The start value is the first value in the packet.  */

          start[1] = previous = packet[0];
          min_delta[1] = 32767;


          /*  Compute the 63 possible difference values and determine the minimum value.  */

          for (k = 1 ; k < 64 ; k++)
            {
              m = k - 1;
              delta[m] = packet[k] - previous;
              min_delta[1] = MIN (delta[m], min_delta[1]);
              previous = packet[k];
            }


          /*  The offset to be applied to the differences is the minimum difference. */

          offset[1] = -min_delta[1];


          /*  This may seem a bit odd since it would seem that we need to find the maximum value in a string of
              deltas but, what we're actually trying to do is find the maximum number of bits needed to pack the
              maximum value.  Normally we would spin through the points computing maximum.  Unfortunately that
              requires a branch, and branches are slow.  A faster way to find the max number of bits is to do a
              bitwise OR of all the values.  This will give us the largest value that can be stored in the number
              of bits that would be needed to store the actual max value.  It may make your head hurt (it sure
              does mine ;-) but, if you think about it for a while, you can see how it works.  This was suggested
              by a note from Preston Bannister on Daniel Lemire's blog (http://lemire.me/blog/).  */

          max_value[1] = 0;
          for (k = 0 ; k < 63 ; k++) max_value[1] = max_value[1] | (delta[k] + offset[1]);


          /*  Compute the number of bits needed to store values based on the maximum value.  */

          delta_bits[1] = czmil_short_log2 (max_value[1]) + 1;


          /*  Compute the first difference packed size.  */

          num_bits = cwf[hnd].type_1_header_bits + 63 * delta_bits[1];
          size[1] = num_bits / 8;
          if (num_bits % 8) size[1]++;


          /*  Add the first difference offset to the first difference delta values.  This either has to be done here
              or on output to the buffer.  We do it here so that the second difference computation gets to work
              against all positive values.  */

          for (k = 0 ; k < 63 ; k++) delta[k] += offset[1];


          /************** Second difference computations *****************/


          /*  Compute the second differences and figure out how big the buffer will be for compression type 2.
              The start value is the first of the first difference values.  */

          start[2] = previous = delta[0];
          min_delta[2] = 32767;


          /*  Compute the 62 possible difference values and determine the minimum value.  */

          for (k = 1 ; k < 63 ; k++)
            {
              m = k - 1;
              delta2[m] = delta[k] - previous;
              min_delta[2] = MIN (delta2[m], min_delta[2]);
              previous = delta[k];
            }


          /*  The offset is the min difference. */

          offset[2] = -min_delta[2];


          /*  Compute the maximum (bit) value in the array of deltas.  See comments in first difference computations above
              for explanation of the following two computations (do a reverse search for Preston Bannister).  */

          max_value[2] = 0;
          for (k = 0 ; k < 62 ; k++) max_value[2] = max_value[2] | (delta2[k] + offset[2]);


          /*  Compute the number of bits needed to store values based on the maximum value.  */

          delta_bits[2] = czmil_short_log2 (max_value[2]) + 1;


          /*  Compute the second difference packed size.  */

          num_bits = cwf[hnd].type_2_header_bits + 62 * delta_bits[2];
          size[2] = num_bits / 8;
          if (num_bits % 8) size[2]++;


          /*  Figure out which one saves the most space.  */

          if (size[0] <= size[1] && size[0] <= size[2])
            {
              type = 0;
            }
          else if (size[1] < size[0] && size[1] <= size[2])
            {
              type = 1;
            }
          else
            {
              type = 2;
            }


          /********** Shallow channel difference computations ************/


          /*  If we're checking shallow channels other than the central channel (in other words, channel[1] through
              channel[6]) we need to see if we can save space by storing these as differences from the central
              shallow channel (channel[CZMIL_SHALLOW_CHANNEL_1]) instead of "along the waveform" differences.  */

          size[3] = 999;
          if (i > 0 && i < 7)
            {
              /*  Set the pointer to the correct packet in the shallow central channel (channel[CZMIL_SHALLOW_CHANNEL_1]).  */

              shallow_central = &record->channel[CZMIL_SHALLOW_CHANNEL_1][j * 64];


              /*  Compute the 64 possible difference values and the minimum difference.  */

              min_delta[3] = 32767;
              for (k = 0 ; k < 64 ; k++)
                {
                  delta3[k] = packet[k] - shallow_central[k];
                  min_delta[3] = MIN (delta3[k], min_delta[3]);
                }


              /*  The offset is the min difference. */

              offset[3] = -min_delta[3];


              /*  Compute the maximum (bit) value in the array of deltas.  See comments in first difference computations above
                  for explanation of the following two computations (do a reverse search for Daniel Lemire).  */

              max_value[3] = 0;
              for (k = 0 ; k < 64 ; k++) max_value[3] = max_value[3] | (delta3[k] + offset[3]);


              /*  Compute the number of bits needed to store values based on the maximum value.  */

              delta_bits[3] = czmil_short_log2 (max_value[3]) + 1;


              /*  Compute the shallow channel difference packed size.  */

              num_bits = cwf[hnd].type_3_header_bits + 64 * delta_bits[3];
              size[3] = num_bits / 8;
              if (num_bits % 8) size[3]++;


              /*  Check to see if this will make a smaller buffer than the previously chosen compression type.  */

              if (size[3] < size[type]) type = 3;
            }


          /*  [CWF:2]  This is the compression type (see below for value definitions).  */

          czmil_bit_pack (buffer, bpos, cwf[hnd].type_bits, type);
          bpos += cwf[hnd].type_bits;


          /*  [CWF:3]  Pack the buffer.  */

          switch (type)
            {
              /*  Compression type 1.  Bit packed first differences.  Approximately 72%.  */

            case CZMIL_FIRST_DIFFERENCE:

              czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_start_bits, start[type]);
              bpos += cwf[hnd].type_1_start_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_offset_bits, offset[type] + cwf[hnd].type_1_offset);
              bpos += cwf[hnd].type_1_offset_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].delta_bits, delta_bits[type]);
              bpos += cwf[hnd].delta_bits;

              for (k = 0 ; k < 63 ; k++)
                {
                  czmil_bit_pack (buffer, bpos, delta_bits[type], delta[k]);
                  bpos += delta_bits[type];
                }

              break;


          /*  Compression type 3.  Bit packed differences between the central shallow channel and one of the
              surrounding shallow channels.  Approximately 19%.  */

            case CZMIL_SHALLOW_CENTRAL_DIFFERENCE:

              czmil_bit_pack (buffer, bpos, cwf[hnd].type_3_offset_bits, offset[type] + cwf[hnd].type_1_offset);
              bpos += cwf[hnd].type_3_offset_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].delta_bits, delta_bits[type]);
              bpos += cwf[hnd].delta_bits;

              for (k = 0 ; k < 64 ; k++)
                {
                  czmil_bit_pack (buffer, bpos, delta_bits[type], delta3[k] + offset[type]);
                  bpos += delta_bits[type];
                }

              break;



          /*  Compression type 2.  Bit packed second differences.  Approximately 8%.  */

            case CZMIL_SECOND_DIFFERENCE:

              czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_start_bits, start[1]);
              bpos += cwf[hnd].type_1_start_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].type_2_start_bits, start[type]);
              bpos += cwf[hnd].type_2_start_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_offset_bits, offset[1] + cwf[hnd].type_1_offset);
              bpos += cwf[hnd].type_1_offset_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].type_2_offset_bits, offset[type] + cwf[hnd].type_2_offset);
              bpos += cwf[hnd].type_2_offset_bits;
              czmil_bit_pack (buffer, bpos, cwf[hnd].delta_bits, delta_bits[type]);
              bpos += cwf[hnd].delta_bits;

              for (k = 0 ; k < 62 ; k++)
                {
                  czmil_bit_pack (buffer, bpos, delta_bits[type], delta2[k] + offset[type]);
                  bpos += delta_bits[type];
                }

              break;


          /*  Compression type 0.  Just 10 bit, bit-packed.  Usually less than 1%.  */

            case CZMIL_BIT_PACKED:

              for (k = 0 ; k < 64 ; k++)
                {
                  czmil_bit_pack (buffer, bpos, 10, packet[k]);
                  bpos += 10;
                }

              break;
            }
        }
    }


  /*  Use first difference compression for the T0 waveform.  It's only one 64 sample packet so if we don't get the
      best compression it really won't matter all that much.  */


  /*  Set the packet pointer to point to the T0 array.  */

  packet = record->T0;


  /*  Compute the first differences and figure out how big the buffer will be.  The start value is the first value in the packet.  */

  start[1] = previous = packet[0];
  min_delta[1] = 32767;


  /*  Compute the 63 possible difference values and determine the minimum value.  */

  for (k = 1 ; k < 64 ; k++)
    {
      m = k - 1;
      delta[m] = packet[k] - previous;
      min_delta[1] = MIN (delta[m], min_delta[1]);
      previous = packet[k];
    }


  /*  The offset to be applied to the differences is the minimum difference. */

  offset[1] = -min_delta[1];


  /*  Compute the maximum (bit) value in the array of deltas.  See comments in first difference computations above
      for explanation of the following two computations (do a reverse search for http).  */

  max_value[1] = 0;
  for (k = 0 ; k < 63 ; k++) max_value[1] = max_value[1] | (delta[k] + offset[1]);


  /*  Compute the number of bits needed to store values based on the maximum value.  */

  delta_bits[1] = czmil_short_log2 (max_value[1]) + 1;


  /*  Compute the first difference packed size.  */

  num_bits = cwf[hnd].type_1_header_bits + 63 * delta_bits[1];
  size[1] = num_bits / 8;
  if (num_bits % 8) size[1]++;


  /*  Add the first difference offset to the first difference delta values.  */

  for (k = 0 ; k < 63 ; k++) delta[k] += offset[1];


  /*  [CWF:4]  Now bit pack the data.  Note that we don't need the packing type since we're always using first difference
      (if you think that those three bits don't matter just do the math on 10,000 T0 packets per second * 3 bits).  */

  czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_start_bits, start[1]);
  bpos += cwf[hnd].type_1_start_bits;
  czmil_bit_pack (buffer, bpos, cwf[hnd].type_1_offset_bits, offset[1] + cwf[hnd].type_1_offset);
  bpos += cwf[hnd].type_1_offset_bits;
  czmil_bit_pack (buffer, bpos, cwf[hnd].delta_bits, delta_bits[1]);
  bpos += cwf[hnd].delta_bits;

  for (k = 0 ; k < 63 ; k++)
    {
      czmil_bit_pack (buffer, bpos, delta_bits[1], delta[k]);
      bpos += delta_bits[1];
    }


  /*  [CWF:5]  Shot ID.  */

  if (record->shot_id > cwf[hnd].shot_id_max)
    {
      sprintf (czmil_error.info, _("In CWF file %s, Record %d :\nCWF shot ID %d exceeds max shot ID %d."), cwf[hnd].path,
               cwf[hnd].header.number_of_records, record->shot_id, cwf[hnd].shot_id_max);
      return (czmil_error.czmil = CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cwf[hnd].shot_id_bits, record->shot_id);
  bpos += cwf[hnd].shot_id_bits;


  /*  [CWF:6]  Timestamp.  */

  if (record->timestamp - cwf[hnd].header.flight_start_timestamp > cwf[hnd].time_max)
    {
      sprintf (czmil_error.info, 
               _("In CWF file %s, Record %d :\nCWF timestamp %"PRIu64" out of range from start timestamp %"PRIu64"."),
               cwf[hnd].path, cwf[hnd].header.number_of_records, record->timestamp, cwf[hnd].header.flight_start_timestamp);
      return (czmil_error.czmil = CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR);
    }

  ui32value = (uint32_t) (record->timestamp - cwf[hnd].header.flight_start_timestamp);
  czmil_bit_pack (buffer, bpos, cwf[hnd].time_bits, ui32value);
  bpos += cwf[hnd].time_bits;


  /*  [CWF:7]  Adjust the scan angle for negatives and greater than 360.0 values.  */

  if (record->scan_angle < 0.0) record->scan_angle += 360.0;
  if (record->scan_angle > 360.0) record->scan_angle -= 360.0;

  i32value = NINT (record->scan_angle * cwf[hnd].angle_scale);
  czmil_bit_pack (buffer, bpos, cwf[hnd].scan_angle_bits, i32value);
  bpos += cwf[hnd].scan_angle_bits;


  /****************************************** VERSION CHECK ******************************************

      This field did not exist prior to major version 2.

  ***************************************************************************************************/

  /*  [CWF:8]  Waveform validity reason.  */

  if (cwf[hnd].major_version >= 2)
    {
      /*  Loop through each of the 9 channels and store the waveform validity reason.  */

      for (i = 0 ; i < 9 ; i++)
        {
          if (record->validity_reason[i] > cwf[hnd].validity_reason_max)
            {
              sprintf (czmil_error.info, _("In CWF file %s, Record %d :\nChannel %d waveform validity reason value %d too large."), cwf[hnd].path,
                       cwf[hnd].header.number_of_records, i, record->validity_reason[i]);
              return (czmil_error.czmil = CZMIL_CWF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cwf[hnd].validity_reason_bits, record->validity_reason[i]);
          bpos += cwf[hnd].validity_reason_bits;
        }
    }


  /*  [CWF:0]  Compute the buffer size and pack it in.  */

  buffer_size = bpos / 8;
  if (bpos % 8) buffer_size++;


  czmil_bit_pack (buffer, 0, cwf[hnd].buffer_size_bytes * 8, buffer_size);


  /*  Now update the I/O buffer address so we can be ready for the next record.  */

  cwf[hnd].io_buffer_address += buffer_size;


  /*  Also, update the apparent file position and use it to write the next CIF record to the CIF file.  */

  cwf[hnd].cif.record.cwf_address = cwf[hnd].create_file_pos;
  cwf[hnd].cif.record.cpf_address = 0;
  cwf[hnd].cif.record.cwf_buffer_size = buffer_size;
  cwf[hnd].cif.record.cpf_buffer_size = 0;
  cwf[hnd].create_file_pos += buffer_size;

  if (czmil_write_cif_record (&cwf[hnd].cif) < 0) return (czmil_error.czmil);


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_uncompress_cwf_record

 - Purpose:     Uncompress and bit unpack a CZMIL CWF waveform record from an unsigned byte buffer.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/02/11

 - Arguments:
                - hnd            =    The file handle
                - record         =    CZMIL CWF record
                - buffer         =    Unsigned byte buffer from which to uncompress/unpack data

 - Returns:
                - CZMIL_SUCCESS

 - Caveats:     Keeping track of what got packed where between the compress and uncompress 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CWF:3])to the beginning of each section so that you can search
                from the compress to uncompress or vice versa.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_uncompress_cwf_record (int32_t hnd, CZMIL_CWF_Data *record, uint8_t *buffer)
{
  int16_t i, j, k, start, offset, delta_bits, previous, delta[64];
  int32_t bpos, i32value;
  uint32_t ui32value;
  uint16_t type, *packet;
  int16_t start2, offset2, delta2[62];
  uint16_t *shallow_central;


  /*  Zero the entire record so that empty packets will be initialized.  */

  memset (record, 0, sizeof (CZMIL_CWF_Data));


  /*  [CWF:0]  We need to skip the buffer size in the beginning of the buffer.  The buffer size will be stored in 
      cwf[hnd].buffer_size_bytes bytes.  We multiply by 8 to get the number of bits to skip.  */

  bpos = cwf[hnd].buffer_size_bytes * 8;


  /*  [CWF:1]  Loop through each of the nine channels.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  [CWF:1-0] First unpack the number of packets for this channel from the buffer.  */

      record->number_of_packets[i] = czmil_bit_unpack (buffer, bpos, cwf[hnd].num_packets_bits);
      bpos += cwf[hnd].num_packets_bits;


      /*  [CWF:1-1]  Next, unpack the channel packet numbers from the buffer.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {
          record->channel_ndx[i][j] = czmil_bit_unpack (buffer, bpos, cwf[hnd].packet_number_bits);
          bpos += cwf[hnd].packet_number_bits;
        }


      /*  [CWF:1-2]  Then, unpack the MCWP ranges from the buffer.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {
          ui32value = czmil_bit_unpack (buffer, bpos, cwf[hnd].range_bits);
          bpos += cwf[hnd].range_bits;


          /*  Return invalid values as -1.0  */

          if (ui32value == cwf[hnd].range_max)
            {
              record->range[i][j] = -1.0;
            }
          else
            {
              record->range[i][j] = (float) ui32value / cwf[hnd].range_scale;
            }
        }


      /*  Now unpack the packets.  */

      for (j = 0 ; j < record->number_of_packets[i] ; j++)
        {
          /*  [CWF:2]  Unpack the compression type.  */

          type = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_bits);
          bpos += cwf[hnd].type_bits;


          /*  "Shorthand"  */

          packet = &record->channel[i][j * 64];


          /*  [CWF:3]  Unpack the buffer.  */

          switch (type)
            {

              /*  Compression type 1.  Bit packed first differences.  Approximately 72%.  */

            case CZMIL_FIRST_DIFFERENCE:

              /*  Unpack the start value.  */

              start = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_start_bits);
              bpos += cwf[hnd].type_1_start_bits;


              /*  Unpack the offset value.  */

              offset = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_offset_bits) - cwf[hnd].type_1_offset;
              bpos += cwf[hnd].type_1_offset_bits;


              /*  Unpack the delta bits value.  */

              delta_bits = czmil_bit_unpack (buffer, bpos, cwf[hnd].delta_bits);
              bpos += cwf[hnd].delta_bits;


              /*  Unpack the first differences and remove the offset.  */

              for (k = 0 ; k < 63 ; k++)
                {
                  delta[k] = czmil_bit_unpack (buffer, bpos, delta_bits) - offset;
                  bpos += delta_bits;
                }


              /*  Convert the first differences to waveform values using the start value.  */

              packet[0] = previous = start;

              for (k = 1 ; k < 64 ; k++)
                {
                  packet[k] = delta[k - 1] + previous;
                  previous = packet[k];
                }

              break;


              /*  Compression type 3.  Bit packed differences between the central shallow channel and one of the
                  surrounding shallow channels.  Approximately 19%.  */

            case CZMIL_SHALLOW_CENTRAL_DIFFERENCE:

              /*  "Shorthand"  */

              shallow_central = &record->channel[CZMIL_SHALLOW_CHANNEL_1][j * 64];


              /*  Unpack the type 3 offset value.  */

              offset = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_3_offset_bits) - cwf[hnd].type_1_offset;
              bpos += cwf[hnd].type_3_offset_bits;


              /*  Unpack the delta bits value.  */

              delta_bits = czmil_bit_unpack (buffer, bpos, cwf[hnd].delta_bits);
              bpos += cwf[hnd].delta_bits;


              /*  Unpack the channel differences and remove the offset.  */

              for (k = 0 ; k < 64 ; k++)
                {
                  delta[k] = czmil_bit_unpack (buffer, bpos, delta_bits) - offset;
                  bpos += delta_bits;
                }


              /*  Convert the channel differences to waveform values using the corresponding shallow central channel value.  */

              for (k = 0 ; k < 64 ; k++) packet[k] = delta[k] + shallow_central[k];

              break;


              /*  Compression type 2.  Bit packed second differences.  Approximately 8%.  */

            case CZMIL_SECOND_DIFFERENCE:

              /*  Unpack the type 1 and type 2 start bits values.  */

              start = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_start_bits);
              bpos += cwf[hnd].type_1_start_bits;
              start2 = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_2_start_bits);
              bpos += cwf[hnd].type_2_start_bits;


              /*  Unpack the type 1 and type 2 offset values.  */

              offset = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_offset_bits) - cwf[hnd].type_1_offset;
              bpos += cwf[hnd].type_1_offset_bits;
              offset2 = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_2_offset_bits) - cwf[hnd].type_2_offset;
              bpos += cwf[hnd].type_2_offset_bits;


              /*  Unpack the delta bits value.  */

              delta_bits = czmil_bit_unpack (buffer, bpos, cwf[hnd].delta_bits);
              bpos += cwf[hnd].delta_bits;


              /*  Unpack the second differences and remove the second offset.  */

              for (k = 0 ; k < 62 ; k++)
                {
                  delta2[k] = czmil_bit_unpack (buffer, bpos, delta_bits) - offset2;
                  bpos += delta_bits;
                }


              /*  Convert the second differences to first differences using the second start value with the first difference offset
                  removed.  */

              delta[0] = previous = start2 - offset;

              for (k = 1 ; k < 63 ; k++)
                {
                  delta[k] = delta2[k - 1] + previous;
                  previous = delta[k];
                }


              /*  Convert the first differences to waveform values using the first start value.  */

              packet[0] = previous = start;

              for (k = 1 ; k < 64 ; k++)
                {
                  packet[k] = delta[k - 1] + previous;
                  previous = packet[k];
                }

              break;


              /*  Compression type 0.  Just 10 bit bit-packed.  Usually less than 1%.  */

            case CZMIL_BIT_PACKED:

              /*  Just unpack the values.  */

              for (k = 0 ; k < 64 ; k++)
                {
                  packet[k] = czmil_bit_unpack (buffer, bpos, 10);
                  bpos += 10;
                }
              break;
            }
        }
    }


  /*  Unpack the T0 waveform data.  Remember, we don't have a type for this packet since we always use first difference.  */

  packet = record->T0;


  /*  [CWF:4]  Unpack the T0 data.  */

  start = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_start_bits);
  bpos += cwf[hnd].type_1_start_bits;
  offset = czmil_bit_unpack (buffer, bpos, cwf[hnd].type_1_offset_bits) - cwf[hnd].type_1_offset;
  bpos += cwf[hnd].type_1_offset_bits;
  delta_bits = czmil_bit_unpack (buffer, bpos, cwf[hnd].delta_bits);
  bpos += cwf[hnd].delta_bits;


  /*  Unpack the first differences and remove the offset.  */

  for (k = 0 ; k < 63 ; k++)
    {
      delta[k] = czmil_bit_unpack (buffer, bpos, delta_bits) - offset;
      bpos += delta_bits;
    }


  /*  Convert the first differences to waveform values using the start value.  */

  packet[0] = previous = start;

  for (k = 1 ; k < 64 ; k++)
    {
      packet[k] = delta[k - 1] + previous;
      previous = packet[k];
    }


  /*  [CWF:5]  Unpack shot ID.  */

  record->shot_id = czmil_bit_unpack (buffer, bpos, cwf[hnd].shot_id_bits);
  bpos += cwf[hnd].shot_id_bits;


  /*  [CWF:6]  Unpack timestamp.  */

  ui32value = czmil_bit_unpack (buffer, bpos, cwf[hnd].time_bits);
  bpos += cwf[hnd].time_bits;
  record->timestamp = cwf[hnd].header.flight_start_timestamp + (uint64_t) ui32value;


  /*  [CWF:7]  Unpack scan angle.  */

  i32value = czmil_bit_unpack (buffer, bpos, cwf[hnd].scan_angle_bits);
  bpos += cwf[hnd].scan_angle_bits;
  record->scan_angle = (float) i32value / cwf[hnd].angle_scale;


  /****************************************** VERSION CHECK ******************************************

      This field did not exist prior to major version 2.

  ***************************************************************************************************/

  /*  [CWF:8]  Waveform validity reason.  */

  if (cwf[hnd].major_version >= 2)
    {
      for (i = 0 ; i < 9 ; i++)
        {
          record->validity_reason[i] = czmil_bit_unpack (buffer, bpos, cwf[hnd].validity_reason_bits);
          bpos += cwf[hnd].validity_reason_bits;
        }
    }
  else
    {
      for (i = 0 ; i < 9 ; i++) record->validity_reason[i] = 0;
    }


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_read_cwf_record_array

 - Purpose:     Retrieve CZMIL CWF records and fill the supplied array of CWF records.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the first CWF record to be retrieved
                - num_requested  =    The number of CWF records requested.
                - record_array   =    The pointer to the array of CWF records that will be populated

 - Returns:
                - The number of records filled or...
                - Error value returned from czmil_read_cwf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_cwf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CWF_Data *record_array)
{
  int32_t i, num_read = 0, recs = 0;


  /*  Make sure we don't try to read past the end of the file.  */

  recs = MIN (recnum + num_requested, cwf[hnd].header.number_of_records) - recnum;


  /*  Loop through the requested number of records (or up to the end of the file).  */

  for (i = 0 ; i < recs ; i++)
    {
      if (czmil_read_cwf_record (hnd, recnum + i, &record_array[i]) < 0) return (czmil_error.czmil);

      num_read++;
    }


  /*  Return the number of records read (since it may not be the same as the number requested if we
      bumped up against the end of file).  */

  return (num_read);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_read_cwf_record

 - Purpose:     Retrieve a CZMIL CWF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be retrieved
                - record         =    The returned CZMIL CWF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CWF_READ_FSEEK_ERROR
                - CZMIL_CWF_READ_ERROR
                - CZMIL_CWF_CIF_BUFFER_SIZE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_cwf_record (int32_t hnd, int32_t recnum, CZMIL_CWF_Data *record)
{
  int32_t size;
  CZMIL_CIF_Data cif_record;


  /*  The local buffer will never be sizeof (CZMIL_CWF_Data) in size since we are unpacking it but this way we don't have
      to worry about an SOD error (if you don't know what SOD stands for you're probably not cleared for RIDICULOUS).  */

  uint8_t buffer[sizeof (CZMIL_CWF_Data)];


  /*  Check for record out of bounds.  */

  if (recnum >= cwf[hnd].header.number_of_records || recnum < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cwf[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Get the CWF record byte address and buffer size from the CIF index file.  */

  if (czmil_read_cif_record (cwf[hnd].cif_hnd, recnum, &cif_record)) return (czmil_error.czmil);


  /*  We only want to do the fseek (which flushes the buffer) if our last operation was a write or if we aren't already in the correct position.  */

  if (cwf[hnd].write || cif_record.cwf_address != cwf[hnd].pos)
    {
      if (fseeko64 (cwf[hnd].fp, cif_record.cwf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF record :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_READ_FSEEK_ERROR);
        }


      /*  Set the new position since we fseeked.  */

      cwf[hnd].pos = cif_record.cwf_address;
    }


  cwf[hnd].at_end = 0;


  /*  Read the buffer.  */

  if (!fread (buffer, cif_record.cwf_buffer_size, 1, cwf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CWF record :\n%s\n"), cwf[hnd].path, recnum, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_READ_ERROR);
    }


  /*  Make sure the buffer size read from the CWF file matches the buffer size read from the CIF file.  This is just a sanity
      check.  If it happens, something is terribly wrong.  */

  size = czmil_bit_unpack (buffer, 0, cwf[hnd].buffer_size_bytes * 8);

  if (size != cif_record.cwf_buffer_size)
    {
      sprintf (czmil_error.info,
               _("File : %s\nRecord : %d\nBuffer sizes from CIF (%d) and CWF (%d) files don't match.\nYou should delete the CIF file and let it be regenerated.\n"),
               cwf[hnd].path, recnum, cif_record.cwf_buffer_size, size);
      return (czmil_error.czmil = CZMIL_CWF_CIF_BUFFER_SIZE_ERROR);
    }


  /*  Unpack the record.  */

  czmil_uncompress_cwf_record (hnd, record, buffer);


  /*  Add the record size to the file location so we can keep track of where we are in the file (to avoid unnecessay fseeks).  */

  cwf[hnd].pos += (int64_t) size;
  cwf[hnd].at_end = 0;
  cwf[hnd].write = 0;

  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_write_cwf_record_array

 - Purpose:     Appends the supplied arrays of raw waveform records and waveforms to the CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - num_supplied   =    The number of CWF records to be written.
                - waveform       =    The array of CZMIL_RAW_WAVEFORM_Data structures (see czmil_optech.h)
                - data           =    The data block of all of the 10 bit packed waveform data

 - Returns:
                - The number of records written (which should be equal to num_supplied) or...
                - Error value returned from czmil_write_cwf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_cwf_record_array (int32_t hnd, int32_t num_supplied, CZMIL_WAVEFORM_RAW_Data *waveform, uint8_t *data)
{
  int32_t i, num_written = 0, bytes = 0, byte_pos = 0;


  /*  Loop through the records and call the record writing function.  */

  for (i = 0 ; i < num_supplied ; i++)
    {
      if ((bytes = czmil_write_cwf_record (hnd, &waveform[i], &data[byte_pos])) <= 0) return (czmil_error.czmil);

      byte_pos += bytes;

      num_written++;
    }


  /*  Return the number of records written.  This should always be the same as num_supplied.  */

  return (num_written);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cwf_record

 - Purpose:     Appends a CZMIL CWF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - waveform       =    The CZMIL_RAW_WAVEFORM_Data structure (see czmil_optech.h)
                - data           =    The data block is a bit packed block of waveform information
                                      whose contents will depend upon the contents of the number of
                                      packets fields in the CZMIL_RAW_WAVEFORM_Data structure.  As an
                                      example, assume that the number of packets is 3, 3, 3, 3, 3, 3,
                                      3, 4, 10, for shallow channels 1 through 7, the IR channel, and
                                      the deep channel respectively.  In that case, the data block
                                      would be constructed as follows:<br><br>

                                      - shallow channel 1, packet number of 1st [0] 64 sample packet
                                      - shallow channel 1, packet number of 2nd [1] packet
                                      - shallow channel 1, packet number of 3rd [2] packet
                                      - shallow channel 1, MCWP range for 1st packet
                                      - shallow channel 1, MCWP range for 2nd packet
                                      - shallow channel 1, MCWP range for 3rd packet
                                      - shallow channel 1, 64 10 bit samples for 1st packet
                                      - shallow channel 1, 64 10 bit samples for 2nd packet
                                      - shallow channel 1, 64 10 bit samples for 3rd packet
                                      -
                                      - shallow channel 2, packet number of 1st 64 sample packet
                                      - shallow channel 2, packet number of 2nd packet
                                      - shallow channel 2, packet number of 3rd packet
                                      - shallow channel 2, MCWP range for 1st packet
                                      - shallow channel 2, MCWP range for 2nd packet
                                      - shallow channel 2, MCWP range for 3rd packet
                                      - shallow channel 2, 64 10 bit samples for 1st packet
                                      - shallow channel 2, 64 10 bit samples for 2nd packet
                                      - shallow channel 2, 64 10 bit samples for 3rd packet
                                      -
                                      - Same for shallow channels 3 through 7
                                      -
                                      - Same for IR channel packets 0 through 3
                                      -
                                      - Same for deep channel packets 0 through 9

 - Returns:
                - The number of bytes unpacked from the data block
                - CZMIL_CWF_APPEND_ERROR
                - CZMIL_CWF_WRITE_FSEEK_ERROR
                - CZMIL_CWF_WRITE_ERROR
                - Error values returned from czmil_compress_cwf_record

 - Caveats:     This function is ONLY used to append a new record to a newly created file.
                There should be no reason to actually change waveform data after the file
                has been created.

                This function does not actually write the record.  It packs it into the 
                cwf[hnd].io_buffer which will be flushed to disk when it becomes
                close to full.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_cwf_record (int32_t hnd, CZMIL_WAVEFORM_RAW_Data *waveform, uint8_t *data)
{
  int32_t i, j, k, bpos, bytes = 0;
  CZMIL_CWF_Data record;


  /*  Appending a record is only allowed if you are creating a new file.  */

  if (!cwf[hnd].created)
    {
      sprintf (czmil_error.info, _("File : %s\nAppending to pre-existing CWF file not allowed.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_APPEND_ERROR);
    }


  /*  If we're not already at the end of the file, move there.  Even though we're not doing any actual I/O here this makes
      sure that the file pointer is at the end of the file prior to flushing the I/O buffer.  It only happens once so the
      overhead is minimal.  */

  if (!cwf[hnd].at_end)
    {
      /*  We're appending so we need to seek to the end of the file.  */

      if (fseeko64 (cwf[hnd].fp, 0, SEEK_END) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CWF record :\n%s\n"), cwf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CWF_WRITE_FSEEK_ERROR);
        }
    }


  cwf[hnd].at_end = 1;


  /*  First we need to convert the input data to a CZMIL_CWF_Data structure before we compress it.  Before we do that
      we need to zero the output record.  */

  memset (&record, 0, sizeof (CZMIL_CWF_Data));


  /*  Transfer the easy stuff.  */

  record.shot_id = waveform->shot_id;
  record.timestamp = waveform->timestamp;
  record.scan_angle = waveform->scan_angle;
  for (i = 0 ; i < 9 ; i++) record.validity_reason[i] = waveform->validity_reason[i];


  bpos = 0;


  /*  Unpack the T0 data.  */

  for (i = 0 ; i < 64 ; i++)
    {
      record.T0[i] = czmil_bit_unpack (waveform->T0, bpos, 10);
      bpos += 10;
    }


  /*  Starting bit position in the "data" buffer.  */

  bpos = 0;


  /*  Unpack each channel.  */

  for (i = 0 ; i < 9 ; i++)
    {
      record.number_of_packets[i] = waveform->number_of_packets[i];


      /*  Unpack the packet number.  */
      /*  HydroFusion will always send 15 (CZMIL_MAX_PACKETS) but Hemanth said it was faster to use the constant in this loop instead
          of using the variable (which was always 15 (CZMIL_MAX_PACKETS).  */

      for (j = 0 ; j < CZMIL_MAX_PACKETS/*waveform->number_of_packets[i]*/ ; j++)
        {
          record.channel_ndx[i][j] = czmil_bit_unpack (data, bpos, 8);
          bpos += 8;
        }


      /*  Unpack the MCWP range.  */
      /*  HydroFusion will always send 15 (CZMIL_MAX_PACKETS) but Hemanth said it was faster to use the constant in this loop instead
          of using the variable (which was always 15 (CZMIL_MAX_PACKETS).  */

      for (j = 0 ; j < CZMIL_MAX_PACKETS/*waveform->number_of_packets[i]*/ ; j++)
        {
          record.range[i][j] = (float) czmil_bit_unpack (data, bpos, 8);
          bpos += 8;
        }


      /*  Unpack the packets.  */
      /*  HydroFusion will always send 15 (CZMIL_MAX_PACKETS) but Hemanth said it was faster to use the constant in this loop instead
          of using the variable (which was always 15 (CZMIL_MAX_PACKETS).  */

      for (j = 0 ; j < CZMIL_MAX_PACKETS/*waveform->number_of_packets[i]*/ ; j++)
        {
          for (k = 0 ; k < 64 ; k++)
            {
              record.channel[i][j * 64 + k] = czmil_bit_unpack (data, bpos, 10);
              bpos += 10;
            }
        }
    }


  /*  Compute the number of bytes unpacked from the data block.  */

  bytes = bpos / 8;


  /*  Check for first record so we can set the start timestamp.  */

  if (!cwf[hnd].header.flight_end_timestamp)
    {
      cwf[hnd].header.flight_start_timestamp = record.timestamp;
    }
  else
    {
      /*  Check for a time regression using the end timestamp that is stored in the header structure each time we add a record.  */

      if (record.timestamp <= cwf[hnd].header.flight_end_timestamp)
        {
          /*  Replace the bad time with the previous time plus 100 microseconds.  I hate doing this!  The reason we are doing this is that
              the MCWP sometimes loses its mind for a shot and gets a corrupted time.  Supposedly Optech is handling this on their end but
              they asked me to check it as well and correct when needed.  */

          record.timestamp = cwf[hnd].header.flight_end_timestamp + 100;
          for (i = 0 ; i < 9 ; i++) record.validity_reason[i] = CZMIL_TIMESTAMP_INVALID;
        }
    }


  /*  Set the end timestamp so that it will be correct when we close the file.  We also use it for time regression testing above.  */

  cwf[hnd].header.flight_end_timestamp = record.timestamp;


  /*  Pack the record.  */

  if ((czmil_compress_cwf_record (hnd, &record)) < 0) return (czmil_error.czmil);


  /*  Increment the number of records counter in the header.  */

  cwf[hnd].header.number_of_records++;


  return (bytes);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_read_cpf_record_array

 - Purpose:     Retrieve CZMIL CPF records and fill the supplied array of CPF records.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the first CPF record to be retrieved
                - num_requested  =    The number of CPF records requested.
                - record_array   =    The pointer to the array of CPF records that will be populated

 - Returns:
                - The number of records filled or...
                - Error value returned from czmil_read_cpf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_cpf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CPF_Data *record_array)
{
  int32_t i, num_read = 0, recs = 0;


  /*  Make sure we don't try to read past the end of the file.  */

  recs = MIN (recnum + num_requested, cpf[hnd].header.number_of_records) - recnum;


  /*  Loop through the requested number of records (or up to the end of the file).  */

  for (i = 0 ; i < recs ; i++)
    {
      if (czmil_read_cpf_record (hnd, recnum + i, &record_array[i]) < 0) return (czmil_error.czmil);

      num_read++;
    }


  /*  Return the number of records read (since it may not be the same as the number requested if we
      bumped up against the end of file).  */

  return (num_read);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_cpf_record

 - Purpose:     Retrieve a CZMIL CPF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/14/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be retrieved
                - record         =    The returned CZMIL CPF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CPF_READ_FSEEK_ERROR
                - CZMIL_CPF_READ_ERROR
                - CZMIL_CPF_CIF_BUFFER_SIZE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CPF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record)
{
  double ref_lat, ref_lon;
  int32_t i, j, bpos, size, i32value, lat_band;
  CZMIL_CIF_Data cif_record;


  /*  Check for record out of bounds.  */

  if (recnum >= cpf[hnd].header.number_of_records || recnum < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cpf[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Get the CPF record byte address and buffer size from the CIF index file.  */

  if (czmil_read_cif_record (cpf[hnd].cif_hnd, recnum, &cif_record)) return (czmil_error.czmil);


  /*  We only want to do the fseek (which flushes the buffer) if our last operation was a write or if we aren't in the
      correct position.  */

  if (cpf[hnd].write || cif_record.cpf_address != cpf[hnd].pos)
    {
      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_READ_FSEEK_ERROR);
        }


      /*  Set the new position since we fseeked.  */

      cpf[hnd].pos = cif_record.cpf_address;
    }


  cpf[hnd].at_end = 0;


  if (!fread (cpf[hnd].buffer, cif_record.cpf_buffer_size, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CPF record :\n%s\n"), cpf[hnd].path, recnum, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_READ_ERROR);
    }


  /*  [CPF:0]  CPF record buffer size.  */

  bpos = 0;
  size = czmil_bit_unpack (cpf[hnd].buffer, 0, cpf[hnd].buffer_size_bytes * 8);
  bpos += cpf[hnd].buffer_size_bytes * 8;


  /*  Make sure the buffer size read from the CPF file matches the buffer size read from the CIF file.  This is just a sanity
      check.  If it happens, something is terribly wrong.  */

  if (size != cif_record.cpf_buffer_size)
    {
      sprintf (czmil_error.info,
               _("File : %s\nRecord : %d\nBuffer sizes from CIF (%d) and CPF (%d) files don't match.\nYou should delete the CIF file and let it be regenerated.\n"),
               cpf[hnd].path, recnum, cif_record.cpf_buffer_size, size);
      return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
    }


  /*  [CPF:1]  Number of returns per channel.  */

  for (i = 0 ; i < 9 ; i++)
    {
      record->returns[i] = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].return_bits);
      bpos += cpf[hnd].return_bits;
    }


  /*  [CPF:2]  Timestamp.  */
  
  record->timestamp = cpf[hnd].header.flight_start_timestamp + (uint64_t) czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].time_bits);
  bpos += cpf[hnd].time_bits;


  /*  [CPF:3]  Off nadir angle.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].off_nadir_angle_bits);
  bpos += cpf[hnd].off_nadir_angle_bits;
  record->off_nadir_angle = (float) (i32value - cpf[hnd].off_nadir_angle_offset) / cpf[hnd].angle_scale;


  /*  [CPF:4]  Reference latitude and longitude.
      Note that base_lat and base_lon are already offset by 90 and 180 respectively.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lat_bits);
  bpos += cpf[hnd].lat_bits;
  ref_lat = (double) (i32value - cpf[hnd].lat_offset) / cpf[hnd].lat_scale + cpf[hnd].header.base_lat;
  record->reference_latitude = ref_lat - 90.0;


  /*  Compute the latitude band to index into the cosine array for longitudes.  */

  lat_band = (int32_t) ref_lat;


  /*  [CPF:5]  Reference longitude.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lon_bits);
  bpos += cpf[hnd].lon_bits;
  ref_lon = (double) (i32value - cpf[hnd].lon_offset) / cos_array[lat_band] / cpf[hnd].lon_scale + cpf[hnd].header.base_lon;
  record->reference_longitude = ref_lon - 180.0;


  /*  [CPF:6]  Water level elevation.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].elev_bits);
  bpos += cpf[hnd].elev_bits;


  /*  Check for null value (max integer stored).  */

  if (i32value == cpf[hnd].elev_max)
    {
      record->water_level = cpf[hnd].header.null_z_value;
    }
  else
    {
      record->water_level = (float) (i32value - cpf[hnd].elev_offset) / cpf[hnd].elev_scale;
    }


  /*  [CPF:7]  Local vertical datum offset (elevation).  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].elev_bits);
  bpos += cpf[hnd].elev_bits;
  record->local_vertical_datum_offset = (float) (i32value - cpf[hnd].elev_offset) / cpf[hnd].elev_scale;


  /*  [CPF:8]  User data (this used to be spare in v2 and shot status in v1 but it was never used).  */

  record->user_data = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].user_data_bits);
  bpos += cpf[hnd].user_data_bits;


  /*  [CPF:9]  Loop through all nine channels.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  If returns are present...  */

      for (j = 0 ; j < record->returns[i] ; j++)
        {
          /*  [CPF:9-0]  Return latitude.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lat_diff_bits);
          bpos += cpf[hnd].lat_diff_bits;
          record->channel[i][j].latitude = (double) ((i32value - cpf[hnd].lat_diff_offset) / cpf[hnd].lat_diff_scale + ref_lat) - 90.0;


          /*  [CPF:9-1]  Return longitude.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lon_diff_bits);
          bpos += cpf[hnd].lon_diff_bits;
          record->channel[i][j].longitude = (double) ((i32value - cpf[hnd].lon_diff_offset) / cpf[hnd].lon_diff_scale / cos_array[lat_band] + ref_lon) - 180.0;


          /*  [CPF:9-2]  Return elevation.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].elev_bits);
          bpos += cpf[hnd].elev_bits;


          /*  Check for null value (max integer stored).  */

          if (i32value == cpf[hnd].elev_max)
            {
              record->channel[i][j].elevation = cpf[hnd].header.null_z_value;
            }
          else
            {
              record->channel[i][j].elevation = (float) (i32value - cpf[hnd].elev_offset) / cpf[hnd].elev_scale;
            }


          /*  [CPF:9-3]  Reflectance.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].reflectance_bits);
          bpos += cpf[hnd].reflectance_bits;
          record->channel[i][j].reflectance = (float) i32value / cpf[hnd].reflectance_scale;


          /*  [CPF:9-4]  Horizontal uncertainty.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].uncert_bits);
          bpos += cpf[hnd].uncert_bits;
          record->channel[i][j].horizontal_uncertainty = (float) i32value / cpf[hnd].uncert_scale;


          /*  [CPF:9-5]  Vertical uncertainty.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].uncert_bits);
          bpos += cpf[hnd].uncert_bits;
          record->channel[i][j].vertical_uncertainty = (float) i32value / cpf[hnd].uncert_scale;


          /*  [CPF:9-6]  Per return status.  */

          record->channel[i][j].status = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].return_status_bits);
          bpos += cpf[hnd].return_status_bits;


          /*  [CPF:9-7]  Per return classification.  */

          record->channel[i][j].classification = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].class_bits);
          bpos += cpf[hnd].class_bits;


          /*  [CPF:9-8]  Interest point.  */

          i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].interest_point_bits);
          bpos += cpf[hnd].interest_point_bits;
          record->channel[i][j].interest_point = (float) i32value / cpf[hnd].interest_point_scale;


          /*  [CPF:9-9]  Interest point rank.  */

          record->channel[i][j].ip_rank = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].ip_rank_bits);
          bpos += cpf[hnd].ip_rank_bits;


          /************************************************* IMPORTANT NOTE ***********************************************

            In earlier versions of the CZMIL API we were using an ip_rank of 0 to signify water surface.  When we switched
            to v3.00 Optech International added CZMIL_OPTECH_CLASS_HYBRID processing mode and we had some confusion about
            how to designate water surface, water processed, land processed, etc.  To save a lot of work in CZMIL
            applications, the following line "fixes" all of these problems (I hope).  Basically, if the "classification" is
            set to anything other than zero, we do nothing.  If the "classification" is set to 0 AND the "ip_rank" is 0, we
            hard-code the "classification" to be 41 (this is a "Water surface" classification based on the "Proposed LAS
            Enhancements to Support Topo-Bathy Lidar", July 17, 2013).  This simplifies water surface detection in CZMIL
            applications.
            
          ************************************************* IMPORTANT NOTE ***********************************************/

          if (record->channel[i][j].classification == 0 && record->channel[i][j].ip_rank == 0) record->channel[i][j].classification = 41;
        }
    }


  /*  [CPF:10]  Loop through the 7 shallow channels and unpack the bare earth values.  */

  for (i = 0 ; i < 7 ; i++)
    {
      /*  [CPF:10-0]  Bare earth latitude.  */

      i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lat_diff_bits);
      bpos += cpf[hnd].lat_diff_bits;
      record->bare_earth_latitude[i] = (double) ((i32value - cpf[hnd].lat_diff_offset) / cpf[hnd].lat_diff_scale + ref_lat) - 90.0;


      /*  [CPF:10-1]  Bare earth longitude.  */

      i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lon_diff_bits);
      bpos += cpf[hnd].lon_diff_bits;
      record->bare_earth_longitude[i] = (double) ((i32value - cpf[hnd].lon_diff_offset) / cpf[hnd].lon_diff_scale / cos_array[lat_band] +
                                                      ref_lon) - 180.0;


      /*  [CPF:10-2]  Bare earth elevation.  */

      i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].elev_bits);
      bpos += cpf[hnd].elev_bits;


      /*  Check for null value (max integer stored).  */
      
      if (i32value == cpf[hnd].elev_max)
        {
          record->bare_earth_elevation[i] = cpf[hnd].header.null_z_value;
        }
      else
        {
          record->bare_earth_elevation[i] = (float) (i32value - cpf[hnd].elev_offset) / cpf[hnd].elev_scale;
        }
    }


  /*  [CPF:11]  Kd value.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].kd_bits);
  bpos += cpf[hnd].kd_bits;
  record->kd = (float) i32value / cpf[hnd].kd_scale;


  /*  [CPF:12]  Laser energy.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].laser_energy_bits);
  bpos += cpf[hnd].laser_energy_bits;
  record->laser_energy = (float) i32value / cpf[hnd].laser_energy_scale;


  /*  [CPF:13]  T0 interest point.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].interest_point_bits);
  bpos += cpf[hnd].interest_point_bits;
  record->t0_interest_point = (float) i32value / cpf[hnd].interest_point_scale;


  /****************************************** VERSION CHECK ******************************************

      The probability, filter_reason, and optech_classification fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 2)
    {
      /*  [CPF:14]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  [CPF:14-0]  Optech waveform processing mode.  */

          record->optech_classification[i] = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].optech_classification_bits);
          bpos += cpf[hnd].optech_classification_bits;


          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:14-1]  Probability of detection.  */

              i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].probability_bits);
              bpos += cpf[hnd].probability_bits;
              record->channel[i][j].probability = (float) i32value / cpf[hnd].probability_scale;


              /*  [CPF:14-2]  Per return filter reason.  */

              record->channel[i][j].filter_reason = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].return_filter_reason_bits);
              bpos += cpf[hnd].return_filter_reason_bits;
            }
        }
    }
  else
    {
      for (i = 0 ; i < 9 ; i++)
        {
          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
	      record->channel[i][j].probability = 0.0;
	      record->channel[i][j].filter_reason = 0;


              /*  Prior to version 2.0, Optech classification (processing mode) was stored in the return classification slot so we'll steal it here.
                  They were all the same per channel so we'll just take whatever the one for the last valid return was set to.  In addition, in 
                  version 2, the water modes were biased by 30 to separate them from the land modes.  So 2 through 8 in version 1 equates to
                  32 through 40 in version 2.  */

              if (record->channel[i][j].classification > 1)
                {
                  record->optech_classification[i] = record->channel[i][j].classification + 30;
                }
              else
                {
                  record->optech_classification[i] = record->channel[i][j].classification;
                }
	    }
        }
    }


  /****************************************** VERSION CHECK ******************************************

      The d_index_cube and d_index fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 3)
    {
      /*  [CPF:15]  d_index_cube.  */

      record->d_index_cube = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].d_index_cube_bits);
      bpos += cpf[hnd].d_index_cube_bits;


      /*  [CPF:16]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:16-0]  d_index.  */

              record->channel[i][j].d_index = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].d_index_bits);
              bpos += cpf[hnd].d_index_bits;
            }
        }
    }
  else
    {
      record->d_index_cube = 0;

      for (i = 0 ; i < 9 ; i++)
        {
          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              record->channel[i][j].d_index = 0;
            }
        }
    }


  /*  Set the last record read so that, if we are doing updates, we can avoid a reread of the buffer.  */

  cpf[hnd].last_record_read = recnum;


  /*  Add the record size to the file location so we can keep track of where we are in the file (to avoid unnecessay fseeks).  */

  cpf[hnd].pos += size;
  cpf[hnd].modified = 0;
  cpf[hnd].write = 0;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}




/*********************************************************************************************/
/*!

 - Function:    czmil_write_cpf_record_array

 - Purpose:     Appends the supplied arrays of raw waveform records and waveforms to the CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The starting CZMIL CPF record number or CZMIL_NEXT_RECORD to append
                                      to a file that is being created
                - num_supplied   =    The number of CPF records to be written.
                - record_array   =    The array of CZMIL_CPF_Data structures

 - Returns:
                - The number of records written (which should be equal to num_supplied) or...
                - Error value returned from czmil_write_cpf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

                To create a file, recnum <b>MUST</b> be set to CZMIL_NEXT_RECORD.  To update a file
                using this function CZMIL_NEXT_RECORD <b>CANNOT</b> be used.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_cpf_record_array (int32_t hnd, int32_t recnum, int32_t num_supplied, CZMIL_CPF_Data *record_array)
{
  int32_t i, num_written = 0, rec = 0;


  /*  Loop through the records and call the record writing function.  */

  for (i = 0 ; i < num_supplied ; i++)
    {
      if (recnum == CZMIL_NEXT_RECORD)
        {
          rec = CZMIL_NEXT_RECORD;
        }
      else
        {
          rec = recnum + i;
        }

      if (czmil_write_cpf_record (hnd, rec, &record_array[i]) < 0) return (czmil_error.czmil);

      num_written++;
    }


  /*  Return the number of records written.  This should always be the same as num_supplied.  */

  return (num_written);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cpf_record

 - Purpose:     Append or modify a CZMIL CPF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The CZMIL CPF record number or CZMIL_NEXT_RECORD to append
                                      to a file that is being created
                - record         =    The CZMIL_CPF_Data structure to be written

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CPF_WRITE_FSEEK_ERROR
                - CZMIL_CPF_APPEND_ERROR
                - CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR
                - CZMIL_CPF_WRITE_ERROR
                - CZMIL_CPF_TIME_REGRESSION_ERROR
                - CZMIL_CPF_READ_ERROR
                - CZMIL_CPF_CIF_BUFFER_SIZE_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CPF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

                To append to a file that is being created, recnum <b>MUST</b> be set to
                CZMIL_NEXT_RECORD.  To update an already existing file using this function,
                CZMIL_NEXT_RECORD <b>CANNOT</b> be used.

 - <b>IMPORTANT NOTE: When using this function to update a record, any field may be modified.
   This may cause aliasing of some parts of the record.  Since we're compressing the record
   there is some loss of precision when writing to the bit-packed buffer.  If we do that over
   and over again a certain amount of "creep" occurs.  This function should only be used by
   the HydroFusion software to append new records or to replace records that have been 
   re-computed (e.g. new location due to re-processing the navigation solution).  DO NOT
   use this call for updating the "modifiable" fields of the CPF record.  Use the
   czmil_update_cpf_record function instead.  See the CZMIL_CPF_Data structure in czmil.h to
   see which fields are considered "modifiable" by post-processing software.</b>

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record)
{
  double ref_lat, ref_lon, lat, lon, band_lat;
  int32_t i, j, bpos, size, i32value, lat_band, err_recnum;
  uint32_t ui32value;
  CZMIL_CIF_Data cif_record;
  uint8_t *buffer;
  uint8_t return_null_z = 0;
  uint8_t bare_earth_null_z = 0;


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }




  /*  This line is just here to suppress a gcc compiler warning.  It is absolutely not necessary.  Try commenting it out
      occasionally to see if the compiler has gotten smarter ;-)  */

  cif_record.cpf_buffer_size = 0;




  /*  Save the record_number for error messages.  */

  err_recnum = recnum;


  /*  This section is for updating a record in a pre-existing file.  */

  if (recnum != CZMIL_NEXT_RECORD)
    {
      /*  Check for record out of bounds.  */

      if (recnum >= cpf[hnd].header.number_of_records || recnum < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cpf[hnd].path, err_recnum);
          return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
        }


      /*  Get the CPF record byte address from the CIF index file.  */

      if (czmil_read_cif_record (cpf[hnd].cif_hnd, recnum, &cif_record)) return (czmil_error.czmil);


      /*  We only want to do the fseek (which flushes the buffer) if our last operation was a read or if we aren't in the correct position.  */

      if (!cpf[hnd].write || cif_record.cpf_address != cpf[hnd].pos)
        {
          if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
            {
              sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
            }


          /*  Set the new position since we fseeked.  */

          cpf[hnd].pos = cif_record.cpf_address;


          /*  Force it to do the subsequent read if we did an fseek.  */

          cpf[hnd].last_record_read = -1;
        }


      /*  Check to see if we just read this record so we can avoid a reread.  */

      if (recnum != cpf[hnd].last_record_read)
        {
          /*  The normal procedure for doing update type operations is to read the record in the external program, modify it, and then write
              the updated information via czmil_update_cpf_record, czmil_update_cpf_return_status, or (on very rare occasions)
              czmil_write_cpf_record.  In these cases we can either skip the read or we can read from the already existing I/O buffer.
              Unfortunately, some external programs will read a large number of sequential records into memory, modify them, and then use
              one of the above mentioned functions to update the record.  In that case this function has been called a number of times
              in sequential order.  The last thing we do in this function is to write a record so we have "write" information in the I/O
              buffer (we also set the "write" flag).  To avoid confusion we need to flush the buffer prior to the subsequent read operation.
              This actually only showed up in Windows, somehow Linux was flushing the buffer properly.  Just to be safe though we'll do the
              fflush in both cases.  */

          if (cpf[hnd].write) fflush (cpf[hnd].fp);


          if (!fread (cpf[hnd].buffer, cif_record.cpf_buffer_size, 1, cpf[hnd].fp))
            {
              sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CPF record prior to write :\n%s\n"), cpf[hnd].path, recnum,
                       strerror (errno));
              return (czmil_error.czmil = CZMIL_CPF_READ_ERROR);
            }


          cpf[hnd].last_record_read = recnum;


          /*  Make sure the buffer size read from the CPF file matches the buffer size read from the CIF file.  */

          bpos = 0;
          size = czmil_bit_unpack (cpf[hnd].buffer, 0, cpf[hnd].buffer_size_bytes * 8);
          bpos += cpf[hnd].buffer_size_bytes * 8;

          if (size != cif_record.cpf_buffer_size)
            {
          fprintf(stderr,"%s %s %d %d %d\n",__FILE__,__FUNCTION__,__LINE__,size,cif_record.cpf_buffer_size);
              sprintf (czmil_error.info, _("File : %s\nRecord : %d\nBuffer sizes from the CIF and CPF files do not match.\n"), cpf[hnd].path, recnum);
              return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
            }


          /*  Position back to where we're going to write the record.  */

          if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
            {
              sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
            }
        }


      /*  Make sure the buffer size read from the CPF file matches the buffer size read from the CIF file.

      bpos = 0;
      size = czmil_bit_unpack (cpf[hnd].buffer, 0, cpf[hnd].buffer_size_bytes * 8);
      bpos += cpf[hnd].buffer_size_bytes * 8;

      if (size != cif_record.cpf_buffer_size)
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nBuffer sizes from the CIF and CPF files do not match.\n"), cpf[hnd].path, err_recnum);
          return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
        }


        Position back to where we're going to write the record.

      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
        }
      */


      cpf[hnd].at_end = 0;


      /*  Set the buffer pointer to the single record buffer in the internal CPF structure.  */

      buffer = cpf[hnd].buffer;
    }


  /*  This is for appending a record to file that is being created.  */

  else
    {
      /*  Appending a record is only allowed if you are creating a new file.  */

      if (!cpf[hnd].created)
        {
          sprintf (czmil_error.info, _("File : %s\nAppending to pre-existing CPF file not allowed.\n"), cpf[hnd].path);
          return (czmil_error.czmil = CZMIL_CPF_APPEND_ERROR);
        }
          

      /*  If there is ANY chance at all that we might overrun our I/O buffer on this write, we need to 
          flush the buffer.  Since we're compressing our records they should not be anywhere near the
          size of a CZMIL_CPF_Data structure.  But, also since we're compressing the records, the actual size
          of the next record is unknown so we are just making sure that we're not within sizeof (CZMIL_CPF_Data)
          bytes of the end of the output buffer.  That way we're taking no chances of exceeding our output
          buffer size.  */

      if ((cpf[hnd].io_buffer_size - cpf[hnd].io_buffer_address) < sizeof (CZMIL_CPF_Data))
        {
          if (czmil_flush_cpf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Set the buffer pointer to the correct location within the I/O buffer.  */

      buffer = &cpf[hnd].io_buffer[cpf[hnd].io_buffer_address];


      cpf[hnd].at_end = 1;


      /*  Check for first record so we can set the start timestamp.  */

      if (!cpf[hnd].header.flight_end_timestamp)
        {
          cpf[hnd].header.flight_start_timestamp = record->timestamp;
        }
      else
        {
          /*  Check for a time regression using the end timestamp that is stored in the header structure each time we add a record.  */

          if (record->timestamp <= cpf[hnd].header.flight_end_timestamp)
            {
              sprintf (czmil_error.info, _("File : %s\nRecord : %d\nTime regression encountered during creation of CPF file:\n"), cpf[hnd].path,
                       cpf[hnd].header.number_of_records);
              return (czmil_error.czmil = CZMIL_CPF_TIME_REGRESSION_ERROR);
            }
        }


      /*  Set the end timestamp so that it will be correct when we close the file.  We also use it for time regression testing above.  */

      cpf[hnd].header.flight_end_timestamp = record->timestamp;


      /*  Check for min and max lat and lon (unbiased by 90 and 180 respectively) since we're appending a record.  */

      for (i = 0 ; i < 9 ; i++)
        {
          for (j = 0 ; j < record->returns[i] ; j++)
            {
              cpf[hnd].header.min_lon = MIN (cpf[hnd].header.min_lon, record->channel[i][j].longitude);
              cpf[hnd].header.max_lon = MAX (cpf[hnd].header.max_lon, record->channel[i][j].longitude);
              cpf[hnd].header.min_lat = MIN (cpf[hnd].header.min_lat, record->channel[i][j].latitude);
              cpf[hnd].header.max_lat = MAX (cpf[hnd].header.max_lat, record->channel[i][j].latitude);
            }
        }

      cpf[hnd].header.min_lon = MIN (cpf[hnd].header.min_lon, record->reference_longitude);
      cpf[hnd].header.max_lon = MAX (cpf[hnd].header.max_lon, record->reference_longitude);
      cpf[hnd].header.min_lat = MIN (cpf[hnd].header.min_lat, record->reference_latitude);
      cpf[hnd].header.max_lat = MAX (cpf[hnd].header.max_lat, record->reference_latitude);


      /*  Save the actual record_number for error messages.  */

      err_recnum = cpf[hnd].header.number_of_records;


      /*  Increment the number of records counter in the header.  */

      cpf[hnd].header.number_of_records++;
    }


  /*  Save the reference lat and lon.  */

  ref_lat = record->reference_latitude + 90.0;
  ref_lon = record->reference_longitude + 180.0;


  /*  [CPF:0]  Pack the record.  cpf[hnd].buffer_size_bytes is the offset for the buffer size which will be stored first in
      the buffer.  We're actually just skipping that part of the buffer here.  It will be populated after we pack the rest
      of the record.  */

  bpos = cpf[hnd].buffer_size_bytes * 8;


  /*  [CPF:1]  Pack number of returns per channel.  */

  for (i = 0 ; i < 9 ; i++)
    {
      if (record->returns[i] > cpf[hnd].return_max)
        {
          sprintf (czmil_error.info,
                   _("In CPF file %s, Record %d :\nNumber of returns %d for channel %d exceeds maximum allowable number of returns %d."),
                   cpf[hnd].path, err_recnum, record->returns[i], i, cpf[hnd].return_max);
          return (czmil_error.czmil = CZMIL_CPF_TOO_MANY_RETURNS_ERROR);
        }

      czmil_bit_pack (buffer, bpos, cpf[hnd].return_bits, record->returns[i]);
      bpos += cpf[hnd].return_bits;
    }


  /*  [CPF:2]  Timestamp.  */

  if (record->timestamp - cpf[hnd].header.flight_start_timestamp > cpf[hnd].time_max)
    {
      sprintf (czmil_error.info,
               _("In CPF file %s, Record %d :\nCPF timestamp %"PRIu64" out of range from start timestamp %"PRIu64"."),
               cpf[hnd].path, err_recnum, record->timestamp, cpf[hnd].header.flight_start_timestamp);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }

  ui32value = (uint32_t) (record->timestamp - cpf[hnd].header.flight_start_timestamp);
  czmil_bit_pack (buffer, bpos, cpf[hnd].time_bits, ui32value);
  bpos += cpf[hnd].time_bits;


  /*  [CPF:3]  Off nadir angle.  */

  i32value = NINT (record->off_nadir_angle * cpf[hnd].angle_scale) + cpf[hnd].off_nadir_angle_offset;
  if (i32value < 0 || i32value > cpf[hnd].off_nadir_angle_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nCPF off nadir angle %f out of range."), cpf[hnd].path, err_recnum,
               record->off_nadir_angle);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].off_nadir_angle_bits, i32value);
  bpos += cpf[hnd].off_nadir_angle_bits;


  /*  [CPF:4]  Reference latitude and longitude.
      Note that base_lat and base_lon are already offset by 90 and 180 respectively.  */

  i32value = NINT ((ref_lat - cpf[hnd].header.base_lat) * cpf[hnd].lat_scale) + cpf[hnd].lat_offset;
  if (i32value < 0 || i32value > cpf[hnd].lat_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReference latitude %f out of range."), cpf[hnd].path, err_recnum,
               record->reference_latitude);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].lat_bits, i32value);


  /*  Compute the latitude band to index into the cosine array for computation of the longitude.  Why are we unpacking what we just 
      packed?  Suppose the latitude that we input was 32.00000001.  If we take the integer latitude (plus 90.0) we get 122, so our
      index into the cosine array is 122.  Unfortunately, the resolution to which we are storing the latitude is 20,000ths of an
      arcsecond.  That is about .0000000135 degrees.  When we unpack the latitude value on read it may get reconstituted as 121.999999999.
      In order to avoid that unpleasantness we unpack the latitude value that we just saved and use that to get the index into the
      cosine array so that we will use the correct value when we read the latitude.  See, there was method to my madness.  The little
      bit of wiggle room that we get by using ref_lat and ref_lon prior to packing and unpacking isn't a problem since our resolution
      is approximately 1.5mm.  We just wanted to make sure that we didn't flip latitude bands in our cosine lookup.  Note that we don't
      increment bpos until after we do this.  */

  i32value = czmil_bit_unpack (buffer, bpos, cpf[hnd].lat_bits);
  band_lat = (double) (i32value - cpf[hnd].lat_offset) / cpf[hnd].lat_scale + cpf[hnd].header.base_lat;
  lat_band = (int32_t) band_lat;


  bpos += cpf[hnd].lat_bits;


  /*  [CPF:5]  Reference longitude.  */

  i32value = NINT ((ref_lon - cpf[hnd].header.base_lon) * cpf[hnd].lon_scale * cos_array[lat_band]) + cpf[hnd].lon_offset;
  if (i32value < 0 || i32value > cpf[hnd].lon_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReference longitude %f out of range."), cpf[hnd].path, err_recnum,
               record->reference_longitude);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].lon_bits, i32value);
  bpos += cpf[hnd].lon_bits;


  /*  [CPF:6]  Water level elevation (check for null first).  */

  if (record->water_level == cpf[hnd].header.null_z_value)
    {
      i32value = cpf[hnd].elev_max;
    }
  else
    {
      i32value = NINT (record->water_level * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
      if (i32value < 0 || i32value > cpf[hnd].elev_max)
        {
          sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nWater level elevation %f out of range."), cpf[hnd].path, err_recnum,
                   record->water_level);
          return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
        }
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
  bpos += cpf[hnd].elev_bits;


  /*  [CPF:7]  Local vertical datum offset (elevation).  */

  i32value = NINT (record->local_vertical_datum_offset * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
  if (i32value < 0 || i32value > cpf[hnd].elev_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nLocal vertical datum offset %f out of range."), cpf[hnd].path, err_recnum, 
               record->local_vertical_datum_offset);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
  bpos += cpf[hnd].elev_bits;


  /*  [CPF:8]  User data (this used to be spare in v2 and shot status in v1 but it was never used).  */

  czmil_bit_pack (buffer, bpos, cpf[hnd].user_data_bits, record->user_data);
  bpos += cpf[hnd].user_data_bits;


  /*  [CPF:9]  Loop through all nine channels.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  If returns are present...  */

      for (j = 0 ; j < record->returns[i] ; j++)
        {
          /*  Check for a null z value in the elevation.  If it's there we're going to force the latitude and longitude to be
              the same as the reference latitude and longitude.  The reason that we do that is that the application may have put
              a 0.0 or some undefined value in the latitude and longitude and that would cause the stored integer value to exceed
              the maximum allowable.  */

          if (record->channel[i][j].elevation == cpf[hnd].header.null_z_value)
            {
              lat = ref_lat;
              lon = ref_lon;
              return_null_z = 1;
            }
          else
            {
              /*  Bias the latitude and longitude.  */

              lat = record->channel[i][j].latitude + 90.0;
              lon = record->channel[i][j].longitude + 180.0;
              return_null_z = 0;
            }


          /*  [CPF:9-0]  Return latitude.  */

          i32value = NINT ((lat - ref_lat) * cpf[hnd].lat_diff_scale) + cpf[hnd].lat_diff_offset;
          if (i32value < 0 || i32value > cpf[hnd].lat_diff_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d latitude %f out of range."), cpf[hnd].path, err_recnum,
                       i, j, record->channel[i][j].latitude);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].lat_diff_bits, i32value);
          bpos += cpf[hnd].lat_diff_bits;


          /*  [CPF:9-1]  Return longitude.  */

          i32value = NINT ((lon - ref_lon) * cpf[hnd].lon_diff_scale * cos_array[lat_band]) + cpf[hnd].lon_diff_offset;
          if (i32value < 0 || i32value > cpf[hnd].lon_diff_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d longitude %f out of range."), cpf[hnd].path, err_recnum,
                       i, j, record->channel[i][j].longitude);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].lon_diff_bits, i32value);
          bpos += cpf[hnd].lon_diff_bits;


          /*  [CPF:9-2]  Return elevation.  */

          if (return_null_z)
            {
              i32value = cpf[hnd].elev_max;
            }
          else
            {
              i32value = NINT (record->channel[i][j].elevation * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
              if (i32value < 0 || i32value > cpf[hnd].elev_max)
                {
                  sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d elevation %f out of range."), cpf[hnd].path, err_recnum,
                           i, j, record->channel[i][j].elevation);
                  return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
                }
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
          bpos += cpf[hnd].elev_bits;


          /*  [CPF:9-3]  Reflectance.  */

          i32value = NINT (record->channel[i][j].reflectance * cpf[hnd].reflectance_scale);
          if (i32value < 0 || i32value > cpf[hnd].reflectance_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d reflectance %f out of range."), cpf[hnd].path,
                       err_recnum, i, j, record->channel[i][j].reflectance);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].reflectance_bits, i32value);
          bpos += cpf[hnd].reflectance_bits;


          /*  [CPF:9-4]  Horizontal uncertainty.  */

          i32value = NINT (record->channel[i][j].horizontal_uncertainty * cpf[hnd].uncert_scale);
          if (i32value > cpf[hnd].uncert_max) i32value = cpf[hnd].uncert_max;
          czmil_bit_pack (buffer, bpos, cpf[hnd].uncert_bits, i32value);
          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-5]  Vertical uncertainty.  */

          i32value = NINT (record->channel[i][j].vertical_uncertainty * cpf[hnd].uncert_scale);
          if (i32value > cpf[hnd].uncert_max) i32value = cpf[hnd].uncert_max;
          czmil_bit_pack (buffer, bpos, cpf[hnd].uncert_bits, i32value);
          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-6]  Per return status.  */

          if (record->channel[i][j].status > cpf[hnd].return_status_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReturn status value %d too large."), cpf[hnd].path, err_recnum,
                       record->channel[i][j].status);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].return_status_bits, record->channel[i][j].status);
          bpos += cpf[hnd].return_status_bits;


          /*  [CPF:9-7]  Per return classification.  */

          if (record->channel[i][j].classification > cpf[hnd].class_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nClassification value %d too large."), cpf[hnd].path, err_recnum,
                       record->channel[i][j].classification);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].class_bits, record->channel[i][j].classification);
          bpos += cpf[hnd].class_bits;


          /*  [CPF:9-8]  Interest point.  */

          i32value = NINT (record->channel[i][j].interest_point * cpf[hnd].interest_point_scale);
          if (i32value < 0 || i32value > cpf[hnd].interest_point_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nInterest point value %f too large."), cpf[hnd].path, err_recnum,
                       record->channel[i][j].interest_point);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].interest_point_bits, i32value);
          bpos += cpf[hnd].interest_point_bits;


          /*  [CPF:9-9]  Interest point rank.  */

          /*  Make sure we only use one or zero (even if this is a pre v3.00 file, since the ip_rank was only ever tested as a boolean).  */

          if (record->channel[i][j].ip_rank) record->channel[i][j].ip_rank = 1;

          czmil_bit_pack (buffer, bpos, cpf[hnd].ip_rank_bits, record->channel[i][j].ip_rank);
          bpos += cpf[hnd].ip_rank_bits;
        }
    }


  /*  [CPF:10]  Loop through the 7 shallow channels and store the bare earth values.  */

  for (i = 0 ; i < 7 ; i++)
    {
      /*  Check for a null z value in the elevation.  If it's there we're going to force the latitude and longitude to be
          the same as the reference latitude and longitude.  The reason that we do that is that the application may have put
          a 0.0 or some undefined value in the latitude and longitude and that would cause the stored integer value to exceed
          the maximum allowable.  */

      if (record->bare_earth_elevation[i] == cpf[hnd].header.null_z_value)
        {
          lat = ref_lat;
          lon = ref_lon;
          bare_earth_null_z = 1;
        }
      else
        {
          /*  Bias the latitude and longitude.  */

          lat = record->bare_earth_latitude[i] + 90.0;
          lon = record->bare_earth_longitude[i] + 180.0;
          bare_earth_null_z = 0;
        }


      /*  [CPF:10-0]  Bare earth latitude.  */

      i32value = NINT ((lat - ref_lat) * cpf[hnd].lat_diff_scale) + cpf[hnd].lat_diff_offset;
      if (i32value < 0 || i32value > cpf[hnd].lat_diff_max)
        {
          sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth latitude[%d] %f out of range."), cpf[hnd].path, err_recnum, i,
                   record->bare_earth_latitude[i]);
          return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
        }
      czmil_bit_pack (buffer, bpos, cpf[hnd].lat_diff_bits, i32value);
      bpos += cpf[hnd].lat_diff_bits;


      /*  [CPF:10-1]  Bare earth longitude.  */

      i32value = NINT ((lon - ref_lon) * cpf[hnd].lon_diff_scale * cos_array[lat_band]) + cpf[hnd].lon_diff_offset;
      if (i32value < 0 || i32value > cpf[hnd].lon_diff_max)
        {
          sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth longitude[%d] %f out of range."), cpf[hnd].path, err_recnum, i,
                   record->bare_earth_longitude[i]);
          return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
        }
      czmil_bit_pack (buffer, bpos, cpf[hnd].lon_diff_bits, i32value);
      bpos += cpf[hnd].lon_diff_bits;


      /*  [CPF:10-2]  Bare earth elevation.  */

      if (bare_earth_null_z)
        {
          i32value = cpf[hnd].elev_max;
        }
      else
        {
          i32value = NINT (record->bare_earth_elevation[i] * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
          if (i32value < 0 || i32value > cpf[hnd].elev_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth elevation[%d] %f out of range."), cpf[hnd].path, err_recnum,
                       i, record->bare_earth_elevation[i]);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
        }
      czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
      bpos += cpf[hnd].elev_bits;
    }


  /*  [CPF:11]  Kd value (min is always 0.0, max is always 1.0)  */

  i32value = NINT (record->kd * cpf[hnd].kd_scale);
  if (record->kd < 0.0 || record->kd > 1.0)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nKd value %f out of range."), cpf[hnd].path, err_recnum, record->kd);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].kd_bits, i32value);
  bpos += cpf[hnd].kd_bits;


  /*  [CPF:12]  Laser energy.  */

  i32value = NINT (record->laser_energy * cpf[hnd].laser_energy_scale);
  if (i32value > cpf[hnd].laser_energy_max) i32value = cpf[hnd].laser_energy_max;
  czmil_bit_pack (buffer, bpos, cpf[hnd].laser_energy_bits, i32value);
  bpos += cpf[hnd].laser_energy_bits;


  /*  [CPF:13]  T0 interest point.  */

  i32value = NINT (record->t0_interest_point * cpf[hnd].interest_point_scale);
  if (i32value < 0 || i32value > cpf[hnd].interest_point_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nT0 interest point value %f too large."), cpf[hnd].path, err_recnum,
               record->t0_interest_point);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].interest_point_bits, i32value);
  bpos += cpf[hnd].interest_point_bits;


  /****************************************** VERSION CHECK ******************************************

      The probability, filter_reason, and optech_classification fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 2)
    {
      /*  [CPF:14]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  [CPF:14-0]  Optech waveform processing mode.  */

          if (record->optech_classification[i] > cpf[hnd].optech_classification_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nOptech classification value %d too large."), cpf[hnd].path, err_recnum,
                       record->optech_classification[i]);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].optech_classification_bits, record->optech_classification[i]);
          bpos += cpf[hnd].optech_classification_bits;


          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:14-1]  Probability of detection.  */

              i32value = NINT (record->channel[i][j].probability * cpf[hnd].probability_scale);
              if (i32value < 0 || i32value > cpf[hnd].probability_max)
                {
                  sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d probability %f out of range."), cpf[hnd].path,
                           err_recnum, i, j, record->channel[i][j].probability);
                  return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
                }
              czmil_bit_pack (buffer, bpos, cpf[hnd].probability_bits, i32value);
              bpos += cpf[hnd].probability_bits;


              /*  [CPF:14-2]  Per return filter reason.  */

              if (record->channel[i][j].filter_reason < 0 || record->channel[i][j].filter_reason > cpf[hnd].return_filter_reason_max)
                {
                  sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReturn filter reason value %d too large."), cpf[hnd].path, err_recnum,
                           record->channel[i][j].filter_reason);
                  return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
                }
              czmil_bit_pack (buffer, bpos, cpf[hnd].return_filter_reason_bits, record->channel[i][j].filter_reason);
              bpos += cpf[hnd].return_filter_reason_bits;
            }
        }
    }


  /****************************************** VERSION CHECK ******************************************

      The d_index_cube and d_index fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 3)
    {
      /*  [CPF:15]  d_index_cube.  */

      if (record->d_index_cube > cpf[hnd].d_index_cube_max) record->d_index_cube = 0;
      czmil_bit_pack (buffer, bpos, cpf[hnd].d_index_cube_bits, record->d_index_cube);
      bpos += cpf[hnd].d_index_cube_bits;


      /*  [CPF:16]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {

          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:16-0]  d_index.  */

              if (record->channel[i][j].d_index > cpf[hnd].d_index_max) record->channel[i][j].d_index = 0;
              czmil_bit_pack (buffer, bpos, cpf[hnd].d_index_bits, record->channel[i][j].d_index);
              bpos += cpf[hnd].d_index_bits;
            }
        }
    }


  /*  Pack in the buffer size.  */

  size = bpos / 8;
  if (bpos % 8) size++;

  czmil_bit_pack (buffer, 0, cpf[hnd].buffer_size_bytes * 8, size);


  /*  If we're updating a record (as opposed to adding a new record to a new file) we actually write it out.  */

  if (recnum != CZMIL_NEXT_RECORD)
    {
      /*  Make sure the buffer size we're going to write hasn't changed (it shouldn't).  */

      if (size != cif_record.cpf_buffer_size)
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nBuffer size from the CIF file doesn't match update buffer size.\n"),
                   cpf[hnd].path, err_recnum);
          return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
        }


      /*  Write the buffer to the file.  */

      if (!fwrite (cpf[hnd].buffer, size, 1, cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CPF data :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_ERROR);
        }


      cpf[hnd].pos += size;
      cpf[hnd].modified = 1;
      cpf[hnd].write = 1;
    }


  /*  If we're appending a record we pack it into the block buffer and write it when it gets close to full.  */

  else
    {
      /*  Now update the I/O buffer address so we can be ready for the next record.  */

      cpf[hnd].io_buffer_address += size;


      /*  Since we're creating a CPF file (from a CWF file) we need to read the pre-existing CIF record from the *.cwi file,
          update it with the CPF information, and write it to the new CIF (*.cif.tmp) file.  */

      if (czmil_read_cif_record (cwf[hnd].cif_hnd, err_recnum, &cif_record)) return (czmil_error.czmil);


      /*  Update the apparent file position and use it to modify the CIF record then write it to the CIF file.  */

      cwf[hnd].cif.record.cpf_address = cpf[hnd].create_file_pos;
      cwf[hnd].cif.record.cpf_buffer_size = size;
      cwf[hnd].cif.record.cwf_address = cif_record.cwf_address;
      cwf[hnd].cif.record.cwf_buffer_size = cif_record.cwf_buffer_size;
      cpf[hnd].create_file_pos += size;

      if (czmil_write_cif_record (&cwf[hnd].cif) < 0) return (czmil_error.czmil);
    }


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_cpf_record

 - Purpose:     Updates the user modifiable fields of a CZMIL CPF record without affecting
                the "non-modifiable" fields.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be written
                - record         =    The CZMIL_CPF_Data structure to be updated

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CPF_WRITE_FSEEK_ERROR
                - CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR
                - CZMIL_CPF_WRITE_ERROR
                - CZMIL_CPF_READ_ERROR
                - CZMIL_CPF_CIF_BUFFER_SIZE_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CPF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

 - <b>IMPORTANT NOTE: When using this function to update a record, only certain fields will be
   modified.  This is to prevent aliasing of the parts of the record that shouldn't be changed.
   Since we're compressing the record there is some loss of precision in floating point values 
   when writing to the bit-packed buffer.  If we do that over and over again a certain amount of
   "creep" occurs.  After creation we never want to change the positions, elevations, or off nadir
   angle.  In order to avoid that we are reading the bit-packed buffer and only replacing the
   modifiable fields.  To see what fields are modifiable check the CZMIL_CPF_Data structure
   definition in czmil.h.</b>

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record)
{
  double ref_lat, ref_lon, lat, lon;
  int32_t i, j, bpos, size, i32value, lat_band;
  CZMIL_CIF_Data cif_record;
  uint8_t *buffer;
  uint8_t bare_earth_null_z = 0;


  /*  Check for record out of bounds.  */

  if (recnum >= cpf[hnd].header.number_of_records || recnum < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cpf[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Get the CPF record byte address from the CIF index file.  */

  if (czmil_read_cif_record (cpf[hnd].cif_hnd, recnum, &cif_record)) return (czmil_error.czmil);


  /*  We only want to do the fseek (which flushes the buffer) if our last operation was a read or if we aren't in the correct position.  */

  if (!cpf[hnd].write || cif_record.cpf_address != cpf[hnd].pos)
    {
      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
        }


      /*  Set the new position since we fseeked.  */

      cpf[hnd].pos = cif_record.cpf_address;


      /*  Force it to do the subsequent read if we did an fseek.  */

      cpf[hnd].last_record_read = -1;
    }


  /*  Check to see if we just read this record so we can avoid a reread.  */

  if (recnum != cpf[hnd].last_record_read)
    {
      /*  The normal procedure for doing update type operations is to read the record in the external program, modify it, and then write
          the updated information via czmil_update_cpf_record, czmil_update_cpf_return_status, or (on very rare occasions)
          czmil_write_cpf_record.  In these cases we can either skip the read or we can read from the already existing I/O buffer.
          Unfortunately, some external programs will read a large number of sequential records into memory, modify them, and then use
          one of the above mentioned functions to update the record.  In that case this function has been called a number of times
          in sequential order.  The last thing we do in this function is to write a record so we have "write" information in the I/O
          buffer (we also set the "write" flag).  To avoid confusion we need to flush the buffer prior to the subsequent read operation.
          This actually only showed up in Windows, somehow Linux was flushing the buffer properly.  Just to be safe though we'll do the
          fflush in both cases.  */

      if (cpf[hnd].write) fflush (cpf[hnd].fp);


      if (!fread (cpf[hnd].buffer, cif_record.cpf_buffer_size, 1, cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CPF record prior to write :\n%s\n"), cpf[hnd].path, recnum,
                   strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_READ_ERROR);
        }


      cpf[hnd].last_record_read = recnum;


     /*  Make sure the buffer size read from the CPF file matches the buffer size read from the CIF file.  */

      bpos = 0;
      size = czmil_bit_unpack (cpf[hnd].buffer, 0, cpf[hnd].buffer_size_bytes * 8);
      bpos += cpf[hnd].buffer_size_bytes * 8;

      if (size != cif_record.cpf_buffer_size)
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nBuffer sizes from the CIF and CPF files do not match.\n"), cpf[hnd].path, recnum);
          return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
        }


      /*  Position back to where we're going to write the record.  */

      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
        }
    }


  cpf[hnd].at_end = 0;


  /*  Set the buffer pointer to the single record buffer in the internal CPF structure.  */

  buffer = cpf[hnd].buffer;


  /*  [CPF:0]  Pack the record while skipping to the relevant fields.  To make it easier to see what is being skipped,
      the bit position within the record is being incremented per field instead of all at once.  */

  bpos = cpf[hnd].buffer_size_bytes * 8;


  /*  [CPF:1]  Number of returns per channel.  */

  for (i = 0 ; i < 9 ; i++) bpos += cpf[hnd].return_bits;


  /*  [CPF:2]  Timestamp.  */

  bpos += cpf[hnd].time_bits;


  /*  [CPF:3]  Off nadir angle.  */

  bpos += cpf[hnd].off_nadir_angle_bits;


  /*  [CPF:4]  Reference latitude and longitude.  We have to break these out on the off chance that we are updating bare earth positions.
      Note that base_lat and base_lon are already offset by 90 and 180 respectively.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lat_bits);
  bpos += cpf[hnd].lat_bits;
  ref_lat = (double) (i32value - cpf[hnd].lat_offset) / cpf[hnd].lat_scale + cpf[hnd].header.base_lat;


  /*  Compute the latitude band to index into the cosine array for longitudes.  */

  lat_band = (int32_t) ref_lat;


  /*  [CPF:5]  Reference longitude.  */

  i32value = czmil_bit_unpack (cpf[hnd].buffer, bpos, cpf[hnd].lon_bits);
  bpos += cpf[hnd].lon_bits;
  ref_lon = (double) (i32value - cpf[hnd].lon_offset) / cos_array[lat_band] / cpf[hnd].lon_scale + cpf[hnd].header.base_lon;


  /*  [CPF:6]  Water level elevation.  */

  bpos += cpf[hnd].elev_bits;


  /*  [CPF:7]  Local vertical datum offset (elevation).  */

  i32value = NINT (record->local_vertical_datum_offset * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
  if (i32value < 0 || i32value > cpf[hnd].elev_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nLocal vertical datum offset %f out of range."), cpf[hnd].path, recnum, 
               record->local_vertical_datum_offset);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
  bpos += cpf[hnd].elev_bits;


  /*  [CPF:8]  User data (this used to be spare in v2 and shot status in v1 but it was never used).  */

  czmil_bit_pack (buffer, bpos, cpf[hnd].user_data_bits, record->user_data);
  bpos += cpf[hnd].user_data_bits;


  /*  [CPF:9]  Loop through all nine channels.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  If returns are present...  */

      for (j = 0 ; j < record->returns[i] ; j++)
        {
          /*  [CPF:9-0]  Return latitude.  */

          bpos += cpf[hnd].lat_diff_bits;


          /*  [CPF:9-1]  Return longitude.  */

          bpos += cpf[hnd].lon_diff_bits;


          /*  [CPF:9-2]  Return elevation.  */

          bpos += cpf[hnd].elev_bits;


          /*  [CPF:9-3]  Reflectance.  */

          i32value = NINT (record->channel[i][j].reflectance * cpf[hnd].reflectance_scale);
          if (i32value < 0 || i32value > cpf[hnd].reflectance_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d reflectance %f out of range."), cpf[hnd].path,
                       recnum, i, j, record->channel[i][j].reflectance);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].reflectance_bits, i32value);
          bpos += cpf[hnd].reflectance_bits;


          /*  [CPF:9-4]  Horizontal uncertainty.  */

          i32value = NINT (record->channel[i][j].horizontal_uncertainty * cpf[hnd].uncert_scale);
          if (i32value > cpf[hnd].uncert_max) i32value = cpf[hnd].uncert_max;
          czmil_bit_pack (buffer, bpos, cpf[hnd].uncert_bits, i32value);
          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-5]  Vertical uncertainty.  */

          i32value = NINT (record->channel[i][j].vertical_uncertainty * cpf[hnd].uncert_scale);
          if (i32value > cpf[hnd].uncert_max) i32value = cpf[hnd].uncert_max;
          czmil_bit_pack (buffer, bpos, cpf[hnd].uncert_bits, i32value);
          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-6]  Per return status.  */

          if (record->channel[i][j].status > cpf[hnd].return_status_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReturn status value %d too large."), cpf[hnd].path, recnum,
                       record->channel[i][j].status);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].return_status_bits, record->channel[i][j].status);
          bpos += cpf[hnd].return_status_bits;


          /*  [CPF:9-7]  Per return classification.  */

          if (record->channel[i][j].classification > cpf[hnd].class_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nClassification value %d too large."), cpf[hnd].path, recnum,
                       record->channel[i][j].classification);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].class_bits, record->channel[i][j].classification);
          bpos += cpf[hnd].class_bits; 


          /*  [CPF:9-8]  Interest point.  */

          bpos += cpf[hnd].interest_point_bits;


          /*  [CPF:9-9]  Interest point rank.  */

          bpos += cpf[hnd].ip_rank_bits;
        }
    }


  /*  [CPF:10]  Loop through the 7 shallow channels and store the bare earth values.  */

  for (i = 0 ; i < 7 ; i++)
    {
      /*  Check for a null z value in the elevation.  If it's there we're going to force the latitude and longitude to be
          the same as the reference latitude and longitude.  The reason that we do that is that the application may have put
          a 0.0 or some undefined value in the latitude and longitude and that would cause the stored integer value to exceed
          the maximum allowable.  */

      if (record->bare_earth_elevation[i] == cpf[hnd].header.null_z_value)
        {
          lat = ref_lat;
          lon = ref_lon;
          bare_earth_null_z = 1;
        }
      else
        {
          /*  Bias the latitude and longitude.  */

          lat = record->bare_earth_latitude[i] + 90.0;
          lon = record->bare_earth_longitude[i] + 180.0;
          bare_earth_null_z = 0;
        }


      /*  [CPF:10-0]  Bare earth latitude.  */

      i32value = NINT ((lat - ref_lat) * cpf[hnd].lat_diff_scale) + cpf[hnd].lat_diff_offset;
      if (i32value < 0 || i32value > cpf[hnd].lat_diff_max)
        {
          sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth latitude[%d] %f out of range."), cpf[hnd].path, recnum, i,
                   record->bare_earth_latitude[i]);
          return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
        }
      czmil_bit_pack (buffer, bpos, cpf[hnd].lat_diff_bits, i32value);
      bpos += cpf[hnd].lat_diff_bits;


      /*  [CPF:10-1]  Bare earth longitude.  */

      i32value = NINT ((lon - ref_lon) * cpf[hnd].lon_diff_scale * cos_array[lat_band]) + cpf[hnd].lon_diff_offset;
      if (i32value < 0 || i32value > cpf[hnd].lon_diff_max)
        {
          sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth longitude[%d] %f out of range."), cpf[hnd].path, recnum, i,
                   record->bare_earth_longitude[i]);
          return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
        }
      czmil_bit_pack (buffer, bpos, cpf[hnd].lon_diff_bits, i32value);
      bpos += cpf[hnd].lon_diff_bits;


      /*  [CPF:10-2]  Bare earth elevation.  */

      if (bare_earth_null_z)
        {
          i32value = cpf[hnd].elev_max;
        }
      else
        {
          i32value = NINT (record->bare_earth_elevation[i] * cpf[hnd].elev_scale) + cpf[hnd].elev_offset;
          if (i32value < 0 || i32value > cpf[hnd].elev_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nBare earth elevation[%d] %f out of range."), cpf[hnd].path, recnum,
                       i, record->bare_earth_elevation[i]);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].elev_bits, i32value);
          bpos += cpf[hnd].elev_bits;
        }
    }


  /*  [CPF:11]  Kd value (min is always 0.0, max is always 1.0)  */

  i32value = NINT (record->kd * cpf[hnd].kd_scale);
  if (record->kd < 0.0 || record->kd > 1.0)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nKd value %f out of range."), cpf[hnd].path, recnum, record->kd);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].kd_bits, i32value);
  bpos += cpf[hnd].kd_bits;


  /*  [CPF:12]  Laser energy.  */

  i32value = NINT (record->laser_energy * cpf[hnd].laser_energy_scale);
  if (i32value > cpf[hnd].laser_energy_max) i32value = cpf[hnd].laser_energy_max;
  czmil_bit_pack (buffer, bpos, cpf[hnd].laser_energy_bits, i32value);
  bpos += cpf[hnd].laser_energy_bits;


  /*  [CPF:13]  T0 interest point.  */

  i32value = NINT (record->t0_interest_point * cpf[hnd].interest_point_scale);
  if (i32value < 0 || i32value > cpf[hnd].interest_point_max)
    {
      sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nT0 interest point value %f too large."), cpf[hnd].path, recnum,
               record->t0_interest_point);
      return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, cpf[hnd].interest_point_bits, i32value);
  bpos += cpf[hnd].interest_point_bits;


  /****************************************** VERSION CHECK ******************************************

      The probability, filter_reason, and optech_classification fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 2)
    {
      /*  [CPF:14]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  [CPF:14-0]  Optech waveform processing mode.  */

          if (record->optech_classification[i] > cpf[hnd].optech_classification_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nOptech classification value %d too large."), cpf[hnd].path, recnum,
                       record->optech_classification[i]);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].optech_classification_bits, record->optech_classification[i]);
          bpos += cpf[hnd].optech_classification_bits;


          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:14-1]  Probability of detection.  */

              i32value = NINT (record->channel[i][j].probability * cpf[hnd].probability_scale);
              if (i32value < 0 || i32value > cpf[hnd].probability_max)
                {
                  sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nChannel %d return %d probability %f out of range."), cpf[hnd].path,
                           recnum, i, j, record->channel[i][j].probability);
                  return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
                }
              czmil_bit_pack (buffer, bpos, cpf[hnd].probability_bits, i32value);
              bpos += cpf[hnd].probability_bits;


              /*  [CPF:14-2]  Per return filter reason.  */

              if (record->channel[i][j].filter_reason < 0 || record->channel[i][j].filter_reason > cpf[hnd].return_filter_reason_max)
                {
                  sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReturn filter reason value %d too large."), cpf[hnd].path, recnum,
                           record->channel[i][j].filter_reason);
                  return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
                }
              czmil_bit_pack (buffer, bpos, cpf[hnd].return_filter_reason_bits, record->channel[i][j].filter_reason);
              bpos += cpf[hnd].return_filter_reason_bits;
            }
        }
    }


  /****************************************** VERSION CHECK ******************************************

      The d_index_cube and d_index fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 3)
    {
      /*  [CPF:15]  d_index_cube.  */

      bpos += cpf[hnd].d_index_cube_bits;


      /*  [CPF:16]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:16-0]  d_index.  */

              bpos += cpf[hnd].d_index_bits;
            }
        }
    }


  /*  Compute the size (even if we read it from the CIF file).  */

  size = bpos / 8;
  if (bpos % 8) size++;


  /*  Make sure the buffer size read from the CIF file matches the buffer size we're going to write to the CPF file.  This is just a sanity
      check.  If it happens, something is terribly wrong.  */

  if (size != cif_record.cpf_buffer_size)
    {
      sprintf (czmil_error.info,
               _("File : %s\nRecord : %d\nBuffer sizes from CIF (%d) and CPF (%d) files don't match.\nYou should delete the CIF file and let it be regenerated.\n"),
               cpf[hnd].path, recnum, cif_record.cpf_buffer_size, size);
      return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
    }


  /*  Write the buffer to the file.  */

  if (!fwrite (cpf[hnd].buffer, size, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CPF data :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_WRITE_ERROR);
    }


  cpf[hnd].pos += size;
  cpf[hnd].modified = 1;
  cpf[hnd].write = 1;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_cpf_return_status

 - Purpose:     Updates <b>ONLY</b> the return status, the classification, the user_data
                field, and the filter_reason of a CZMIL CPF record without affecting any of
                the other fields.  This will be slightly faster than using
                czmil_update_cpf_record if you are just updating the return status, the
                classification, the user_data field, and/or the filter_reason (things that
                are done fairly often).  It also won't bias the non-integer fields since they
                aren't touched.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/30/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be written
                - record         =    The CZMIL CPF record structure (only the status and/or
                                      classification fields have to be valid).

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CPF_WRITE_FSEEK_ERROR
                - CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR
                - CZMIL_CPF_WRITE_ERROR
                - CZMIL_CPF_READ_ERROR
                - CZMIL_CPF_CIF_BUFFER_SIZE_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CPF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

 - <b>IMPORTANT NOTE: When using this function to update a record, only the status,
   classification, and/or user_data fields will be modified.  This is to prevent aliasing of
   the parts of the record that shouldn't be changed.  Since we're compressing the record
   there is some loss of precision floating point values when writing to the bit-packed
   buffer.  If we do that over and over again a certain amount of "creep" occurs.  After
   creation we never want to change the positions, elevations, timestamp, or off nadir
   angle.  In order to avoid that we are reading the bit-packed buffer and only replacing
   the status, classification, and user_data fields.</b>

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_cpf_return_status (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record)
{
  int32_t i, j, bpos, size;
  CZMIL_CIF_Data cif_record;
  uint8_t *buffer;


  /*  Check for record out of bounds.  */

  if (recnum >= cpf[hnd].header.number_of_records || recnum < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cpf[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Get the CPF record byte address from the CIF index file.  */

  if (czmil_read_cif_record (cpf[hnd].cif_hnd, recnum, &cif_record)) return (czmil_error.czmil);


  /*  We only want to do the fseek (which flushes the buffer) if our last operation was a read or if we aren't in the correct position.  */

  if (!cpf[hnd].write || cif_record.cpf_address != cpf[hnd].pos)
    {
      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
        }


      /*  Set the new position since we fseeked.  */

      cpf[hnd].pos = cif_record.cpf_address;


      /*  Force it to do the subsequent read if we did an fseek.  */

      cpf[hnd].last_record_read = -1;
    }


  /*  Check to see if we just read this record so we can avoid a reread.  */

  if (recnum != cpf[hnd].last_record_read)
    {
      /*  The normal procedure for doing update type operations is to read the record in the external program, modify it, and then write
          the updated information via czmil_update_cpf_record, czmil_update_cpf_return_status, or (on very rare occasions)
          czmil_write_cpf_record.  In these cases we can either skip the read or we can read from the already existing I/O buffer.
          Unfortunately, some external programs will read a large number of sequential records into memory, modify them, and then use
          one of the above mentioned functions to update the record.  In that case this function has been called a number of times
          in sequential order.  The last thing we do in this function is to write a record so we have "write" information in the I/O
          buffer (we also set the "write" flag).  To avoid confusion we need to flush the buffer prior to the subsequent read operation.
          This actually only showed up in Windows, somehow Linux was flushing the buffer properly.  Just to be safe though we'll do the
          fflush in both cases.  */

      if (cpf[hnd].write) fflush (cpf[hnd].fp);


      if (!fread (cpf[hnd].buffer, cif_record.cpf_buffer_size, 1, cpf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CPF record prior to write :\n%s\n"), cpf[hnd].path, recnum,
                   strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_READ_ERROR);
        }


      cpf[hnd].last_record_read = recnum;


      /*  Make sure the buffer size read from the CPF file matches the buffer size read from the CIF file.  */

      bpos = 0;
      size = czmil_bit_unpack (cpf[hnd].buffer, 0, cpf[hnd].buffer_size_bytes * 8);
      bpos += cpf[hnd].buffer_size_bytes * 8;

      if (size != cif_record.cpf_buffer_size)
        {
          fprintf(stderr,"%s %s %d %d %d\n",__FILE__,__FUNCTION__,__LINE__,size,cif_record.cpf_buffer_size);
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nBuffer sizes from the CIF and CPF files do not match.\n"), cpf[hnd].path, recnum);
          return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
        }


      /*  Position back to where we're going to write the record.  */

      if (fseeko64 (cpf[hnd].fp, cif_record.cpf_address, SEEK_SET) < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CPF record :\n%s\n"), cpf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CPF_WRITE_FSEEK_ERROR);
        }
    }


  cpf[hnd].at_end = 0;


  /*  Set the buffer pointer to the single record buffer in the internal CPF structure.  */

  buffer = cpf[hnd].buffer;


  /*  [CPF:0]  Pack the record while skipping to the relevant fields.  To make it easier to see what is being skipped,
      the bit position within the record is being incremented per field instead of all at once.  */

  bpos = cpf[hnd].buffer_size_bytes * 8;


  /*  [CPF:1]  Number of returns per channel.  */

  for (i = 0 ; i < 9 ; i++) bpos += cpf[hnd].return_bits;


  /*  [CPF:2]  Timestamp.  */

  bpos += cpf[hnd].time_bits;


  /*  [CPF:3]  Off nadir angle.  */

  bpos += cpf[hnd].off_nadir_angle_bits;


  /*  [CPF:4]  Reference latitude.  */

  bpos += cpf[hnd].lat_bits;


  /*  [CPF:5]  Reference longitude.  */

  bpos += cpf[hnd].lon_bits;


  /*  [CPF:6]  Water level elevation.  */

  bpos += cpf[hnd].elev_bits;


  /*  [CPF:7]  Local vertical datum offset (elevation).  */

  bpos += cpf[hnd].elev_bits;


  /*  [CPF:8]  User data (this used to be spare in v2 and shot status in v1 but it was never used).  */

  czmil_bit_pack (buffer, bpos, cpf[hnd].user_data_bits, record->user_data);
  bpos += cpf[hnd].user_data_bits;


  /*  [CPF:9]  Loop through all nine channels.  */

  for (i = 0 ; i < 9 ; i++)
    {
      /*  If returns are present...  */

      for (j = 0 ; j < record->returns[i] ; j++)
        {
          /*  [CPF:9-0]  Return latitude.  */

          bpos += cpf[hnd].lat_diff_bits;


          /*  [CPF:9-1]  Return longitude.  */

          bpos += cpf[hnd].lon_diff_bits;


          /*  [CPF:9-2]  Return elevation.  */

          bpos += cpf[hnd].elev_bits;


          /*  [CPF:9-3]  Reflectance.  */

          bpos += cpf[hnd].reflectance_bits;


          /*  [CPF:9-4]  Horizontal uncertainty.  */

          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-5]  Vertical uncertainty.  */

          bpos += cpf[hnd].uncert_bits;


          /*  [CPF:9-6]  Per return status.  */

          if (record->channel[i][j].status > cpf[hnd].return_status_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nReturn status value %d too large."), cpf[hnd].path, recnum,
                       record->channel[i][j].status);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].return_status_bits, record->channel[i][j].status);
          bpos += cpf[hnd].return_status_bits;


          /*  [CPF:9-7]  Per return classification.  */

          if (record->channel[i][j].classification > cpf[hnd].class_max)
            {
              sprintf (czmil_error.info, _("In CPF file %s, Record %d :\nClassification value %d too large."), cpf[hnd].path, recnum,
                       record->channel[i][j].classification);
              return (czmil_error.czmil = CZMIL_CPF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, cpf[hnd].class_bits, record->channel[i][j].classification);
          bpos += cpf[hnd].class_bits; 


          /*  [CPF:9-8]  Interest point.  */

          bpos += cpf[hnd].interest_point_bits;


          /*  [CPF:9-9]  Interest point rank.  */

          bpos += cpf[hnd].ip_rank_bits;
        }
    }


  /*  [CPF:10]  Loop through the 7 shallow channels and bypass the bare earth values.  */

  for (i = 0 ; i < 7 ; i++)
    {
      /*  [CPF:10-0]  Bare earth latitude.  */

      bpos += cpf[hnd].lat_diff_bits;


      /*  [CPF:10-1]  Bare earth longitude.  */

      bpos += cpf[hnd].lon_diff_bits;


      /*  [CPF:10-2]  Bare earth elevation.  */

      bpos += cpf[hnd].elev_bits;
    }


  /*  [CPF:11]  Kd value.  */

  bpos += cpf[hnd].kd_bits;


  /*  [CPF:12]  Laser energy.  */

  bpos += cpf[hnd].laser_energy_bits;


  /*  [CPF:13]  T0 interest point.  */

  bpos += cpf[hnd].interest_point_bits;


  /****************************************** VERSION CHECK ******************************************

      The probability, filter_reason, and optech_classification fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 2)
    {
      /*  [CPF:14]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  [CPF:14-0]  Optech waveform processing mode.  */

          bpos += cpf[hnd].optech_classification_bits;


          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:14-1]  Probability of detection.  */

              bpos += cpf[hnd].probability_bits;


              /*  [CPF:14-2]  Per return filter reason.  */

              czmil_bit_pack (buffer, bpos, cpf[hnd].return_filter_reason_bits, record->channel[i][j].filter_reason);
              bpos += cpf[hnd].return_filter_reason_bits;
            }
        }
    }


  /****************************************** VERSION CHECK ******************************************

      The d_index_cube and d_index fields did not exist prior to major version 3.

  ***************************************************************************************************/

  if (cpf[hnd].major_version >= 3)
    {
      /*  [CPF:15]  d_index_cube.  */

      bpos += cpf[hnd].d_index_cube_bits;


      /*  [CPF:16]  Loop through all nine channels.  */

      for (i = 0 ; i < 9 ; i++)
        {
          /*  If returns are present...  */

          for (j = 0 ; j < record->returns[i] ; j++)
            {
              /*  [CPF:16-0]  d_index.  */

              bpos += cpf[hnd].d_index_bits;
            }
        }
    }


  /*  Compute the size (even if we read it from the CIF file).  */

  size = bpos / 8;
  if (bpos % 8) size++;


  /*  Make sure the buffer size read from the CIF file matches the buffer size we're going to write to the CPF file.  This is just a sanity
      check.  If it happens, something is terribly wrong.  */

  if (size != cif_record.cpf_buffer_size)
    {
      sprintf (czmil_error.info,
               _("File : %s\nRecord : %d\nBuffer sizes from CIF (%d) and CPF (%d) files don't match.\nYou should delete the CIF file and let it be regenerated.\n"),
               cpf[hnd].path, recnum, cif_record.cpf_buffer_size, size);
      return (czmil_error.czmil = CZMIL_CPF_CIF_BUFFER_SIZE_ERROR);
    }


  /*  Write the buffer to the file.  */

  if (!fwrite (cpf[hnd].buffer, size, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError writing CPF data :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_WRITE_ERROR);
    }


  cpf[hnd].pos += size;
  cpf[hnd].modified = 1;
  cpf[hnd].write = 1;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_read_csf_record_array

 - Purpose:     Retrieve CZMIL CSF records and fill the supplied array of CSF records.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the first CSF record to be retrieved
                - num_requested  =    The number of CSF records requested.
                - record_array   =    The pointer to the array of CSF records that will be populated

 - Returns:
                - The number of records filled or...
                - Error value returned from czmil_read_csf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_csf_record_array (int32_t hnd, int32_t recnum, int32_t num_requested, CZMIL_CSF_Data *record_array)
{
  int32_t i, num_read = 0, recs = 0;


  /*  Make sure we don't try to read past the end of the file.  */

  recs = MIN (recnum + num_requested, csf[hnd].header.number_of_records) - recnum;


  /*  Loop through the requested number of records (or up to the end of the file).  */

  for (i = 0 ; i < recs ; i++)
    {
      if (czmil_read_csf_record (hnd, recnum + i, &record_array[i]) < 0) return (czmil_error.czmil);

      num_read++;
    }


  /*  Return the number of records read (since it may not be the same as the number requested if we
      bumped up against the end of file).  */

  return (num_read);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_read_csf_record

 - Purpose:     Retrieve a CZMIL CSF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be retrieved or
                                      CZMIL_NEXT_RECORD if you are reading sequentially.
                - record         =    The returned CZMIL CSF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CSF_READ_FSEEK_ERROR
                - CZMIL_CSF_READ_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CSF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_csf_record (int32_t hnd, int32_t recnum, CZMIL_CSF_Data *record)
{
  int64_t address;
  int32_t i, bpos, i32value, lat_band;
  double lat, lon;


  /*  The actual buffer will never be sizeof (CZMIL_CSF_Data) in size since we are unpacking it but this way
      we don't have to worry about blowing this up.  Also, we don't allocate the memory because memory
      allocation invokes a system wide mutex.  */

  uint8_t buffer[sizeof (CZMIL_CSF_Data)];


  /*  Check for record out of bounds.  */

  if (recnum >= csf[hnd].header.number_of_records || recnum < CZMIL_NEXT_RECORD)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), csf[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  We only need to seek the record if we're not reading sequentially.  */

  if (recnum != CZMIL_NEXT_RECORD)
    {
      /*  Compute the CSF record byte address.  */

      address = (int64_t) recnum * (int64_t) csf[hnd].buffer_size + (int64_t) csf[hnd].header.header_size;


      /*  We only want to do the fseek (which flushes the buffer) if our last operation was a write or if we aren't in the
          correct position.  */

      if (csf[hnd].write || address != csf[hnd].pos)
        {
          if (fseeko64 (csf[hnd].fp, address, SEEK_SET) < 0)
            {
              sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF record :\n%s\n"), csf[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CSF_READ_FSEEK_ERROR);
            }


          /*  Set the new position since we fseeked.  */

          csf[hnd].pos = address;
        }
    }


  csf[hnd].at_end = 0;


  if (!fread (buffer, csf[hnd].buffer_size, 1, csf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nError reading CSF record :\n%s\n"), csf[hnd].path, recnum, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_READ_ERROR);
    }


  bpos = 0;


  /*  [CSF:0]  Timestamp.  */
  
  record->timestamp = csf[hnd].header.flight_start_timestamp + (uint64_t) czmil_bit_unpack (buffer, bpos, csf[hnd].time_bits);
  bpos += csf[hnd].time_bits;


  /*  [CSF:1]  Scan angle.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].scan_angle_bits);
  bpos += csf[hnd].scan_angle_bits;
  record->scan_angle = (float) i32value / csf[hnd].angle_scale;


  /*  [CSF:2]  Latitude, longitude, and elevation.  Note that base_lat and base_lon are already offset by 90 and 180 respectively.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].lat_bits);
  bpos += csf[hnd].lat_bits;
  lat = (double) (i32value - csf[hnd].lat_offset) / csf[hnd].lat_scale + csf[hnd].header.base_lat;
  record->latitude = lat - 90.0;


  /*  Compute the latitude band to index into the cosine array for longitudes.  */

  lat_band = (int32_t) lat;


  /*  [CSF:3]  Longitude.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].lon_bits);
  bpos += csf[hnd].lon_bits;
  lon = (double) (i32value - csf[hnd].lon_offset) / cos_array[lat_band] / csf[hnd].lon_scale + csf[hnd].header.base_lon;
  record->longitude = lon - 180.0;


  /*  [CSF:4]  Altitude.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].alt_bits);
  bpos += csf[hnd].alt_bits;
  record->altitude = (float) (i32value - csf[hnd].alt_offset) / csf[hnd].alt_scale;


  /*  [CSF:5]  Platform roll.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].roll_pitch_bits);
  bpos += csf[hnd].roll_pitch_bits;
  record->roll = (float) (i32value - csf[hnd].roll_pitch_offset) / csf[hnd].angle_scale;


  /*  [CSF:6]  Platform pitch.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].roll_pitch_bits);
  bpos += csf[hnd].roll_pitch_bits;
  record->pitch = (float) (i32value - csf[hnd].roll_pitch_offset) / csf[hnd].angle_scale;


  /*  [CSF:7]  Platform heading.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].heading_bits);
  bpos += csf[hnd].heading_bits;
  record->heading = (float) i32value / csf[hnd].angle_scale;


  /*  [CSF:8]  Ranges.  If value is csf[hnd].range_max, this is an invalid range so we return -1.0  */

  for (i = 0 ; i < 9 ; i++)
    {
      i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].range_bits);
      bpos += csf[hnd].range_bits;

      if (i32value == csf[hnd].range_max)
        {
          record->range[i] = -1.0;
        }
      else
        {
          record->range[i] = (float) (i32value) / csf[hnd].range_scale;
        }
    }


  /****************************************** VERSION CHECK ******************************************

      The range_in_water, intensity, and intensity_in_water fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (csf[hnd].major_version >= 2)
    {
      /*  [CSF:9]  Ranges in water.  If value is csf[hnd].range_max, this is an invalid range so we return -1.0  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].range_bits);
          bpos += csf[hnd].range_bits;

          if (i32value == csf[hnd].range_max)
            {
              record->range_in_water[i] = -1.0;
            }
          else
            {
              record->range_in_water[i] = (float) (i32value) / csf[hnd].range_scale;
            }
        }


      /*  [CSF:10]  Intensities.  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].intensity_bits);
          bpos += csf[hnd].intensity_bits;
          record->intensity[i] = (float) i32value / csf[hnd].intensity_scale;
        }


      /*  [CSF:11]  Intensities in water.  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].intensity_bits);
          bpos += csf[hnd].intensity_bits;
          record->intensity_in_water[i] = (float) i32value / csf[hnd].intensity_scale;
        }
    }
  else
    {
      for (i = 0 ; i < 9 ; i++)
        {
          record->range_in_water[i] = 0.0;
          record->intensity[i] = 0.0;
          record->intensity_in_water[i] = 0.0;
        }
    }


  csf[hnd].pos += csf[hnd].buffer_size;
  csf[hnd].write = 0;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/*********************************************************************************************/
/*!

 - Function:    czmil_write_csf_record_array

 - Purpose:     Appends the supplied arrays of raw waveform records and waveforms to the CWF file.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/19/12

 - Arguments:
                - hnd            =    The file handle
                - num_supplied   =    The number of CSF records to be written.
                - record_array   =    The array of CZMIL_CSF_Data structures

 - Returns:
                - The number of records written (which should be equal to num_supplied) or...
                - Error value returned from czmil_write_csf_record

 - Caveats:     All returned error values are less than zero.  A simple test for failure is to
                check to see if the return is less than zero.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_csf_record_array (int32_t hnd, int32_t num_supplied, CZMIL_CSF_Data *record_array)
{
  int32_t i, num_written = 0;


  /*  Loop through the records and call the record writing function.  */

  for (i = 0 ; i < num_supplied ; i++)
    {
      if (czmil_write_csf_record (hnd, CZMIL_NEXT_RECORD, &record_array[i]) < 0) return (czmil_error.czmil);

      num_written++;
    }


  /*  Return the number of records written.  This should always be the same as num_supplied.  */

  return (num_written);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_csf_record

 - Purpose:     Appends a CZMIL CSF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/19/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The CZMIL CSF record number or CZMIL_NEXT_RECORD to append
                                      to a file that is being created
                - record         =    The CZMIL CSF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_APPEND_ERROR
                - CZMIL_CSF_WRITE_FSEEK_ERROR
                - CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR
                - CZMIL_CSF_WRITE_BUFFER_SIZE_ERROR
                - CZMIL_CSF_WRITE_ERROR
                - CZMIL_CSF_TIME_REGRESSION_ERROR

 - Caveats:     This function is normally used to append a new record to a newly created
                file but may be used to update records during reprocessing of waveforms
                since the range and intensity fields may be modified.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CSF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

                To append to a file that is being created, recnum <b>MUST</b> be set to
                CZMIL_NEXT_RECORD.  To update an already existing file using this function,
                CZMIL_NEXT_RECORD <b>CANNOT</b> be used.

 - <b>IMPORTANT NOTE: When using this function to update a record, any field may be modified.
   This may cause aliasing of some parts of the record.  Since we're compressing the record
   there is some loss of precision when writing to the bit-packed buffer.  If we do that over
   and over again a certain amount of "creep" occurs.  This function should only be used by
   the HydroFusion software to append new records or to replace records that have been 
   re-computed (e.g. range values due to re-processing the waveform).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_csf_record (int32_t hnd, int32_t recnum, CZMIL_CSF_Data *record)
{
  int32_t i, i32value, lat_band, bpos, err_recnum;
  uint32_t ui32value;
  double lat, lon, band_lat;
  uint8_t *buffer;
  int64_t csf_address;


  /*  Check for CZMIL_UPDATE mode.  */

  if (csf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Save the record_number for error messages.  */

  err_recnum = recnum;


  /*  This section is for updating a record in a pre-existing file.  */

  if (recnum != CZMIL_NEXT_RECORD)
    {
      /*  Check for record out of bounds.  */

      if (recnum >= csf[hnd].header.number_of_records || recnum < 0)
        {
          sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), csf[hnd].path, err_recnum);
          return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
        }


      /*  Compute the CSF record byte address from the record number.  */

      csf_address = recnum * csf[hnd].buffer_size + csf[hnd].header.header_size;


      /*  We only want to do the fseek (which flushes the buffer) if our last operation was a read or if we aren't in the correct position.  */

      if (!csf[hnd].write || csf_address != csf[hnd].pos)
        {
          if (fseeko64 (csf[hnd].fp, csf_address, SEEK_SET) < 0)
            {
              sprintf (czmil_error.info, _("File : %s\nError during fseek prior to writing CSF record :\n%s\n"), csf[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CSF_WRITE_FSEEK_ERROR);
            }


          /*  Set the new position since we fseeked.  */

          csf[hnd].pos = csf_address;
        }


      csf[hnd].at_end = 0;


      /*  Set the buffer pointer to the single record buffer in the internal CSF structure.  */

      buffer = csf[hnd].buffer;
    }
  else
    {
      /*  Appending a record is only allowed if you are creating a new file.  */

      if (!csf[hnd].created)
        {
          sprintf (czmil_error.info, _("File : %s\nAppending to pre-existing CSF file not allowed.\n"), csf[hnd].path);
          return (czmil_error.czmil = CZMIL_CSF_APPEND_ERROR);
        }


      /*  If there is ANY chance at all that we might overrun our I/O buffer on this write, we need to 
          flush the buffer.  Since we're compressing our records they should not be anywhere near the
          size of a CZMIL_CSF_Data structure.  But, also since we're compressing the records, the actual size
          of the next record is unknown so we are just making sure that we're not within sizeof (CZMIL_CSF_Data)
          bytes of the end of the output buffer.  That way we're taking no chances of exceeding our output
          buffer size.  */

      if ((csf[hnd].io_buffer_size - csf[hnd].io_buffer_address) < sizeof (CZMIL_CSF_Data))
        {
          if (czmil_flush_csf_io_buffer (hnd) < 0) return (czmil_error.czmil);
        }


      /*  Set the buffer pointer to the correct location within the I/O buffer.  */

      buffer = &csf[hnd].io_buffer[csf[hnd].io_buffer_address];


      csf[hnd].at_end = 1;


      /*  Check for first record so we can set the start timestamp.  */

      if (!csf[hnd].header.flight_end_timestamp)
        {
          csf[hnd].header.flight_start_timestamp = record->timestamp;
        }
      else
        {
          /*  Check for a time regression using the end timestamp that is stored in the header structure each time we add a record.  */

          if (record->timestamp <= csf[hnd].header.flight_end_timestamp)
            {
              sprintf (czmil_error.info, _("File : %s\nRecord : %d\nTime regression encountered during creation of CSF file:\n"), csf[hnd].path,
                       csf[hnd].header.number_of_records);
              return (czmil_error.czmil = CZMIL_CSF_TIME_REGRESSION_ERROR);
            }
        }


      /*  Set the end timestamp so that it will be correct when we close the file.  We also use it for time regression testing above.  */

      csf[hnd].header.flight_end_timestamp = record->timestamp;
    }


  bpos = 0;


  /*  [CSF:0]  Timestamp.  */
  
  if (record->timestamp - csf[hnd].header.flight_start_timestamp > csf[hnd].time_max)
    {
      sprintf (czmil_error.info,
               _("In CSF file %s, Record %d : \nCSF timestamp %"PRIu64" out of range from start timestamp %"PRIu64"."),
               csf[hnd].path, csf[hnd].header.number_of_records, record->timestamp, csf[hnd].header.flight_start_timestamp);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }

  ui32value = (uint32_t) (record->timestamp - csf[hnd].header.flight_start_timestamp);
  czmil_bit_pack (buffer, bpos, csf[hnd].time_bits, ui32value);
  bpos += csf[hnd].time_bits;


  /*  Adjust the scan angle for negatives and greater than 360.0 values.  */

  if (record->scan_angle < 0.0) record->scan_angle += 360.0;
  if (record->scan_angle > 360.0) record->scan_angle -= 360.0;


  /*  [CSF:1]  Scan angle.  */

  i32value = NINT (record->scan_angle * csf[hnd].angle_scale);
  czmil_bit_pack (buffer, bpos, csf[hnd].scan_angle_bits, i32value);
  bpos += csf[hnd].scan_angle_bits;


  /*  [CSF:2]  Latitude, longitude, and altitude.  Note that base_lat and base_lon are already offset by 90 and 180 respectively.  */

  lat = record->latitude + 90.0;
  lon = record->longitude + 180.0;


  /*  Compute the latitude band to index into the cosine array.  */

  lat_band = (int32_t) lat;


  i32value = NINT ((lat - csf[hnd].header.base_lat) * csf[hnd].lat_scale) + csf[hnd].lat_offset;
  if (i32value < 0 || i32value > csf[hnd].lat_max)
    {
      sprintf (czmil_error.info, _("In CSF file %s, Record %d : \nLatitude %f out of range."), csf[hnd].path,
               csf[hnd].header.number_of_records, record->latitude);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, csf[hnd].lat_bits, i32value);


  /*  Compute the latitude band to index into the cosine array for computation of the longitude.  Why are we unpacking what we just 
      packed?  Suppose the latitude that we input was 32.00000001.  If we take the integer latitude (plus 90.0) we get 122, so our
      index into the cosine array is 122.  Unfortunately, the resolution to which we are storing the latitude is 10,000ths of an
      arcsecond.  That is about .000000027 degrees.  When we unpack the latitude value on read it may get reconstituted as 121.999999998.
      In order to avoid that unpleasantness we unpack the latitude value that we just saved and use that to get the index into the
      cosine array so that we will use the correct value when we read the longitude.  See, there was method to my madness.  The little
      bit of wiggle room that we get by using ref_lat and ref_lon prior to packing and unpacking isn't a problem since our resolution
      is approximately 3mm.  We just wanted to make sure that we didn't flip latitude bands in our cosine lookup.
      Note that we don't increment bpos until after we do this.  */

  i32value = czmil_bit_unpack (buffer, bpos, csf[hnd].lat_bits);
  band_lat = (double) (i32value - csf[hnd].lat_offset) / csf[hnd].lat_scale + csf[hnd].header.base_lat;
  lat_band = (int32_t) band_lat;


  bpos += csf[hnd].lat_bits;


  /*  [CSF:3]  Longitude.  */

  i32value = NINT ((lon - csf[hnd].header.base_lon) * csf[hnd].lon_scale * cos_array[lat_band]) + csf[hnd].lon_offset;
  if (i32value < 0 || i32value > csf[hnd].lon_max)
    {
      sprintf (czmil_error.info, _("In CSF file %s, Record %d : \nLongitude %f out of range."), csf[hnd].path,
               csf[hnd].header.number_of_records, record->longitude);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, csf[hnd].lon_bits, i32value);
  bpos += csf[hnd].lon_bits;


  /*  [CSF:4]  Altitude.  */

  i32value = NINT ((record->altitude * csf[hnd].alt_scale) + csf[hnd].alt_offset);
  if (i32value < 0 || i32value > csf[hnd].alt_max)
    {
      sprintf (czmil_error.info, _("In CSF file %s, Record %d : \nCSF altitude %f out of range."), csf[hnd].path,
               csf[hnd].header.number_of_records, record->altitude);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, csf[hnd].alt_bits, i32value);
  bpos += csf[hnd].alt_bits;


  /*  [CSF:5]  Platform roll.  */

  i32value = NINT (record->roll * csf[hnd].angle_scale) + csf[hnd].roll_pitch_offset;
  if (i32value < 0 || i32value > csf[hnd].roll_pitch_max)
    {
      sprintf (czmil_error.info, _("In CSF file %s, Record %d : \nPlatform roll %f out of range."), csf[hnd].path,
               csf[hnd].header.number_of_records, record->roll);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, csf[hnd].roll_pitch_bits, i32value);
  bpos += csf[hnd].roll_pitch_bits;



  /*  [CSF:6]  Platform pitch.  */

  i32value = NINT (record->pitch * csf[hnd].angle_scale) + csf[hnd].roll_pitch_offset;
  if (i32value < 0 || i32value > csf[hnd].roll_pitch_max)
    {
      sprintf (czmil_error.info, _("In CSF file %s, Record %d : \nPlatform pitch %f out of range."), csf[hnd].path,
               csf[hnd].header.number_of_records, record->pitch);
      return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, csf[hnd].roll_pitch_bits, i32value);
  bpos += csf[hnd].roll_pitch_bits;


  /*  Adjust the heading for negatives and greater than 360.0 values.  */

  if (record->heading < 0.0) record->heading += 360.0;
  if (record->heading > 360.0) record->heading -= 360.0;


  /*  [CSF:7]  Platform heading.  */

  czmil_bit_pack (buffer, bpos, csf[hnd].heading_bits, NINT (record->heading * csf[hnd].angle_scale));
  bpos += csf[hnd].heading_bits;


  /*  [CSF:8]  Ranges.  Store invalid range as csf[hnd].range_max and return -1.0 on read.  */

  for (i = 0 ; i < 9 ; i++)
    {
      i32value = NINT (record->range[i] * csf[hnd].range_scale);
      if (i32value < 0 || i32value > csf[hnd].range_max) i32value = csf[hnd].range_max;
      czmil_bit_pack (buffer, bpos, csf[hnd].range_bits, i32value);
      bpos += csf[hnd].range_bits;
    }


  /****************************************** VERSION CHECK ******************************************

      The range_in_water, intensity, and intensity_in_water fields did not exist prior to major
      version 2.

  ***************************************************************************************************/

  if (csf[hnd].major_version >= 2)
    {
      /*  [CSF:9]  Ranges in water.  Store invalid range as csf[hnd].range_max and return -1.0 on read.  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = NINT (record->range_in_water[i] * csf[hnd].range_scale);
          if (i32value < 0 || i32value > csf[hnd].range_max) i32value = csf[hnd].range_max;
          czmil_bit_pack (buffer, bpos, csf[hnd].range_bits, i32value);
          bpos += csf[hnd].range_bits;
        }


      /*  [CSF:10]  Intensities.  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = NINT (record->intensity[i] * csf[hnd].intensity_scale);
          if (i32value < 0 || i32value > csf[hnd].intensity_max)
            {
              sprintf (czmil_error.info, _("In CSF file %s, Record %d, Channel %d :\nIntensity %f out of range."), csf[hnd].path,
                       csf[hnd].header.number_of_records, i, record->intensity[i]);
              return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, csf[hnd].intensity_bits, i32value);
          bpos += csf[hnd].intensity_bits;
        }


      /*  [CSF:11]  Intensities in water.  */

      for (i = 0 ; i < 9 ; i++)
        {
          i32value = NINT (record->intensity_in_water[i] * csf[hnd].intensity_scale);
          if (i32value < 0 || i32value > csf[hnd].intensity_max)
            {
              sprintf (czmil_error.info, _("In CSF file %s, Record %d, Channel %d :\nIntensity in water %f out of range."), csf[hnd].path,
                       csf[hnd].header.number_of_records, i, record->intensity_in_water[i]);
              return (czmil_error.czmil = CZMIL_CSF_VALUE_OUT_OF_RANGE_ERROR);
            }
          czmil_bit_pack (buffer, bpos, csf[hnd].intensity_bits, i32value);
          bpos += csf[hnd].intensity_bits;
        }
    }


  /*  Compute the min and max lat and lon (unbiased by 90 and 180 respectively).  */

  csf[hnd].header.min_lon = MIN (csf[hnd].header.min_lon, record->longitude);
  csf[hnd].header.max_lon = MAX (csf[hnd].header.max_lon, record->longitude);
  csf[hnd].header.min_lat = MIN (csf[hnd].header.min_lat, record->latitude);
  csf[hnd].header.max_lat = MAX (csf[hnd].header.max_lat, record->latitude);


  /*  Quick check just to make sure we didn't do something weird.  */

  i32value = bpos / 8;
  if (bpos % 8) i32value++;

  if (i32value != csf[hnd].buffer_size)
    {
      sprintf (czmil_error.info, _("File : %s\nBuffer size packed does not match expected buffer size :\nRecord : %d\n"), csf[hnd].path,
               csf[hnd].header.number_of_records);
      return (czmil_error.czmil = CZMIL_CSF_WRITE_BUFFER_SIZE_ERROR);
    }


  /*  If we're updating a record (as opposed to adding a new record to a new file) we actually write it out.  */

  if (recnum != CZMIL_NEXT_RECORD)
    {
      /*  Write the buffer to the file.  */

      if (!fwrite (csf[hnd].buffer, csf[hnd].buffer_size, 1, csf[hnd].fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CSF data :\n%s\n"), csf[hnd].path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CSF_WRITE_ERROR);
        }


      csf[hnd].modified = 1;
      csf[hnd].write = 1;
    }


  /*  If we're appending a record we pack it into the block buffer and write it when it gets close to full.  */

  else
    {
      /*  Increment the number of records counter in the header.  */

      csf[hnd].header.number_of_records++;


      /*  Now update the I/O buffer address so we can be ready for the next record.  */

      csf[hnd].io_buffer_address += csf[hnd].buffer_size;
    }


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_cif_record

 - Purpose:     Write a single CIF record to the CIF file at the current location.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/14/12

 - Arguments:
                - cif_struct     =    The internal CIF structure

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CIF_WRITE_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static int32_t czmil_write_cif_record (INTERNAL_CZMIL_CIF_STRUCT *cif_struct)
{
  int32_t   bpos;


  /*  The actual buffer will never be sizeof (CZMIL_CIF_Data) in size since we are bit packing it but this way
      we don't have to worry about blowing this up.  Also, we don't allocate the memory because memory
      allocation invokes a system wide mutex.  */

  uint8_t buffer[sizeof (CZMIL_CIF_Data)];


  /*  Added this memset to avoid an uninitialize warning in czmil_bit_pack.  */

  memset (buffer, 0, sizeof (CZMIL_CIF_Data));


  bpos = 0;


  /*  Bit pack the CWF address, CPF address, CWF buffer size, and CPF buffer size (in that order).  */

  czmil_double_bit_pack (buffer, bpos, cif_struct->header.cwf_address_bits, cif_struct->record.cwf_address);
  bpos += cif_struct->header.cwf_address_bits;

  czmil_double_bit_pack (buffer, bpos, cif_struct->header.cpf_address_bits, cif_struct->record.cpf_address);
  bpos += cif_struct->header.cpf_address_bits;

  czmil_bit_pack (buffer, bpos, cif_struct->header.cwf_buffer_size_bits, cif_struct->record.cwf_buffer_size);
  bpos += cif_struct->header.cwf_buffer_size_bits;

  czmil_bit_pack (buffer, bpos, cif_struct->header.cpf_buffer_size_bits, cif_struct->record.cpf_buffer_size);
  bpos += cif_struct->header.cpf_buffer_size_bits;


  /*  If the buffer is full, we need to flush the buffer.  */

  if ((cif_struct->io_buffer_size - cif_struct->io_buffer_address) < cif_struct->header.record_size_bytes)
    {
      /*  Write the output buffer to disk.  Note that io_buffer_address is the address in the output buffer
          of the next available byte.  Therefor, it is also the size of the data already written into the 
          output buffer.  */

      if (!fwrite (cif_struct->io_buffer, cif_struct->io_buffer_address, 1, cif_struct->fp))
        {
          sprintf (czmil_error.info, _("File : %s\nError writing CIF data :\n%s\n"), cif_struct->path, strerror (errno));
          return (czmil_error.czmil = CZMIL_CIF_WRITE_ERROR);
        }


      /*  Reset the address to the beginning of the buffer.  */

      cif_struct->io_buffer_address = 0;
    }


  /*  Copy the latest record into the CIF output buffer and increment the output buffer pointer.  */

  memcpy (&cif_struct->io_buffer[cif_struct->io_buffer_address], buffer, cif_struct->header.record_size_bytes);
  cif_struct->io_buffer_address += cif_struct->header.record_size_bytes;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_read_cif_record

 - Purpose:     Retrieve a CZMIL CIF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - recnum         =    The record number of the CZMIL record to be retrieved
                - record         =    The CIF_Data record to be returned

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_INVALID_RECORD_NUMBER_ERROR
                - CZMIL_CIF_READ_FSEEK_ERROR
                - CZMIL_CIF_READ_ERROR

 - Caveats:     Note that we don't pass in a record structure to fill.  This is because this
                function is only used internally and we want to put the record into
                cif[hnd].record so that we can re-use it for CPF and CWF I/O.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

CZMIL_INLINE int32_t czmil_read_cif_record (int32_t hnd, int32_t recnum, CZMIL_CIF_Data *record)
{
  int64_t pos;
  int32_t bpos;
  uint8_t buffer[sizeof (CZMIL_CIF_Data) + 16];


  /*  Check for record out of bounds.  */

  if (recnum >= cif[hnd].header.number_of_records || recnum < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nRecord : %d\nInvalid record number.\n"), cif[hnd].path, recnum);
      return (czmil_error.czmil = CZMIL_INVALID_RECORD_NUMBER_ERROR);
    }


  /*  Compute the record position based on the record size computed from the packed bit sizes of the CIF record.  */

  pos = (int64_t) recnum * (int64_t) cif[hnd].header.record_size_bytes + (int64_t) cif[hnd].header.header_size;


  /*  If we have just read this record we don't need to do it again.  This will happen if we just read the CIF record for
      a CPF file and now we want to read the CWF record (or vice versa).  Note that cif[hnd].prev_pos and cif[hnd].pos are
      initially set to 0 (which is not a valid CIF pointer since the header is always at 0) so we can't accidentally think
      we've already read the desired record.  */

  if (pos == cif[hnd].prev_pos)
    {
      *record = cif[hnd].record;
    }
  else
    {
      /*  We only want to do the fseek (which flushes the buffer) if we aren't already in the correct position.  We may be in
          the correct position if we just read a CIF record to find a CPF record and now we want to read the next CPF record.  */

      if (pos != cif[hnd].pos)
        {
          if (fseeko64 (cif[hnd].fp, pos, SEEK_SET) < 0)
            {
              sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CIF record :\n%s\n"), cif[hnd].path, strerror (errno));
              return (czmil_error.czmil = CZMIL_CIF_READ_FSEEK_ERROR);
            }
        }


      /*  Read the record.  */

      if (!fread (buffer, cif[hnd].header.record_size_bytes, 1, cif[hnd].fp)) return (czmil_error.czmil = CZMIL_CIF_READ_ERROR);


      /*  Unpack the CWF address, CPF address, CWF buffer size, and CPF buffer size (in that order).  */

      bpos = 0;

      cif[hnd].record.cwf_address = czmil_double_bit_unpack (buffer, bpos, cif[hnd].header.cwf_address_bits);
      bpos += cif[hnd].header.cwf_address_bits;
      cif[hnd].record.cpf_address = czmil_double_bit_unpack (buffer, bpos, cif[hnd].header.cpf_address_bits);
      bpos += cif[hnd].header.cpf_address_bits;
      cif[hnd].record.cwf_buffer_size = czmil_bit_unpack (buffer, bpos, cif[hnd].header.cwf_buffer_size_bits);
      bpos += cif[hnd].header.cwf_buffer_size_bits;
      cif[hnd].record.cpf_buffer_size = czmil_bit_unpack (buffer, bpos, cif[hnd].header.cpf_buffer_size_bits);
      bpos += cif[hnd].header.cpf_buffer_size_bits;

      *record = cif[hnd].record;
      cif[hnd].pos = pos + cif[hnd].header.record_size_bytes;
    }


  /*  Save the position of the previous read in case we come back and want to read it again.  */

  cif[hnd].prev_pos = pos;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}




/*********************************************************************************************/
/*!

 - Function:    czmil_read_caf_record

 - Purpose:     Retrieve a CZMIL CAF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The file handle
                - record         =    The returned CZMIL CAF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CAF_READ_ERROR

 - Caveats:     All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CAF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_read_caf_record (int32_t hnd, CZMIL_CAF_Data *record)
{
  int32_t bpos, i32value;


  /*  The actual buffer will never be sizeof (CZMIL_CAF_Data) in size since we are unpacking it but this way
      we don't have to worry about blowing this up.  Also, we don't allocate the memory because memory
      allocation invokes a system wide mutex.  */

  uint8_t buffer[sizeof (CZMIL_CAF_Data)];


  caf[hnd].at_end = 0;


  if (!fread (buffer, caf[hnd].buffer_size, 1, caf[hnd].fp))
    {
      sprintf (czmil_error.info, _("File : %s\nError reading CAF record:\n%s\n"), caf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CAF_READ_ERROR);
    }


  bpos = 0;


  /*  [CAF:0]  Record number (shot ID).  */
  
  record->shot_id = czmil_bit_unpack (buffer, bpos, caf[hnd].shot_id_bits);
  bpos += caf[hnd].shot_id_bits;


  /*  [CAF:1]  Channel number.  */

  record->channel_number = czmil_bit_unpack (buffer, bpos, caf[hnd].channel_number_bits);
  bpos += caf[hnd].channel_number_bits;


  /*  [CAF:2]  Optech classification.  */

  record->optech_classification = czmil_bit_unpack (buffer, bpos, caf[hnd].optech_classification_bits);
  bpos += caf[hnd].optech_classification_bits;


  /*  [CAF:3]  Interest_point.  */

  i32value = czmil_bit_unpack (buffer, bpos, caf[hnd].interest_point_bits);
  bpos += caf[hnd].interest_point_bits;
  record->interest_point = (float) (i32value) / caf[hnd].interest_point_scale;


  /*  [CAF:4]  Return number.  */

  record->return_number = czmil_bit_unpack (buffer, bpos, caf[hnd].return_bits);
  bpos += caf[hnd].return_bits;


  /*  [CAF:5]  Number of returns.  */

  record->number_of_returns = czmil_bit_unpack (buffer, bpos, caf[hnd].return_bits);
  bpos += caf[hnd].return_bits;


  caf[hnd].write = 0;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_write_caf_record

 - Purpose:     Appends a CZMIL CAF record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/03/14

 - Arguments:
                - hnd            =    The file handle
                - record         =    The CZMIL CAF record

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CAF_APPEND_ERROR
                - CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR
                - CZMIL_CAF_WRITE_BUFFER_SIZE_ERROR
                - CZMIL_CAF_WRITE_ERROR

 - Caveats:     This function is ONLY used to append a new record to a newly created file.
                There should be no reason to actually change the CAF data after the file
                has been created.

                All returned error values are less than zero.  Success or a file handle
                will be greater than or equal to zero.  A simple test for failure is to
                check to see if the return is less than zero.

                Keeping track of what got packed where between the read and write 
                code can be a bit difficult.  To make it simpler to track I have added a
                label (e.g. [CAF:3])to the beginning of each section so that you can search
                from the read to write or vice versa.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_write_caf_record (int32_t hnd, CZMIL_CAF_Data *record)
{
  int32_t i32value, bpos;
  uint8_t *buffer;


  /*  Appending a record is only allowed if you are creating a new file.  */

  if (!caf[hnd].created)
    {
      sprintf (czmil_error.info, _("File : %s\nAppending to pre-existing CAF file not allowed.\n"), caf[hnd].path);
      return (czmil_error.czmil = CZMIL_CAF_APPEND_ERROR);
    }


  /*  If there is ANY chance at all that we might overrun our I/O buffer on this write, we need to 
      flush the buffer.  Since we're compressing our records they should not be anywhere near the
      size of a CZMIL_CAF_Data structure.  But, also since we're compressing the records, the actual size
      of the next record is unknown so we are just making sure that we're not within sizeof (CZMIL_CAF_Data)
      bytes of the end of the output buffer.  That way we're taking no chances of exceeding our output
      buffer size.  */

  if ((caf[hnd].io_buffer_size - caf[hnd].io_buffer_address) < sizeof (CZMIL_CAF_Data))
    {
      if (czmil_flush_caf_io_buffer (hnd) < 0) return (czmil_error.czmil);
    }


  /*  Set the buffer pointer to the correct location within the I/O buffer.  */

  buffer = &caf[hnd].io_buffer[caf[hnd].io_buffer_address];


  caf[hnd].at_end = 1;


  bpos = 0;


  /*  [CAF:0]  Record number (shot ID).  */
  
  if (record->shot_id < 0 || record->shot_id > caf[hnd].shot_id_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nCAF shot ID %d out of range"),
               caf[hnd].path, caf[hnd].header.number_of_records, record->shot_id);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }

  czmil_bit_pack (buffer, bpos, caf[hnd].shot_id_bits, record->shot_id);
  bpos += caf[hnd].shot_id_bits;


  /*  [CAF:1]  Channel number.  */

  if (record->channel_number < 0 || record->channel_number > caf[hnd].channel_number_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nCAF channel number %d out of range"),
               caf[hnd].path, caf[hnd].header.number_of_records, record->channel_number);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }

  czmil_bit_pack (buffer, bpos, caf[hnd].channel_number_bits, record->channel_number);
  bpos += caf[hnd].channel_number_bits;


  /*  [CAF:2]  Optech classification.  */

  if (record->optech_classification < 0 || record->optech_classification > caf[hnd].optech_classification_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nOptech classification %d out of range."), caf[hnd].path,
               caf[hnd].header.number_of_records, record->optech_classification);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, caf[hnd].optech_classification_bits, record->optech_classification);
  bpos += caf[hnd].optech_classification_bits;


  /*  [CAF:3]  Interest point.  */

  i32value = NINT ((record->interest_point * caf[hnd].interest_point_scale));
  if (i32value < 0 || i32value > caf[hnd].interest_point_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nCAF interest point %f out of range."), caf[hnd].path,
               caf[hnd].header.number_of_records, record->interest_point);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, caf[hnd].interest_point_bits, i32value);
  bpos += caf[hnd].interest_point_bits;


  /*  [CAF:4]  Return number.  */

  if (record->return_number < 0 || record->return_number > caf[hnd].return_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nReturn number %d out of range."), caf[hnd].path,
               caf[hnd].header.number_of_records, record->return_number);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, caf[hnd].return_bits, record->return_number);
  bpos += caf[hnd].return_bits;


  /*  [CAF:5]  Number of returns.  */

  if (record->number_of_returns < 0 || record->number_of_returns > caf[hnd].return_max)
    {
      sprintf (czmil_error.info, _("In CAF file %s, Record %d : \nNumber of returns %d out of range."), caf[hnd].path,
               caf[hnd].header.number_of_records, record->number_of_returns);
      return (czmil_error.czmil = CZMIL_CAF_VALUE_OUT_OF_RANGE_ERROR);
    }
  czmil_bit_pack (buffer, bpos, caf[hnd].return_bits, record->number_of_returns);
  bpos += caf[hnd].return_bits;


  /*  Quick check just to make sure we didn't do something weird.  */

  i32value = bpos / 8;
  if (bpos % 8) i32value++;

  if (i32value != caf[hnd].buffer_size)
    {
      sprintf (czmil_error.info, _("File : %s\nBuffer size packed does not match expected buffer size :\nRecord : %d\n"), caf[hnd].path,
               caf[hnd].header.number_of_records);
      return (czmil_error.czmil = CZMIL_CAF_WRITE_BUFFER_SIZE_ERROR);
    }


  /*  Increment the number of records counter in the header.  */

  caf[hnd].header.number_of_records++;


  /*  Now update the I/O buffer address so we can be ready for the next record.  */

  caf[hnd].io_buffer_address += caf[hnd].buffer_size;


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_cwf_header

 - Purpose:     Update the modifiable fields of the CWF header record.  See CZMIL_CWF_Header in
                czmil.h to determine which fields are modifiable after file creation.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - cwf_header     =    The CZMIL_CWF_Header structure.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR

 - Modifiable fields: 

                - modification_software
                - comments

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_cwf_header (int32_t hnd, CZMIL_CWF_Header *cwf_header)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cwf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  These are the only modifiable fields.  */

  strcpy (cwf[hnd].header.modification_software, cwf_header->modification_software);
  strcpy (cwf[hnd].header.comments, cwf_header->comments);


  /*  Force a header write when we close the file.  */

  cwf[hnd].modified = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_cpf_header

 - Purpose:     Update the modifiable fields of the CPF header record.  See CZMIL_CPF_Header in
                czmil.h to determine which fields are modifiable after file creation.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The file handle
                - cpf_header     =    The CZMIL_CPF_Header structure.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR

 - Modifiable fields: 

                - modification_software
                - local_vertical_datum
                - comments
                - user_data_description


*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_cpf_header (int32_t hnd, CZMIL_CPF_Header *cpf_header)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  These are the only modifiable fields.  */

  strcpy (cpf[hnd].header.modification_software, cpf_header->modification_software);
  cpf[hnd].header.local_vertical_datum = cpf_header->local_vertical_datum;
  strcpy (cpf[hnd].header.comments, cpf_header->comments);
  strcpy (cpf[hnd].header.user_data_description, cpf_header->user_data_description);


  /*  Force a header write when we close the file.  */

  cpf[hnd].modified = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_csf_header

 - Purpose:     Update the modifiable fields of the CSF header record.  See CZMIL_CSF_Header in
                czmil.h to determine which fields are modifiable after file creation.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/19/12

 - Arguments:
                - hnd            =    The file handle
                - csf_header     =    The CZMIL_CSF_Header structure.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR

 - Modifiable fields: 

                - modification_software

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_CSF_header (int32_t hnd, CZMIL_CSF_Header *csf_header)
{
#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (csf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  This is the only modifiable field.  */

  strcpy (csf[hnd].header.modification_software, csf_header->modification_software);


  /*  Force a header write when we close the file.  */

  csf[hnd].modified = 1;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_add_field_to_cwf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CWF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_cwf_header or update it using czmil_update_field_in_cwf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CWF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_add_field_to_cwf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_add_field_to_cwf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_add_field_to_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal add_field_to_cwf_header function.  */

  return (czmil_add_field_to_cwf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_add_field_to_cwf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CWF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_cwf_header or update it using czmil_update_field_in_cwf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CWF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CWF_EXCEEDED_HEADER_SIZE_ERROR
                - Error value from czmil_write_cwf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_add_field_to_cwf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], tag_bracket[1024];
  int64_t pos = 0;
  uint8_t section_label = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cwf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (tag_bracket, "%s", tag);
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (tag_bracket, "[%s]", tag);
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cwf[hnd].fp))
    {
      /*  Check to see if this tag is already part of the header.  */

      if (!strncmp (varin, tag_bracket, strlen (tag_bracket)))
        {
          sprintf (czmil_error.info, _("File : %s\nCannot use pre-existing tag %s for application defined tag!\n"), cwf[hnd].path, tag_bracket);
          return (czmil_error.czmil = CZMIL_BAD_TAG_ERROR);
        }


      /*  Check for the [APPLICATION DEFINED FIELDS] section label.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]"))) section_label = 1;


      /*  Save the position for computation of header size.  */

      pos = ftello64 (cwf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += strlen (info) + 1;
  pos += strlen (N_("\n########## [END OF HEADER] ##########\n"));
  if (!section_label) pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));

  if (pos >= cwf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the section label if it wasn't there.  */

  if (!section_label)
    {
      sprintf (&cwf[hnd].app_tags[cwf[hnd].app_tags_pos], N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
      cwf[hnd].app_tags_pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
    }


  /*  Add the tagged field.  */

  sprintf (&cwf[hnd].app_tags[cwf[hnd].app_tags_pos], "%s\n", info);
  cwf[hnd].app_tags_pos += strlen (info) + 1;


  /*  Write the header.  */

  if (czmil_write_cwf_header (hnd) < 0) return (czmil_error.czmil);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cwf[hnd].fp, cwf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after adding user field to CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_get_field_from_cwf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CWF file handle
                - idl_tag        =    The unterminated ASCII tag value, without brackets.
                                      Include the leading brace, {, for multi-line fields.
                - tag_length     =    The length of the unterminated IDL tag string
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - Error value returned from czmil_get_field_from_cwf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_get_field_to_cwf_header.  We're just null terminating the tag name.
                This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_get_field_from_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal get_field_from_cwf_header function.  */

  return (czmil_get_field_from_cwf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_field_from_cwf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CWF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_get_field_from_cwf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *s;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  We only need to retrieve the application tagged field.  */

  while (czmil_ngets (varin, sizeof (varin), cwf[hnd].fp))
    {
      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {
          if (strstr (varin, info))
            {
              match = 1;


              /*  Deal with multi-line tagged info.  */

              if (varin[0] == '{' && tag[0] == '{')
                {
                  strcpy (contents, "");
                  while (fgets (varin, sizeof (varin), cwf[hnd].fp))
                    {
                      if (varin[0] == '}')
                        {
                          //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                          s = contents;

                          while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                          if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                          break;
                        }

                      strcat (contents, varin);
                    }
                }
              else
                {
                  /*  Put everything to the right of the equals sign into 'contents'.   */

                  czmil_get_string (varin, contents);
                }

              break;
            }
        }
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cwf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_update_field_in_cwf_header

 - Purpose:     Updates an application defined field in the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CWF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_update_field_in_cwf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_update_field_in_cwf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_update_field_in_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal update_field_in_cwf_header function.  */

  return (czmil_update_field_in_cwf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_field_in_cwf_header

 - Purpose:     Updates an application defined field in the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CWF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CWF_EXCEEDED_HEADER_SIZE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cwf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_field_in_cwf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cwf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (cwf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cwf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cwf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (cwf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cwf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = cwf[hnd].header.header_size - ftello64 (cwf[hnd].fp);

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_cwf_header");
      exit (-1);
    }

  if (!fread (buffer, size, 1, cwf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Now we add the tagged field at its previous location.  */

  if (fseeko64 (cwf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to updating user field in CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Create the tagged field with the new contents.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += size - (strlen (info) + 1);

  if (pos >= cwf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), cwf[hnd].path);
      free (buffer);
      return (czmil_error.czmil = CZMIL_CWF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the tagged field.  */

  fprintf (cwf[hnd].fp, "%s\n", info);


  /*  Now write the remainder of the header back out.  */

  fwrite (buffer, size - (strlen (info) + 1), 1, cwf[hnd].fp);
  fflush (cwf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CWF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  cwf[hnd].app_tags_pos = 0;
  czmil_read_cwf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cwf[hnd].fp, cwf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after updating user field in CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_delete_field_from_cwf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd             =    The CWF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_delete_field_from_cwf_header

 - Caveats      This is a wrapper function that can be called from IDL that calls
                czmil_delete_field_from_cwf_header.  We're just null terminating the tag
                name.  This function should only be used by HydroFusion.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_delete_field_from_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal delete_field_from_cwf_header function.  */

  return (czmil_delete_field_from_cwf_header (hnd, tag));
}



/********************************************************************************************/
/*!

 - Function:    czmil_delete_field_from_cwf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CWF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd            =    The CWF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CWF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR
                - Error value from czmil_write_cwf_header

 - Caveats

*********************************************************************************************/

CZMIL_DLL int32_t czmil_delete_field_from_cwf_header (int32_t hnd, char *tag)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size, rem_size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cwf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cwf[hnd].path);
      return (czmil_error.czmil = CZMIL_CWF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cwf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (cwf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cwf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cwf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (cwf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cwf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = cwf[hnd].header.header_size - pos;

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_cwf_header");
      exit (-1);
    }


  //  Clear the buffer

  memset (buffer, ' ', size);


  //  We only want to read the remaining part, not the piece we are deleting.

  rem_size = cwf[hnd].header.header_size - ftello64 (cwf[hnd].fp);
  if (!fread (buffer, rem_size, 1, cwf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Seek to the position of the beginning of the tag that we are deleting.  */

  if (fseeko64 (cwf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to deleting user field from CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Now write the remainder of the header back out (over the deleted field).  Note that I'm not worried about not having enough remaining
      header to overwrite the field we're deleting.  That's because the ASCII header is usually much larger than the actual data that is stored
      in it.  The remainder of the header is filled with blanks so this shouldn't be a problem.  */

  fwrite (buffer, size - (strlen (info) + 1), 1, cwf[hnd].fp);
  fflush (cwf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CWF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  cwf[hnd].app_tags_pos = 0;
  czmil_read_cwf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cwf[hnd].fp, cwf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after deleting user field from CWF header :\n%s\n"), cwf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CWF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_add_field_to_cpf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CPF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_cpf_header or update it using czmil_update_field_in_cpf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CPF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_add_field_to_cpf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_add_field_to_cpf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_add_field_to_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal add_field_to_cpf_header function.  */

  return (czmil_add_field_to_cpf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_add_field_to_cpf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CPF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_cpf_header or update it using czmil_update_field_in_cpf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CPF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CPF_EXCEEDED_HEADER_SIZE_ERROR
                - Error value from czmil_write_cpf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_add_field_to_cpf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], tag_bracket[1024];
  int64_t pos = 0;
  uint8_t section_label = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (tag_bracket, "%s", tag);
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (tag_bracket, "[%s]", tag);
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cpf[hnd].fp))
    {
      /*  Check to see if this tag is already part of the header.  */

      if (!strncmp (varin, tag_bracket, strlen (tag_bracket)))
        {
          sprintf (czmil_error.info, _("File : %s\nCannot use pre-existing tag %s for application defined tag!\n"), cpf[hnd].path, tag_bracket);
          return (czmil_error.czmil = CZMIL_BAD_TAG_ERROR);
        }


      /*  Check for the [APPLICATION DEFINED FIELDS] section label.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]"))) section_label = 1;


      /*  Save the position for computation of header size.  */

      pos = ftello64 (cpf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += strlen (info) + 1;
  pos += strlen (N_("\n########## [END OF HEADER] ##########\n"));
  if (!section_label) pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));

  if (pos >= cpf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the section label if it wasn't there.  */

  if (!section_label)
    {
      sprintf (&cpf[hnd].app_tags[cpf[hnd].app_tags_pos], N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
      cpf[hnd].app_tags_pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
    }


  /*  Add the tagged field.  */

  sprintf (&cpf[hnd].app_tags[cpf[hnd].app_tags_pos], "%s\n", info);
  cpf[hnd].app_tags_pos += strlen (info) + 1;


  /*  Write the header.  */

  if (czmil_write_cpf_header (hnd) < 0) return (czmil_error.czmil);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cpf[hnd].fp, cpf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after adding user field to CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_get_field_from_cpf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CPF file handle
                - idl_tag        =    The unterminated ASCII tag value, without brackets.
                                      Include the leading brace, {, for multi-line fields.
                - tag_length     =    The length of the unterminated IDL tag string
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_get_field_from_cpf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                This is a wrapper function that can be called from IDL that calls
                czmil_get_field_from_cpf_header.  We're just null terminating the tag name.
                This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_get_field_from_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal get_field_from_cpf_header function.  */

  return (czmil_get_field_from_cpf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_field_from_cpf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CPF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_get_field_from_cpf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *s;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  We only need to retrieve the application tagged field.  */

  while (czmil_ngets (varin, sizeof (varin), cpf[hnd].fp))
    {
      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {
          if (strstr (varin, info))
            {
              match = 1;


              /*  Deal with multi-line tagged info.  */

              if (varin[0] == '{' && tag[0] == '{')
                {
                  strcpy (contents, "");
                  while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                    {
                      if (varin[0] == '}')
                        {
                          //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                          s = contents;

                          while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                          if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                          break;
                        }

                      strcat (contents, varin);
                    }
                }
              else
                {
                  /*  Put everything to the right of the equals sign into 'contents'.   */

                  czmil_get_string (varin, contents);
                }

              break;
            }
        }
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cpf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_update_field_in_cpf_header

 - Purpose:     Updates an application defined field in the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CPF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_update_field_in_cpf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_update_field_in_cpf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_update_field_in_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal update_field_in_cpf_header function.  */

  return (czmil_update_field_in_cpf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_field_in_cpf_header

 - Purpose:     Updates an application defined field in the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CPF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CPF_EXCEEDED_HEADER_SIZE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_cpf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_field_in_cpf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (cpf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cpf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (cpf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cpf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = cpf[hnd].header.header_size - ftello64 (cpf[hnd].fp);

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_cpf_header");
      exit (-1);
    }

  if (!fread (buffer, size, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Now we add the tagged field at its previous location.  */

  if (fseeko64 (cpf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to updating user field in CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Create the tagged field with the new contents.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += size - (strlen (info) + 1);

  if (pos >= cpf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), cpf[hnd].path);
      free (buffer);
      return (czmil_error.czmil = CZMIL_CPF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the tagged field.  */

  fprintf (cpf[hnd].fp, "%s\n", info);


  /*  Now write the remainder of the header back out.  */

  fwrite (buffer, size - (strlen (info) + 1), 1, cpf[hnd].fp);
  fflush (cpf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CPF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  cpf[hnd].app_tags_pos = 0;
  czmil_read_cpf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cpf[hnd].fp, cpf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after updating user field in CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_delete_field_from_cpf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd             =    The CPF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_delete_field_from_cpf_header

 - Caveats      This is a wrapper function that can be called from IDL that calls
                czmil_delete_field_from_cpf_header.  We're just null terminating the tag
                name.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_delete_field_from_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal delete_field_from_cpf_header function.  */

  return (czmil_delete_field_from_cpf_header (hnd, tag));
}



/********************************************************************************************/
/*!

 - Function:    czmil_delete_field_from_cpf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CPF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd            =    The CPF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CPF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR
                - Error value from czmil_write_cpf_header

 - Caveats

*********************************************************************************************/

CZMIL_DLL int32_t czmil_delete_field_from_cpf_header (int32_t hnd, char *tag)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size, rem_size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (cpf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), cpf[hnd].path);
      return (czmil_error.czmil = CZMIL_CPF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (cpf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (cpf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), cpf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), cpf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (cpf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), cpf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = cpf[hnd].header.header_size - pos;

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_cpf_header");
      exit (-1);
    }


  //  Clear the buffer

  memset (buffer, ' ', size);


  //  We only want to read the remaining part, not the piece we are deleting.

  rem_size = cpf[hnd].header.header_size - ftello64 (cpf[hnd].fp);
  if (!fread (buffer, rem_size, 1, cpf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Seek to the position of the beginning of the tag that we are deleting.  */

  if (fseeko64 (cpf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to deleting user field from CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Now write the remainder of the header back out (over the deleted field).  Note that I'm not worried about not having enough remaining
      header to overwrite the field we're deleting.  That's because the ASCII header is usually much larger than the actual data that is stored
      in it.  The remainder of the header is filled with blanks so this shouldn't be a problem.  */

  fwrite (buffer, size, 1, cpf[hnd].fp);
  fflush (cpf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CPF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  cpf[hnd].app_tags_pos = 0;
  czmil_read_cpf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (cpf[hnd].fp, cpf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after deleting user field from CPF header :\n%s\n"), cpf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CPF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_add_field_to_csf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CSF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_csf_header or update it using czmil_update_field_in_csf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CSF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_add_field_to_csf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_add_field_to_csf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_add_field_to_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal add_field_to_csf_header function.  */

  return (czmil_add_field_to_csf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_add_field_to_csf_header

 - Purpose:     Adds an application defined tagged ASCII field to the CSF header.  Since the
                API doesn't know what the tagged field means, it will be ignored when the
                header is read.  You can retrieve the user defined fields using
                czmil_get_field_from_csf_header or update it using czmil_update_field_in_csf_header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CSF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CSF_EXCEEDED_HEADER_SIZE_ERROR
                - Error value from czmil_write_csf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_add_field_to_csf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], tag_bracket[1024];
  int64_t pos = 0;
  uint8_t section_label = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (csf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (tag_bracket, "%s", tag);
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (tag_bracket, "[%s]", tag);
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), csf[hnd].fp))
    {
      /*  Check to see if this tag is already part of the header.  */

      if (!strncmp (varin, tag_bracket, strlen (tag_bracket)))
        {
          sprintf (czmil_error.info, _("File : %s\nCannot use pre-existing tag %s for application defined tag!\n"), csf[hnd].path, tag_bracket);
          return (czmil_error.czmil = CZMIL_BAD_TAG_ERROR);
        }


      /*  Check for the [APPLICATION DEFINED FIELDS] section label.  */

      if (strstr (varin, N_("[APPLICATION DEFINED FIELDS]"))) section_label = 1;


      /*  Save the position for computation of header size.  */

      pos = ftello64 (csf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += strlen (info) + 1;
  pos += strlen (N_("\n########## [END OF HEADER] ##########\n"));
  if (!section_label) pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));

  if (pos >= csf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the section label if it wasn't there.  */

  if (!section_label)
    {
      sprintf (&csf[hnd].app_tags[csf[hnd].app_tags_pos], N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
      csf[hnd].app_tags_pos += strlen (N_("\n########## [APPLICATION DEFINED FIELDS] ##########\n\n"));
    }


  /*  Add the tagged field.  */

  sprintf (&csf[hnd].app_tags[csf[hnd].app_tags_pos], "%s\n", info);
  csf[hnd].app_tags_pos += strlen (info) + 1;


  /*  Write the header.  */

  if (czmil_write_csf_header (hnd) < 0) return (czmil_error.czmil);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (csf[hnd].fp, csf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after adding user field to CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_get_field_from_csf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CSF file handle
                - idl_tag        =    The unterminated ASCII tag value, without brackets.
                                      Include the leading brace, {, for multi-line fields.
                - tag_length     =    The length of the unterminated IDL tag string
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_get_field_from_csf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                This is a wrapper function that can be called from IDL that calls
                czmil_get_field_from_csf_header.  We're just null terminating the tag name.
                This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_get_field_from_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal get_field_from_csf_header function.  */

  return (czmil_get_field_from_csf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_field_from_csf_header

 - Purpose:     Retrieves an application defined tagged ASCII field from the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CSF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_READ_FSEEK_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_get_field_from_csf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *s;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d\n", __FILE__, __FUNCTION__, __LINE__, hnd);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Create the tagged field.  */

  sprintf (info, "[%s]", tag);


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Read the tagged ASCII header data.  We only need to retrieve the application tagged field.  */

  while (czmil_ngets (varin, sizeof (varin), csf[hnd].fp))
    {
      if (strstr (varin, N_("[END OF HEADER]"))) break;


      /*  Skip comments and blank lines.  */

      if (varin[0] != '#' && (strstr (varin, "[") || strstr (varin, "{")))
        {
          if (strstr (varin, info))
            {
              match = 1;


              /*  Deal with multi-line tagged info.  */

              if (varin[0] == '{' && tag[0] == '{')
                {
                  strcpy (contents, "");
                  while (fgets (varin, sizeof (varin), csf[hnd].fp))
                    {
                      if (varin[0] == '}')
                        {
                          //  Strip any trailing CR/LF off of the multi-line string so that, if we update the header, we won't keep adding blank lines.

                          s = contents;

                          while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

                          if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;

                          break;
                        }

                      strcat (contents, varin);
                    }
                }
              else
                {
                  /*  Put everything to the right of the equals sign into 'contents'.   */

                  czmil_get_string (varin, contents);
                }

              break;
            }
        }
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), csf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_update_field_in_csf_header

 - Purpose:     Updates an application defined field in the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd             =    The CSF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string
                - idl_contents    =    The unterminated ASCII tagged field contents
                - contents_length =    The length of the unterminated IDL contents string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_update_field_in_csf_header

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

                This is a wrapper function that can be called from IDL that calls
                czmil_update_field_in_csf_header.  We're just null terminating the tag name and
                contents field.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_update_field_in_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length)
{
  char tag[1024], contents[8192];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Null terminate the unterminated IDL contents string.  */

  strncpy (contents, idl_contents, contents_length);
  contents[contents_length] = 0;


  /*  Call the normal update_field_in_csf_header function.  */

  return (czmil_update_field_in_csf_header (hnd, tag, contents));
}



/********************************************************************************************/
/*!

 - Function:    czmil_update_field_in_csf_header

 - Purpose:     Updates an application defined field in the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/16/12

 - Arguments:
                - hnd            =    The CSF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.
                - contents       =    The ASCII tagged field contents

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_CSF_EXCEEDED_HEADER_SIZE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR

 - Caveats      <b>DO NOT, under any circumstances, use a tag that is already used in
                czmil_write_csf_header!</b>  The combination of the contents and the tag
                must not exceed 8K bytes.  Do not use the equal sign (=) or brackets
                ([ or ]) in either the tag or the contents.

                DO NOT put a trailing line-feed (\n) on multi-line fields.  You may put
                line-feeds in the body of the field to make it more readable but the API
                will add a final line-feed prior to the trailing bracket (}).

*********************************************************************************************/

CZMIL_DLL int32_t czmil_update_field_in_csf_header (int32_t hnd, char *tag, char *contents)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s  Contents = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag, contents);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (csf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (csf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), csf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), csf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (csf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), csf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = csf[hnd].header.header_size - ftello64 (csf[hnd].fp);

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_csf_header");
      exit (-1);
    }

  if (!fread (buffer, size, 1, csf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Now we add the tagged field at its previous location.  */

  if (fseeko64 (csf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to updating user field in CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Create the tagged field with the new contents.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s =\n%s\n}", tag, contents);
    }
  else
    {
      sprintf (info, "[%s] = %s", tag, contents);
    }


  /*  Make sure we're not going to exceed our header size.  */

  pos += size - (strlen (info) + 1);

  if (pos >= csf[hnd].header.header_size)
    {
      sprintf (czmil_error.info, _("File : %s\nAttempt to exceed header size limit!\n"), csf[hnd].path);
      free (buffer);
      return (czmil_error.czmil = CZMIL_CSF_EXCEEDED_HEADER_SIZE_ERROR);
    }


  /*  Add the tagged field.  */

  fprintf (csf[hnd].fp, "%s\n", info);


  /*  Now write the remainder of the header back out.  */

  fwrite (buffer, size - (strlen (info) + 1), 1, csf[hnd].fp);
  fflush (csf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CSF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  csf[hnd].app_tags_pos = 0;
  czmil_read_csf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (csf[hnd].fp, csf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after updating user field in CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_idl_delete_field_from_csf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd             =    The CSF file handle
                - idl_tag         =    The unterminated ASCII tag value, without brackets.
                                       Include the leading brace, {, for multi-line fields.
                - tag_length      =    The length of the unterminated IDL tag string

 - Returns:
                - CZMIL_SUCCESS
                - Error value from czmil_delete_field_from_csf_header

 - Caveats      This is a wrapper function that can be called from IDL that calls
                czmil_delete_field_from_csf_header.  We're just null terminating the tag
                name.  This function should only be used by HydroFusion.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_idl_delete_field_from_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length)
{
  char tag[1024];


  /*  Null terminate the unterminated IDL tag name.  */

  strncpy (tag, idl_tag, tag_length);
  tag[tag_length] = 0;


  /*  Call the normal delete_field_from_csf_header function.  */

  return (czmil_delete_field_from_csf_header (hnd, tag));
}



/********************************************************************************************/
/*!

 - Function:    czmil_delete_field_from_csf_header

 - Purpose:     Deletes an application defined tagged ASCII field from the CSF header.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        07/09/16

 - Arguments:
                - hnd            =    The CSF file handle
                - tag            =    The ASCII tag value, without brackets.  Include the
                                      leading brace, {, for multi-line fields.

 - Returns:
                - CZMIL_SUCCESS
                - CZMIL_CSF_HEADER_READ_FSEEK_ERROR
                - CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR
                - CZMIL_NO_MATCHING_TAG_ERROR
                - Error value from czmil_write_csf_header

 - Caveats

*********************************************************************************************/

CZMIL_DLL int32_t czmil_delete_field_from_csf_header (int32_t hnd, char *tag)
{
  char varin[8192], info[8192], *buffer;
  int64_t pos;
  int32_t size, rem_size;
  uint8_t match = 0;


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d Handle = %d  Tag = %s\n", __FILE__, __FUNCTION__, __LINE__, hnd, tag);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  /*  Check for CZMIL_UPDATE mode.  */

  if (csf[hnd].mode != CZMIL_UPDATE)
    {
      sprintf (czmil_error.info, _("File : %s\nNot opened for update.\n"), csf[hnd].path);
      return (czmil_error.czmil = CZMIL_CSF_NOT_OPEN_FOR_UPDATE_ERROR);
    }


  /*  Create the tagged field.  */

  if (tag[0] == '{')
    {
      sprintf (info, "%s", tag);
    }
  else
    {
      sprintf (info, "[%s]", tag);
    }


  /*  Position to the beginning of the file.  */

  if (fseeko64 (csf[hnd].fp, 0LL, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to reading CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Save the position so we can back up over the pre-existing tagged field.  */

  pos = ftello64 (csf[hnd].fp);


  /*  Read the tagged ASCII header data.  Note, we're using czmil_ngets instead of fgets since we really don't want the
      CR/LF in the strings.  */

  while (czmil_ngets (varin, sizeof (varin), csf[hnd].fp))
    {
      /*  Check for the tagged field.  */

      if (strstr (varin, info))
        {
          match = 1;


          /*  Deal with multi-line tagged info.  */

          if (varin[0] == '{' && tag[0] == '{')
            {
              while (fgets (varin, sizeof (varin), csf[hnd].fp))
                {
                  if (varin[0] == '}') break;
                }
            }

          break;
        }


      /*  Save the position so we can back up over the pre-existing tagged field.  */

      pos = ftello64 (csf[hnd].fp);


      /*  Check for end of header.  */

      if (strstr (varin, N_("[END OF HEADER]"))) break;
    }


  /*  If we didn't find the tag, return an error.  */

  if (!match)
    {
      sprintf (czmil_error.info, _("File : %s\nApplication defined tag %s not found.\n"), csf[hnd].path, tag);
      return (czmil_error.czmil = CZMIL_NO_MATCHING_TAG_ERROR);
    }


  /*  Read the rest of the header as a block of characters.  */

  size = csf[hnd].header.header_size - pos;

  buffer = (char *) malloc (size);

  if (buffer == NULL)
    {
      perror ("Allocating header buffer memory in czmil_update_field_in_csf_header");
      exit (-1);
    }


  //  Clear the buffer

  memset (buffer, ' ', size);


  //  We only want to read the remaining part, not the piece we are deleting.

  rem_size = csf[hnd].header.header_size - ftello64 (csf[hnd].fp);
  if (!fread (buffer, rem_size, 1, csf[hnd].fp))
    {
      sprintf (czmil_error.info, "Bad return in file %s, function %s at line %d.  This should never happen!", __FILE__, __FUNCTION__, __LINE__ - 2);
      free (buffer);
      return (czmil_error.czmil = CZMIL_GCC_IGNORE_RETURN_VALUE_ERROR);
    }


  /*  Seek to the position of the beginning of the tag that we are deleting.  */

  if (fseeko64 (csf[hnd].fp, pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek prior to deleting user field from CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      free (buffer);
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


  /*  Now write the remainder of the header back out (over the deleted field).  Note that I'm not worried about not having enough remaining
      header to overwrite the field we're deleting.  That's because the ASCII header is usually much larger than the actual data that is stored
      in it.  The remainder of the header is filled with blanks so this shouldn't be a problem.  */

  fwrite (buffer, size - (strlen (info) + 1), 1, csf[hnd].fp);
  fflush (csf[hnd].fp);


  /*  Free the buffer we allocated.  */

  free (buffer);


  /*  Now read the header to force a load of the application defined tags into the app_tags buffer of the internal CSF structure.
      IMPORTANT NOTE: We have to zero out the app_tags_pos position so that it will not tack the revised application defined tags
      onto the existing tags.  */

  csf[hnd].app_tags_pos = 0;
  czmil_read_csf_header (hnd);


  /*  Seek back to wherever we were.  */

  if (fseeko64 (csf[hnd].fp, csf[hnd].pos, SEEK_SET) < 0)
    {
      sprintf (czmil_error.info, _("File : %s\nError during fseek after deleting user field from CSF header :\n%s\n"), csf[hnd].path, strerror (errno));
      return (czmil_error.czmil = CZMIL_CSF_HEADER_READ_FSEEK_ERROR);
    }


#ifdef CZMIL_DEBUG
  fprintf (CZMIL_DEBUG_OUTPUT, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
  fflush (CZMIL_DEBUG_OUTPUT);
#endif


  return (czmil_error.czmil = CZMIL_SUCCESS);
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_errno

 - Purpose:     Returns the latest CZMIL error condition code

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - void

 - Returns:
                - error condition code

 - Caveats:     The only thing this is good for at present is to determine if, when you opened
                the file, the library version was older than the file.  That is, if
                CZMIL_NEWER_FILE_VERSION_WARNING has been set when you called czmil_open_file.
                Otherwise, you can just use czmil_perror or czmil_strerror to get the last
                error information.

*********************************************************************************************/

CZMIL_DLL int32_t czmil_get_errno ()
{
  return (czmil_error.czmil);
}



/********************************************************************************************/
/*!

 - Function:    czmil_strerror

 - Purpose:     Returns the error string related to the latest error.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - void

 - Returns:
                - Error message

*********************************************************************************************/

CZMIL_DLL char *czmil_strerror ()
{
  return (czmil_error.info);
}



/********************************************************************************************/
/*!

 - Function:    czmil_perror

 - Purpose:     Prints (to stderr) the latest error messages.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - void

 - Returns:
                - void

*********************************************************************************************/

CZMIL_DLL void czmil_perror ()
{
  fprintf (stderr, "%s", czmil_strerror ());
  fflush (stderr);
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_version

 - Purpose:     Returns the CZMIL library version string

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - void

 - Returns:
                - version string

*********************************************************************************************/

CZMIL_DLL char *czmil_get_version ()
{
  return (CZMIL_VERSION);
}



/*******************************************************************************************/
/*!

  - Module Name:        czmil_get_version_numbers

  - Programmer(s):      Jan C. Depner

  - Date Written:       August 23, 2012

  - Purpose:            Parses the version string (from a file or the library) and extracts
                        the integer major and minor version numbers.

  - Arguments:
                        - version         =   Version string containing "library Vx.xx"
                        - major_version   =   Major version number
                        - minor_version   =   Major version number

  - Return Value:
                        - void

*********************************************************************************************/

CZMIL_DLL void czmil_get_version_numbers (char *version, uint16_t *major_version, uint16_t *minor_version)
{
  char tmp[200], major[10], minor[10];
  int32_t i, j;
  uint8_t start;


  strcpy (tmp, strstr (version, N_("V")));

  start = 0;


  /*  I know, I could have done this with strtok but I don't know how long that's going to hang
      around since everyone hates it.  Also, it can't be used on static strings.  */

  for (i = 0, j = 0 ; i < strlen (tmp) ; i++)
    {
      if (start)
        {
          if (tmp[i] == ' ')
            {
              minor[j] = 0;
              break;
            }
          else
            {
              minor[j] = tmp[i];
              j++;
            }
        }
      else
        {
          if (tmp[i] == '.')
            {
              major[j] = 0;
              start = 1;
              j = 0;
            }
          else
            {
              if (tmp[i] != 'V')
                {
                  major[j] = tmp[i];
                  j++;
                }
            }
        }
    }


  sscanf (major, "%hd", major_version);
  sscanf (minor, "%hd", minor_version);
}



/********************************************************************************************/
/*!

 - Function:    czmil_compute_pd_cube

 - Purpose:     Given the d_index_cube value from a CPF record and a probability of false
                alarm index (from 0 to 6) this function computes the probability of 
                detection of a cube based on processing parameters used in HydroFusion.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/30/16

 - Arguments:
                - record         =    The CZMIL CPF record
                - false_alarm    =    A value from 0 to 6 that denotes one of the following
                                      values:
                                      - 0 = 0.01 / (10 ^ 0) = 0.01       = 1 in 100
                                      - 1 = 0.01 / (10 ^ 1) = 0.001      = 1 in 1000
                                      - 2 = 0.01 / (10 ^ 2) = 0.0001     = 1 in 10000
                                      - 3 = 0.01 / (10 ^ 3) = 0.00001    = 1 in 100000
                                      - 4 = 0.01 / (10 ^ 4) = 0.000001   = 1 in 1000000
                                      - 5 = 0.01 / (10 ^ 5) = 0.0000001  = 1 in 10000000
                                      - 6 = 0.01 / (10 ^ 6) = 0.00000001 = 1 in 100000000

 - Returns:
                - Probability of detection as a percentage

*********************************************************************************************/

CZMIL_DLL double czmil_compute_pd_cube (CZMIL_CPF_Data *record, int32_t false_alarm)
{
  static double pd_cube_table[11][7] =
    {
      /*              0.01          0.001        0.0001        0.00001      0.000001     0.0000001     0.00000001  */
      /* 00 */   {0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000, 0.0000000000},
      /* 01 */   {0.0923622481, 0.0182984684, 0.0032738172, 0.0005475314, 0.0000872176, 0.0000133848, 0.0000019940},
      /* 02 */   {0.3720805854, 0.1378054129, 0.0428056851, 0.0117596909, 0.0029487703, 0.0006887189, 0.0001519216},
      /* 03 */   {0.7497337475, 0.4640513062, 0.2360653812, 0.1029552390, 0.0397646007, 0.0139269636, 0.0045006963},
      /* 04 */   {0.9529005062, 0.8185274823, 0.6106384779, 0.3955467952, 0.2255974899, 0.1151983536, 0.0534808346},
      /* 05 */   {0.9962484881, 0.9719184347, 0.8999002717, 0.7688635060, 0.5973816903, 0.4209993413, 0.2702684675},
      /* 06 */   {0.9998804459, 0.9981915125, 0.9887252888, 0.9586392680, 0.8937234402, 0.7883364474, 0.6509915232},
      /* 07 */   {0.9999985205, 0.9999538075, 0.9994827710, 0.9968820208, 0.9876664214, 0.9641219477, 0.9174312881},
      /* 08 */   {0.9999999930, 0.9999995441, 0.9999906965, 0.9999061831, 0.9994159881, 0.9974501081, 0.9915298005},
      /* 09 */   {1.0000000000, 0.9999999983, 0.9999999358, 0.9999989053, 0.9999891469, 0.9999278451, 0.9996479771},
      /* 10 */   {1.0000000000, 1.0000000000, 0.9999999998, 0.9999999951, 0.9999999225, 0.9999992093, 0.9999942801}
    };

  double cube_percentage;

  cube_percentage = 100.0;

  if (record->d_index_cube < 10) cube_percentage = pd_cube_table[record->d_index_cube][false_alarm] * 100.0;

  return (cube_percentage);
}



/********************************************************************************************/
/*!

 - Function:    czmil_dump_cwf_record

 - Purpose:     Print the CZMIL CWF record to stdout.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - hnd            =    The CWF file handle
                - record         =    The CZMIL CWF record
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

CZMIL_DLL void czmil_dump_cwf_record (int32_t hnd, CZMIL_CWF_Data *record, FILE *fp)
{
  int32_t         i, j, year, day, hour, minute, month, mday;
  float           second;
  char            validity_reason_string[1024];
  int32_t         loop[9];


  czmil_cvtime (record->timestamp, &year, &day, &hour, &minute, &second);
  czmil_jday2mday (year, day, &month, &mday);
  month++;

  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("Timestamp : %"PRIu64"    %d-%02d-%02d (%d) %02d:%02d:%05.7f\n"), record->timestamp, year + 1900, month, mday, day,
          hour, minute, second);

  fprintf (fp, _("Scan angle : %f\n"), record->scan_angle);


  fprintf (fp, _("\n********************** Indexes ***********************************\n"));

  for (i = 0 ; i < 9 ; i++)
    {
      loop[i] = 0;
      fprintf (fp, _("\n********************* Channel[%d] *********************************\n"), i);

      fprintf (fp, _("Validity reason : %d\n"), record->validity_reason[i]);
      czmil_get_cwf_validity_reason_string (record->validity_reason[i], validity_reason_string);
      fprintf (fp, _("Validity reason string : %s\n"), validity_reason_string);

      for (j = 0 ; j < cwf[hnd].czmil_max_packets ; j++)
        {
          if (record->channel_ndx[i][j])
            {
              loop[i]++;
              fprintf (fp, N_("%02d - %d\n"), j, record->channel_ndx[i][j]);
            }
        }

      loop[i] *= 64;
    }


  fprintf (fp, _("\n********************* Waveforms **********************************\n"));

  for (i = 0 ; i < 9 ; i++)
    {
      fprintf (fp, _("\n********************* Channel[%d] *********************************\n"), i);

      for (j = 0 ; j < loop[i] ; j += 8) fprintf (fp, N_("%03d - %05d %05d %05d %05d %05d %05d %05d %05d\n"), j,
                                              record->channel[i][j], record->channel[i][j + 1], record->channel[i][j + 2],
                                              record->channel[i][j + 3], record->channel[i][j + 4],
                                              record->channel[i][j + 5], record->channel[i][j + 6],
                                              record->channel[i][j + 7]);
    }

  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    czmil_dump_cpf_record

 - Purpose:     Print the CZMIL CPF record to stdout.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/22/12

 - Arguments:
                - record         =    The CZMIL CPF record
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

CZMIL_DLL void czmil_dump_cpf_record (CZMIL_CPF_Data *record, FILE *fp)
{
  int32_t         i, j, year, day, hour, minute, month, mday;
  float           second;
  char            filter_reason_string[1024], status_string[1024];


  czmil_cvtime (record->timestamp, &year, &day, &hour, &minute, &second);
  czmil_jday2mday (year, day, &month, &mday);
  month++;

  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("Timestamp : %"PRIu64"    %d-%02d-%02d (%d) %02d:%02d:%05.7f\n") , record->timestamp, year + 1900, month, mday, day, hour, minute, second);
  fprintf (fp, _("Off nadir angle : %f\n"), record->off_nadir_angle);
  fprintf (fp, _("Reference latitude : %.11f\n"), record->reference_latitude);
  fprintf (fp, _("Reference longitude : %.11f\n"), record->reference_longitude);
  fprintf (fp, _("Water level : %f\n"), record->water_level);
  fprintf (fp, _("Local vertical datum offset : %f\n"), record->local_vertical_datum_offset);
  fprintf (fp, _("Kd value : %.4f\n"), record->kd);
  fprintf (fp, _("Laser energy : %.4f\n"), record->laser_energy);
  fprintf (fp, _("T0 interest point : %.1f\n"), record->t0_interest_point);
  fprintf (fp, _("D_index_cube : %d\n"), record->d_index_cube);
  fprintf (fp, _("User data : %d\n"), record->user_data);

  for (i = 0 ; i < 9 ; i++)
    {
      fprintf (fp, _("Number of returns for channel[%d] : %d\n"), i, record->returns[i]);
      fprintf (fp, _("Optech waveform processing mode for channel[%d] : %d\n"), i, record->optech_classification[i]);

      for (j = 0 ; j < record->returns[i] ; j++)
        {
          /*  Check for a filter_reason of CZMIL_REPROCESSING_BUFFER.  If it's set just output a single line since none of the 
              data will be valid anyway.  */

          if (record->channel[i][j].filter_reason == CZMIL_REPROCESSING_BUFFER)
            {
              fprintf (fp, _("Channel[%d] return[%d] = CZMIL REPROCESSING BUFFER\n"), i, j);              
            }
          else
            {
              fprintf (fp, _("Channel[%d] return[%d] latitude : %.11f\n"), i, j, record->channel[i][j].latitude);
              fprintf (fp, _("Channel[%d] return[%d] longitude : %.11f\n"), i, j, record->channel[i][j].longitude);
              fprintf (fp, _("Channel[%d] return[%d] elevation : %f\n"), i, j, record->channel[i][j].elevation);
              fprintf (fp, _("Channel[%d] return[%d] interest point : %.1f\n"), i, j, record->channel[i][j].interest_point);
              fprintf (fp, _("Channel[%d] return[%d] interest point rank : %d\n"), i, j, record->channel[i][j].ip_rank);
              fprintf (fp, _("Channel[%d] return[%d] reflectance : %f\n"), i, j, record->channel[i][j].reflectance);
              fprintf (fp, _("Channel[%d] return[%d] horizontal uncertainty : %f\n"), i, j, record->channel[i][j].horizontal_uncertainty);
              fprintf (fp, _("Channel[%d] return[%d] vertical uncertainty : %f\n"), i, j, record->channel[i][j].vertical_uncertainty);
              fprintf (fp, _("Channel[%d] return[%d] status : 0x%x\n"), i, j, record->channel[i][j].status);
              czmil_get_cpf_return_status_string (record->channel[i][j].status, status_string);
              fprintf (fp, _("Channel[%d] return[%d] status string : %s\n"), i, j, status_string);
              fprintf (fp, _("Channel[%d] return[%d] classification : %d\n"), i, j, record->channel[i][j].classification);
              fprintf (fp, _("Channel[%d] return[%d] probability of detection : %f\n"), i, j, record->channel[i][j].probability);
              fprintf (fp, _("Channel[%d] return[%d] filter reason : %d\n"), i, j, record->channel[i][j].filter_reason);
              czmil_get_cpf_filter_reason_string (record->channel[i][j].filter_reason, filter_reason_string);
              fprintf (fp, _("Channel[%d] return[%d] filter reason string : %s\n"), i, j, filter_reason_string);
              fprintf (fp, _("Channel[%d] return[%d] d_index : %d\n"), i, j, record->channel[i][j].d_index);
            }
        }
    }

  for (i = 0 ; i < 7 ; i++)
    {
      fprintf (fp, _("Bare earth latitude for channel[%d] : %.11f\n"), i, record->bare_earth_latitude[i]);
      fprintf (fp, _("Bare earth longitude for channel[%d] : %.11f\n"), i, record->bare_earth_longitude[i]);
      fprintf (fp, _("Bare earth elevation for channel[%d] : %f\n"), i, record->bare_earth_elevation[i]);
    }


  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    czmil_dump_csf_record

 - Purpose:     Print the CZMIL CSF record to stdout.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/22/12

 - Arguments:
                - record         =    The CZMIL CSF record
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

CZMIL_DLL void czmil_dump_csf_record (CZMIL_CSF_Data *record, FILE *fp)
{
  int32_t         i, year, day, hour, minute, month, mday;
  float           second;


  czmil_cvtime (record->timestamp, &year, &day, &hour, &minute, &second);
  czmil_jday2mday (year, day, &month, &mday);
  month++;

  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("Timestamp : %"PRIu64"    %d-%02d-%02d (%d) %02d:%02d:%05.7f\n") , record->timestamp, year + 1900, month, mday, day, hour, minute, second);
  fprintf (fp, _("Scan angle : %f\n"), record->scan_angle);
  fprintf (fp, _("Latitude : %.11f\n"), record->latitude);
  fprintf (fp, _("Longitude : %.11f\n"), record->longitude);
  fprintf (fp, _("Altitude : %f\n"), record->altitude);
  fprintf (fp, _("Roll : %f\n"), record->roll);
  fprintf (fp, _("Pitch : %f\n"), record->pitch);
  fprintf (fp, _("Heading : %f\n"), record->heading);
  for (i = 0 ; i < 9 ; i++) fprintf (fp, _("Range[%d] : %f\n"), i, record->range[i]);
  for (i = 0 ; i < 9 ; i++) fprintf (fp, _("Range in water[%d] : %f\n"), i, record->range_in_water[i]);
  for (i = 0 ; i < 9 ; i++) fprintf (fp, _("Intensity[%d] : %f\n"), i, record->intensity[i]);
  for (i = 0 ; i < 9 ; i++) fprintf (fp, _("Intensity in water[%d] : %f\n"), i, record->intensity_in_water[i]);

  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    czmil_dump_cif_record

 - Purpose:     Print the CZMIL CIF record to stdout.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - record         =    The CZMIL CIF record
                - fp             =    The file pointer where you want to dump the record.
                                      This can be stderr, stdout, or a file that you specify.

 - Returns:
                - void

*********************************************************************************************/

static void czmil_dump_cif_record (CZMIL_CIF_Data *record, FILE *fp)
{
  fprintf (fp, N_("\n******************************************************************\n"));
  fprintf (fp, _("CPF record address : %"PRIu64"\n"), record->cpf_address);
  fprintf (fp, _("CWF record address : %"PRIu64"\n"), record->cwf_address);
  fprintf (fp, _("CPF buffer size : %d\n"), record->cpf_buffer_size);
  fprintf (fp, _("CWF buffer size : %d\n"), record->cwf_buffer_size);

  fflush (fp);
}



/********************************************************************************************/
/*!

 - Function:    czmil_cvtime

 - Purpose:     Convert from CZMIL timestamp to year, day of year, hour, minute, second.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - timestamp      =    Microseconds from epoch (Jan. 1, 1970)
                - year           =    4 digit year - 1900
                - jday           =    day of year
                - hour           =    hour of day
                - minute         =    minute of hour
                - second         =    seconds of minute

 - Returns:
                - void

 - Caveats:     This function returns the year as a 2 digit year (offset from 1900).
                Add 1900 to get the actual 4 digit year.

*********************************************************************************************/
 
CZMIL_DLL void czmil_cvtime (int64_t micro_sec, int32_t *year, int32_t *jday, int32_t *hour, int32_t *minute, float *second)
{
  struct tm            time_struct, *time_ptr = &time_struct;
  time_t               tv_sec;
  int32_t              msec;


  tv_sec = micro_sec / 1000000;


  if (!tz_set)
    {
#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif

      tz_set = 1;
    }
    

  time_ptr = localtime (&tv_sec);

  msec = micro_sec % 1000000;

  *year = (short) time_ptr->tm_year;
  *jday = (short) time_ptr->tm_yday + 1;
  *hour = (short) time_ptr->tm_hour;
  *minute = (short) time_ptr->tm_min;
  *second = (float) time_ptr->tm_sec + (float) ((double) msec / 1000000.);
}



/********************************************************************************************/
/*!

 - Function:    czmil_inv_cvtime

 - Purpose:     Convert from year, day of year, hour, minute, second to microseconds from
                01-01-1970.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - year           =    4 digit year - 1900
                - jday           =    day of year
                - hour           =    hour of day
                - minute         =    minute of hour
                - second         =    seconds of minute
                - timestamp      =    Microseconds from epoch (Jan. 1, 1970)

 - Returns:
                - void

 - Caveats:     This function requires the year to be a 2 digit year (offset from 1900).
                Subtract 1900 from the actual 4 digit year prior to calling this function.

*********************************************************************************************/
 
CZMIL_DLL void czmil_inv_cvtime (int32_t year, int32_t jday, int32_t hour, int32_t min, float sec, uint64_t *timestamp)
{
  struct tm                    tm;
  time_t                       tv_sec;
  long                         tv_nsec;


  tm.tm_year = year;

  czmil_jday2mday (year, jday, &tm.tm_mon, &tm.tm_mday);

  tm.tm_hour = hour;
  tm.tm_min = min;
  tm.tm_sec = (int32_t) sec;
  tm.tm_isdst = 0;


  if (!tz_set)
    {
#ifdef _WIN32
  #if defined (__MINGW64__) || defined (__MINGW32__)
      putenv ("TZ=GMT");
      tzset ();
  #else
      _putenv ("TZ=GMT");
      _tzset ();
  #endif
#else
      putenv ("TZ=GMT");
      tzset ();
#endif

      tz_set = 1;
    }
    

  tv_sec = mktime (&tm);
  tv_nsec = (long)(fmod ((double) sec, 1.0) * 1.0e9);

  *timestamp = (int64_t) tv_sec * 1000000L + NINT64 ((double) tv_nsec / 1000.0L);
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_cpf_return_status_string

 - Purpose:     Returns a character string containing a comma separated list of the "set"
                return status names for the supplied CZMIL CPF return status value.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/10/12

 - Arguments:
                - status         =    The CZMIL CPF return status value
                - status_string  =    Address of returned status string

 - Returns:     void

 - Caveats:     The supplied status_string character variable must be large enough to 
                hold the concatenated status names (to be safe just set it to 1024).

*********************************************************************************************/

CZMIL_DLL void czmil_get_cpf_return_status_string (uint16_t status, char *status_string)
{
  status_string[0] = 0;


  if (status & CZMIL_RETURN_MANUALLY_INVAL) strcat (status_string, "CZMIL_RETURN_MANUALLY_INVAL");

  if (status & CZMIL_RETURN_FILTER_INVAL)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_FILTER_INVAL");
    }

  if (status & CZMIL_RETURN_CLASSIFICATION_MODIFIED)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_CLASSIFICATION_MODIFIED");
    }

  if (status & CZMIL_RETURN_SELECTED_FEATURE)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_SELECTED_FEATURE");
    }

  if (status & CZMIL_RETURN_DESIGNATED_SOUNDING)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_DESIGNATED_SOUNDING");
    }

  if (status & CZMIL_RETURN_SUSPECT)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_SUSPECT");
    }

  if (status & CZMIL_RETURN_REFERENCE)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_REFERENCE");
    }

  if (status & CZMIL_RETURN_REPROCESSED)
    {
      if (status_string[0] != 0) strcat (status_string, ", ");
      strcat (status_string, "CZMIL_RETURN_REPROCESSED");
    }


  if (status_string[0] == 0) strcpy (status_string, "CZMIL_RETURN_VALID");
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_cpf_filter_reason_string

 - Purpose:     Returns a character string containing a comma separated list of the "set"
                filter reason names for the supplied CZMIL CPF return filter reason
                value.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        09/10/12

 - Arguments:
                - filter_reason           =    The CZMIL CPF return filter reason value
                - filter_reason_string    =    Address of returned filter reason string

 - Returns:     void

 - Caveats:     The supplied filter_reason_string character variable must be large enough to 
                hold the filter reason name (to be safe just set it to 1024).

                Since the CPF record filter_reason field may be a copy of the CWF record
                validity_reason field this function calls czmil_get_cwf_validity_reason_string
                for values under 16.

*********************************************************************************************/

CZMIL_DLL void czmil_get_cpf_filter_reason_string (uint16_t filter_reason, char *filter_reason_string)
{
  if (filter_reason < 16)
    {
      czmil_get_cwf_validity_reason_string (filter_reason, filter_reason_string);
    }
  else
    {
      filter_reason_string[0] = 0;

      switch (filter_reason)
        {
        case CZMIL_DEEP_LAND:
          sprintf (filter_reason_string, "DEEP CHANNEL LAND RETURN");
          break;

        case CZMIL_PRE_PULSE:
          sprintf (filter_reason_string, "INVALIDATED BY PRE-PULSE FILTER");
          break;

        case CZMIL_AFTER_PULSE:
          sprintf (filter_reason_string, "INVALIDATED BY AFTER-PULSE FILTER");
          break;

        case CZMIL_AUTO_EDIT_SPATIAL_FILTERED:
          sprintf (filter_reason_string, "INVALIDATED BY AUTO-EDIT SPATIAL FILTER");
          break;

        case CZMIL_TURBID_WATER_LAYER_RETURN:
          sprintf (filter_reason_string, "PROBABLE TURBID WATER LAYER RETURN");
          break;

        case CZMIL_WATER_LAST_RETURN_INVALID:
          sprintf (filter_reason_string, "WATER RETURN INVALIDATED WHEN USING LAST RETURN PROCESSING MODE");
          break;

        case CZMIL_WAVEFORM_SATURATED:
          sprintf (filter_reason_string, "RETURN INVALIDATED BECAUSE OF WAVEFORM SATURATION");
          break;

        case CZMIL_INVALIDATED_DEEP_W_SH_PRESENT:
          sprintf (filter_reason_string, "DEEP RETURN INVALIDATED WHEN SHALLOW PRESENT");
          break;

        case CZMIL_SPATIAL_FILTERED:
          sprintf (filter_reason_string, "RETURN INVALIDATED BY LEVEL 2 SPATIAL FILTER");
          break;

        case CZMIL_DEEP_CHANNEL_MULTI_REFLECTION_RETURNS:
          sprintf (filter_reason_string, "RETURN INVALIDATED BY LEVEL 2 DEEP CHANNEL MULTIPLE REFLECTION RETURN FILTER");
          break;

        case CZMIL_DEEP_CHANNEL_MEDIAN_FILTER:
          sprintf (filter_reason_string, "RETURN INVALIDATED BY LEVEL 2 DEEP CHANNEL MEDIAN FILTER");
          break;

        case CZMIL_DIGITIZER_NOISE:
          sprintf (filter_reason_string, "CZMIL RETURN INVALIDATED DUE TO DIGITIZER NOISE");
          break;

        case CZMIL_START_AMP_EXCEEDS_THRESHOLD:
          sprintf (filter_reason_string, "RETURN INVALIDATED BY LEVEL 2 STARTING AMPLITUDE FILTER");
          break;

        case CZMIL_REPROCESSING_BUFFER:
          sprintf (filter_reason_string, "CZMIL REPROCESSING BUFFER");
          break;

        case CZMIL_APP_HP_FILTERED:
          sprintf (filter_reason_string, "RETURN INVALIDATED BY HOCKEY PUCK FILTER APPLICATION");
          break;

        default:
          sprintf (filter_reason_string, "UNKNOWN FILTER REASON");
          break;
        }
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_cwf_validity_reason_string

 - Purpose:     Returns a character string containing a comma separated list of the "set"
                channel waveform validity reason names for the supplied CZMIL CWF channel
                waveform validity reason value.

 - Author:      Jan C. Depner (PFM Software, area.based.editor AT gmail DOT com)

 - Date:        06/24/13

 - Arguments:
                - validity_reason         =    The CZMIL CWF channel waveform validity_reason value
                - validity_reason_string  =    Address of returned validity_reason string

 - Returns:     void

 - Caveats:     The supplied validity_reason_string character variable must be large enough to 
                hold the validity_reason name (to be safe just set it to 1024).

*********************************************************************************************/

CZMIL_DLL void czmil_get_cwf_validity_reason_string (uint16_t validity_reason, char *validity_reason_string)
{
  validity_reason_string[0] = 0;

  switch (validity_reason)
    {
    case CZMIL_WAVEFORM_VALID:
      sprintf (validity_reason_string, "DATA IS VALID");
      break;

    case CZMIL_DATA_ID_CORRUPTED:
      sprintf (validity_reason_string, "WAVEFORM DATA ID CORRUPTED");
      break;

    case CZMIL_TIMESTAMP_INVALID:
      sprintf (validity_reason_string, "WAVEFORM TIMESTAMP INVALID (ESTIMATED)");
      break;

    case CZMIL_SCAN_ANGLE_INVALID:
      sprintf (validity_reason_string, "WAVEFORM SCAN ANGLE INVALID");
      break;

    case CZMIL_DROPPED_PACKET:
      sprintf (validity_reason_string, "DROPPED PACKET (MCWP)");
      break;

    case CZMIL_NUMBER_OF_PACKETS_INVALID:
      sprintf (validity_reason_string, "INVALID NUMBER OF PACKETS FOR WAVEFORM");
      break;

    case CZMIL_PACKET_NUMBER_INVALID:
      sprintf (validity_reason_string, "PACKET NUMBER INVALID");
      break;

    case CZMIL_WAVEFORM_DROPPED:
      sprintf (validity_reason_string, "NO WAVEFORM RECORDED (MCWP)");
      break;

    case CZMIL_CHANNEL_FILE_DOES_NOT_EXIST:
      sprintf (validity_reason_string, "DOWNLOAD COULD NOT LOCATE LIDAR RAW FILE");
      break;

    default:
      sprintf (validity_reason_string, "UNKNOWN VALIDITY REASON");
      break;
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_jday2mday

 - Purpose:     Convert from day of year to month and day.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        08/30/11

 - Arguments:
                - year           =    4 digit year or offset from 1900
                - jday           =    day of year
                - mon            =    month
                - mday           =    day of month

 - Returns:
                - void

 - Caveats:     The returned month value will start at 0 for January.  To get the right
                month value just add 1.

*********************************************************************************************/
 
CZMIL_DLL void czmil_jday2mday (int32_t year, int32_t jday, int32_t *mon, int32_t *mday)
{
  int32_t l_year;


  /*  Months start at zero, days at 1 (go figure).  */

  static const int32_t months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  static const int32_t leap_months[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};


  l_year = year;

  if (year < 1899) l_year += 1900;


  *mday = jday;


  /*  If the year is evenly divisible by 4 but not by 100, or it's evenly divisible by 400, this is a leap year.  */

  if ((!(l_year % 4) && (l_year % 100)) || !(l_year % 400))
    {
      for (*mon = 0 ; *mon < 12 ; (*mon)++)
        {
          if (*mday - leap_months[*mon] <= 0) break;

          *mday -= leap_months[*mon];
        }
    }
  else
    {
      for (*mon = 0 ; *mon < 12 ; (*mon)++)
        {
          if (*mday - months[*mon] <= 0) break;

          *mday -= months[*mon];
        }
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_channel_string

 - Purpose:     Returns a character string containing the name of the channel based on the 
                channel number.

 - Author:      Jan C. Depner (PFM Software, area.based.editor AT gmail DOT com)

 - Date:        06/05/13

 - Arguments:
                - channel_num    =    The CZMIL channel number (from 0 to 8)
                - channel_string =    Address of returned channel string

 - Returns:     void

 - Caveats:     The supplied channel_string character variable must be large enough to 
                hold the name (to be safe just set it to 128).

*********************************************************************************************/

CZMIL_DLL void czmil_get_channel_string (int32_t channel_num, char *channel_string)
{
  switch (channel_num)
    {
    case CZMIL_SHALLOW_CHANNEL_1:
      strcpy (channel_string, "Shallow Channel 1 (central)");
      break;

    case CZMIL_SHALLOW_CHANNEL_2:
      strcpy (channel_string, "Shallow Channel 2");
      break;

    case CZMIL_SHALLOW_CHANNEL_3:
      strcpy (channel_string, "Shallow Channel 3");
      break;

    case CZMIL_SHALLOW_CHANNEL_4:
      strcpy (channel_string, "Shallow Channel 4");
      break;

    case CZMIL_SHALLOW_CHANNEL_5:
      strcpy (channel_string, "Shallow Channel 5");
      break;

    case CZMIL_SHALLOW_CHANNEL_6:
      strcpy (channel_string, "Shallow Channel 6");
      break;

    case CZMIL_SHALLOW_CHANNEL_7:
      strcpy (channel_string, "Shallow Channel 7");
      break;

    case CZMIL_IR_CHANNEL:
      strcpy (channel_string, "IR Channel (8)");
      break;

    case CZMIL_DEEP_CHANNEL:
      strcpy (channel_string, "Deep Channel (9)");
      break;

    default:
      strcpy (channel_string, "Unknown CZMIL Channel Number");
      break;
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_proc_mode_string

 - Purpose:     Returns a character string containing the name of the processing mode
                optech_classification value.

 - Author:      Jan C. Depner (PFM Software, area.based.editor AT gmail DOT com)

 - Date:        12/21/14

 - Arguments:
                - optech_classification  =    The optech_classification value (from 0 to 63)
                - proc_mode_string       =    Address of returned processing mode string

 - Returns:     void

 - Caveats:     The supplied proc_mode_string character variable must be large enough to 
                hold the name (to be safe just set it to 128).

*********************************************************************************************/

CZMIL_DLL void czmil_get_proc_mode_string (int32_t optech_classification, char *proc_mode_string)
{
  switch (optech_classification)
    {
    case CZMIL_OPTECH_CLASS_LAND:
      strcpy (proc_mode_string, "Land");
      break;

    case CZMIL_OPTECH_CLASS_HYBRID:
      strcpy (proc_mode_string, "Hybrid (land and water on waveform)");
      break;

    case CZMIL_OPTECH_CLASS_WATER:
      strcpy (proc_mode_string, "Water");
      break;

    case CZMIL_OPTECH_CLASS_SHALLOW_WATER:
      strcpy (proc_mode_string, "Shallow water");
      break;

    case CZMIL_OPTECH_CLASS_AVERAGE_DEPTH:
      strcpy (proc_mode_string, "Average depth");
      break;

    case CZMIL_OPTECH_CLASS_TURBID_WATER:
      strcpy (proc_mode_string, "Turbid water");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MSR:
      strcpy (proc_mode_string, "Robust Surface Detection missed surface return");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MWSD:
      strcpy (proc_mode_string, "Robust Surface Detection multiple water surface detection");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MWSD_DEEP:
      strcpy (proc_mode_string, "Robust Surface Detection multiple water surface detection (deep channel)");
      break;

    case CZMIL_OPTECH_CLASS_WATER_DEEP_W_DEEP_SURFACE:
      strcpy (proc_mode_string, "Waveform processed using deep channel surface return (deep channel)");
      break;

    case CZMIL_OPTECH_CLASS_SMALL_BOTTOM:
      strcpy (proc_mode_string, "Waveform processed using small bottom return algorithm");
      break;

    default:
    case CZMIL_OPTECH_CLASS_UNDEFINED:
      strcpy (proc_mode_string, "Unknown");
      break;
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_short_proc_mode_string

 - Purpose:     Returns a character string containing the abbreviated or short name of the
                processing mode optech_classification value.  This is useful for labels
                in GUI programs.

 - Author:      Jan C. Depner (PFM Software, area.based.editor AT gmail DOT com)

 - Date:        12/21/14

 - Arguments:
                - optech_classification  =    The optech_classification value (from 0 to 63)
                - proc_mode_string       =    Address of returned short processing mode string

 - Returns:     void

 - Caveats:     The supplied proc_mode_string character variable must be large enough to 
                hold the name (to be safe just set it to 128).

*********************************************************************************************/

CZMIL_DLL void czmil_get_short_proc_mode_string (int32_t optech_classification, char *proc_mode_string)
{
  switch (optech_classification)
    {
    case CZMIL_OPTECH_CLASS_LAND:
      strcpy (proc_mode_string, "Land");
      break;

    case CZMIL_OPTECH_CLASS_HYBRID:
      strcpy (proc_mode_string, "Hybrid land water");
      break;

    case CZMIL_OPTECH_CLASS_WATER:
      strcpy (proc_mode_string, "Water");
      break;

    case CZMIL_OPTECH_CLASS_SHALLOW_WATER:
      strcpy (proc_mode_string, "Shallow water");
      break;

    case CZMIL_OPTECH_CLASS_AVERAGE_DEPTH:
      strcpy (proc_mode_string, "Average depth");
      break;

    case CZMIL_OPTECH_CLASS_TURBID_WATER:
      strcpy (proc_mode_string, "Turbid water");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MSR:
      strcpy (proc_mode_string, "RSD MSR");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MWSD:
      strcpy (proc_mode_string, "RSD MWSD");
      break;

    case CZMIL_OPTECH_CLASS_RSD_MWSD_DEEP:
      strcpy (proc_mode_string, "RSD MWSD (deep)");
      break;

    case CZMIL_OPTECH_CLASS_WATER_DEEP_W_DEEP_SURFACE:
      strcpy (proc_mode_string, "Deep deep surface");
      break;

    case CZMIL_OPTECH_CLASS_SMALL_BOTTOM:
      strcpy (proc_mode_string, "Small bottom");
      break;

    default:
    case CZMIL_OPTECH_CLASS_UNDEFINED:
      strcpy (proc_mode_string, "Unknown");
      break;
    }
}



/********************************************************************************************/
/*!

 - Function:    czmil_get_local_vertical_datum_string

 - Purpose:     Returns a character string containing the description of the applied datum
                based on the vertical datum constant values defined in czmil_macros.h.

 - Author:      Jan C. Depner (PFM Software, area.based.editor AT gmail DOT com)

 - Date:        05/10/16

 - Arguments:
                - local_vertical_datum   =    The vertical datum value
                - datum_string           =    Address of returned short processing mode string

 - Returns:     void

 - Caveats:     The supplied datum_string character variable must be large enough to 
                hold the name (to be safe just set it to 128).

*********************************************************************************************/

CZMIL_DLL void czmil_get_local_vertical_datum_string (uint16_t local_vertical_datum, char *datum_string)
{
  switch (local_vertical_datum)
    {
    case CZMIL_V_DATUM_UNDEFINED:
      strcpy (datum_string, "No vertical datum shift has been applied");
      break;

    default:
    case CZMIL_V_DATUM_UNKNOWN:
      strcpy (datum_string, "Unknown vertical datum");
      break;

    case CZMIL_V_DATUM_MLLW:
      strcpy (datum_string, "Mean lower low water");
      break;

    case CZMIL_V_DATUM_MLW:
      strcpy (datum_string, "Mean low water");
      break;

    case CZMIL_V_DATUM_ALAT:
      strcpy (datum_string, "Approximate Lowest Astronomical Tide");
      break;

    case CZMIL_V_DATUM_ESLW:
      strcpy (datum_string, "Equatorial Springs Low Water");
      break;

    case CZMIL_V_DATUM_ISLW:
      strcpy (datum_string, "Indian Springs Low Water");
      break;

    case CZMIL_V_DATUM_LAT:
      strcpy (datum_string, "Lowest Astronomical Tide");
      break;

    case CZMIL_V_DATUM_LLW:
      strcpy (datum_string, "Lowest Low Water");
      break;

    case CZMIL_V_DATUM_LNLW:
      strcpy (datum_string, "Lowest Normal Low Water");
      break;

    case CZMIL_V_DATUM_LWD:
      strcpy (datum_string, "Low Water Datum");
      break;

    case CZMIL_V_DATUM_MLHW:
      strcpy (datum_string, "Mean Lower High Water");
      break;

    case CZMIL_V_DATUM_MLLWS:
      strcpy (datum_string, "Mean Lower Low Water Springs");
      break;

    case CZMIL_V_DATUM_MLWN:
      strcpy (datum_string, "Mean Low Water Neap");
      break;

    case CZMIL_V_DATUM_MSL:
      strcpy (datum_string, "Mean Sea Level");
      break;
    }
}

