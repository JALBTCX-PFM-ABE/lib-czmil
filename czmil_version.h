
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



#ifndef CZMIL_VERSION

#define     CZMIL_VERSION     "PFM Software - CZMIL library V3.16 - 08/18/19"

#endif


/*!< <pre>

    Version 0.10
    07/02/12
    Jan Depner

    First test version.  Happy (real) independence day.


    Version 1.00
    10/12/12
    Jan Depner

    First working version.


    Version 1.01
    06/05/13
    Jan C. Depner (PFM Software)

    Added czmil_get_channel_string to return the ASCII channel name based on the channel number.


    Version 1.11
    09/11/13
    Jan C. Depner (PFM Software)

    This was a temporary version that was hacked from version 2.11 to allow Optech to get the benefit of the on-the-fly CIF file
    generation without having too update HydroFusion to version 2.  The actual date this was done was 09/11/14 but we're leaving
    the date set to (bogus) 09/11/13 to keep our tracking software from going nuts.  If you see a CZMIL file with this version
    number it is the same as any other version 1 CZMIL file.


    Version 2.00
    06/24/13
    Jan C. Depner (PFM Software)

    - Added shot status to CWF record to accomodate the "shot bad" (filter invalid), "time bad", "scan angle bad", and "range bad" flags.
    - Added czmil_get_cwf_shot_status_string function.
    - Added check and correct for CWF time regression error (at the request of Optech International).
    - Store csf[hnd].range_max for invalid CSF ranges and return -1.0 on read.
    - Store cwf[hnd].range_max for invalid CWF ranges and return -1.0 on read.
    - Removed log file functionality but left in place as comments.
    - Moved the code to break out the major and minor version numbers from the file version in the czmil_write_XXX_header functions
      so that we don't write out major version 2 stuff in major version 1 files.
    - Added range_in_water, intensity, and intensity_in_water to the CSF records.
    - Added probability (probability of detection) to the CPF records (one per return).
    - Added validity_reason (reason a return is marked invalid) to the CPF records (one per return).
    - Added major version 2 tests for all new fields on read, write, and write_header.
    - Corrected bug in czmil_open_cwf_file and czmil_open_cpf_file when the cif file has disappeared.


    Version 2.01
    08/15/13
    Jan C. Depner (PFM Software)

    - Made sure that tzset was called if you used the cvtime and inv_cvtime functions without using any of the other
      CZMIL functions.


    Version 2.02
    12/12/13
    Jan C. Depner (PFM Software)

    - Added CZMIL_RETURN_CLASS_LAND, CZMIL_RETURN_CLASS_WATER, and CZMIL_RETURN_CLASS_SHALLOW_WATER to czmil_macros.h.


    Version 2.03
    12/19/13
    Jan C. Depner (PFM Software)

    - Fixed major version bug in czmil_get_version_numbers.


    Version 2.04
    01/18/14
    Jan C. Depner (PFM Software)

    - Now using setvbuf and freads instead of no buffering and fseeks to speed up reading of CWF and CPF files in
      czmil_create_cif_file.
    - Added CZMIL_READONLY_SEQUENTIAL mode for using setvbuf on reads of CWF, CPF, and CSF files.
    - Added setvbuf to CWF, CPF, and CSF read operations (obviously).
    - Added processing_mode (land, water, shallow water, TBD) to the CPF records (one per return).  Note that we only
      have to test for major version 2.0 for this field since Optech has not created any 2.0 or better files.  Another 
      note - for pre 2.0 files we will be taking the processing mode out of the classification field since that was 
      where Optech was storing it.
    - Changed the CZMIL_RETURN_CLASS... constant names to CZMIL_RETURN_PROC...


    Version 2.05
    03/06/14
    Jan C. Depner (PFM Software)

    - Fixed bug introduced when trying to stuff CPF classification into processing_mode in pre 2.0 files.


    Version 2.06
    05/07/14
    Jan C. Depner (PFM Software)

    - Fixed bug in czmil_perror (found by the GCC 4.8.2 compiler in Kubuntu 14.04).
    - Added memset in czmil_write_cif_record to clear the buffer prior to filling it with bit-packed data.
    - Added return value checks for fread to get rid of GCC warnings.  These should never happen.
    

    Version 2.07
    07/21/14
    Jan C. Depner (PFM Software)

    - Switched from nvtypes.h data types to the more standard stdint.h.
    - Now uses PRId64 (from inttypes.h) instead of NV_INT64_SPECIFIER (from nvtypes.h) for printing out 64 bit integers.
    

    Version 2.08
    Jan C. Depner (PFM Software)
    07/29/14

    - Fixed possible memory leak discovered by cppcheck.


    Version 2.09
    Jan C. Depner (PFM Software)
    08/08/14

    - Now creates CIF file while creating CWF and updates it when the CPF file is created.  This should be faster
      than having to reread the CPF and CWF files after we create them.
    - Mocified czmil_create_cif_file to only be called when the CIF file has been deleted or corrupted.
    - Fixed czmil_create_cif_file to work properly with more CWF records than CPF records.


    Version 2.10
    Jan C. Depner (PFM Software)
    09/05/14

    - Changes agreed upon by Optech and ACE for v2 (extra fields, see czmil.h).


    Version 2.11
    Hemanth Kalluri (Optech International), Jan Depner (PFM Software)
    09/11/14

    - Caught really bad array out of bounds access in czmil_compress_cwf_record second difference computation.
    - Changes to support Microsoft Visual C.


    Version 2.12
    Jan C. Depner (PFM Software)
    09/17/14

    Added return number and number of returns to CAF record.


    Version 2.13
    Jan C. Depner (PFM Software)
    10/15/14

    Added new Optech classification (processing mode) definitions to czmil_macros.h.


    Version 2.14
    Jan C. Depner (PFM Software)
    11/10/14

    Added czmil_abort_cpf_file.
    Updated the Optech classification modes and added constants for the filter_reason field.


    Version 2.15
    Jan C. Depner (PFM Software)
    12/11/14

    Added structures and constants to czmil_optech.h to support reading PFM files to retrieve subsets
    of the CZMIL CPF and/or CSF data based on an area in the PFM.  There were no changes to the
    operational code since these structures and constants are only used in external programs that are
    called from HydroFusion (which why they're in czmil_optech.h).  At some point in the future they
    may be used internally though so this was a very convenient place to locate them.


    Version 2.16
    Jan C. Depner (PFM Software)
    12/22/14

    Fixed a bug in czmil_write_cpf_record.  We just started doing reprocessing so this was the first time
    we called czmil_write_cpf_record to update a record instead of appending a new record.


    Version 2.17
    Jan C. Depner (PFM Software)
    01/08/15

    Fixed a bug in czmil_read_cpf_record dealing with pre 2.0 procesing mode.


    Version 2.18
    Jan C. Depner (PFM Software)
    01/20/15

    Added interest_point to HF_CZMIL_XYZ_SUB_Data and shot_id to HF_CZMIL_CSF_SUB_Data.


    Version 2.19
    Jan C. Depner (PFM Software)
    01/27/15

    Fixed bug in czmil_update_cpf_record and czmil_update_cpf_return_status.


    Version 2.20
    Jan C. Depner (PFM Software)
    02/10/15

    Force fflush of I/O buffer in czmil_update_cpf_record, czmil_update_cpf_return_status, and czmil_write_cpf_record
    (for update only) when the external calling program is doing multiple, sequential update calls without a read of
    the record prior to the update.  In other words, the external program is buffering a bunch of updates.


    Version 2.21
    Jan C. Depner (PFM Software)
    03/19/15

    Modified the czmil_write_cpf_header to check to see if the calling application has already defined the wkt field.
    If so it writes that into the header.  If not it writes the standard WGS84/WGS84E well-known text string.  This
    has been done to support the Army Corps of Engineers who adjust their surveys to the NAD83 datum prior to processing
    to create the CPF file.


    Version 2.22
    Jan C. Depner (PFM Software)
    06/26/15

    - Added channel numbers to channel strings for IR and Deep channels.
    - Added useless line in czmil_write_cpf_record to suppress a gcc warning.


    Version 2.23
    Jan C. Depner (PFM Software)
    11/06/15

    - At the request of Optech International, added one decimal place to interest point precision (changed
      CPF_INTEREST_POINT_SCALE from 10.0 to 100.0).


    Version 2.24
    Jan C. Depner (PFM Software)
    04/01/16

    - Added CZMIL_OPTECH_CLASSIFICATION_DEEP_SS constant for processing mode.  This signifies that the deep return was computed using
      the shallow channel surface.


    Version 2.25
    Jan C. Depner (PFM Software)
    04/14/16

    - Replaced CZMIL_RETURN_SELECTED_SOUNDING constant with CZMIL_RETURN_CLASSIFICATION_MODIFIED.  This allows us to track changes in
      classification in ABE without having to use the PFM_MODIFIED flag.  Instead we use the PFM_SELECTED_SOUNDING flag which is
      defined as an application specific flag and, at present, isn't used in ABE with the exception of load/unload.


    Version 2.26
    Jan C. Depner (PFM Software)
    04/25/16

    - Changed czmil_update_cpf_return_status to also update the classification.  Due to changes in the way we process CZMIL data we
      began to update the classification almost as often as the status.  Instead of calling czmil_update_cpf_record and causing
      aliasing of the modifiable floating point values I thought it was better to add classification update to
      czmil_update_cpf_return_status.  Granted, this is a change to an existing API function instead of adding an identical function
      with a single change.  So, in the interest of keeping programmers happy:
      <start mechanical arm waving> WARNING WARNING WARNING!  DANGER WILL ROBINSON, ALIENS APPROACHING!  </end mechanical arm waving
      after taking hit from alien ray weapon>.  Meanwhile, back at the ranch Trapper was hovering between Shiela and Shirley.  Wait,
      I'm mixing my TV shows (not to mention decades).  OK, so it actually shouldn't be a problem since, as the documentation says,
      the application is supposed to read the record, modify the pertinent fields, and then call the update function.  As long as
      the programmer does that, there should be no problem (he said with some trepidation).


    Version 2.27
    Jan C. Depner (PFM Software)
    05/10/16

    - Added czmil_get_local_vertical_datum_string function to return description of local vertical datum based on numeric value
      defined in czmil_macros.h.


    Version 3.00
    Jan C. Depner (PFM Software)
    06/27/16

    - Increased size of TIME_BITS : added one bit to the TIME_BITS constant (now 31).  This will double the maximum time length of a
      CZMIL file from 35 minutes to 70 minutes.  This will add one bit to each shot record in the CWF, CPF, and CSF files.
    - Increased size of CPF_LAT_BITS : added one bit to the CPF_LAT_BITS constant (now 27).  This will double the nominal maximum line
      length from approximately 104 kilometers to 208 kilometers.  The distance limit is nominal since the offset values are actually
      stored in 20,000ths of an arcsecond so the actual distance limit will vary with latitude and direction.  This will add one bit
      to the size of the CPF shot record.
    - Increased size of CPF_LON_BITS : added one bit to the CPF_LON_BITS constant (now 27). This will double the nominal maximum line
      length from approximately 104 kilometers to 208 kilometers. The distance limit is nominal since the offset values are actually
      stored in 20,000ths of an arcsecond so the actual distance limit will vary with latitude and direction.  The longitude offset
      is scaled by the cosine of the integer portion of the latitude so it won't vary as much as would be expected.  This will add
      one bit to the CPF shot record.
    - Increased size of CSF_LAT_BITS : added one bit to the CSF_LAT_BITS constant (now 26).  This will double the nominal maximum line
      length from approximately 104 kilometers to 208 kilometers.  The distance limit is nominal since the offset values are actually
      stored in 10,000ths of an arcsecond so the actual distance limit will vary with latitude and direction.  This will add one bit
      to the size of the CSF record.
    - Increased size of CSF_LON_BITS : added one bit to the CSF_LON_BITS constant (now 26). This will double the nominal maximum line
      length from approximately 104 kilometers to 208 kilometers. The distance limit is nominal since the offset values are actually
      stored in 10,000ths of an arcsecond so the actual distance limit will vary with latitude and direction.  The longitude offset
      is scaled by the cosine of the integer portion of the latitude so it won't vary as much as would be expected.  This will add
      one bit to the CSF record.
    - Decreased the stored size of the ip_rank field of the return from five bits to one bit : the ip_rank field has never been used
      except to store a zero to indicate a water surface return or any integer to indicate a non-water surface return.  The definition
      of this field in the documentation will be changed to reflect the changed use of the field.  The name will not be changed so
      that software changes will not be needed.
    - Added d_index field to each CPF return portion of the CPF shot record : added a new field to the CPF return structure (and storage)
      that contains an indicator of the relative amplitude of the return.  This is actually more like a normalized signal to noise
      ratio.  The field will contain a number from 0.0 to 1.0 stored as a scaled integer (times 1000).  This will require ten extra
      bits per return but the net increase will be six bits per return since the ip_rank field has been decreased from five bits to one
      bit.
    - Added d_index_cube field to the CPF shot record : added a field to the CPF shot record that contains the prediction of the
      normalized signal amplitude from a cube on the seafloor (deep channel).  The field is stored as a value from 0 to 1,023 (requires
      10 bits).
    - Changed the spare field of the CPF shot record to user_data : changed the name of the spare field of the CPF shot record structure
      to user_data. The field may be used by application programs for any purpose.  Since this field has existed in all previous CZMIL
      CPF files (albeit with a different name) the field may be used for any previous version CZMIL CPF file.  The BITS designator
      (e.g. [SPARE BITS])in the headers of previous versioned CPF files will not be modified.
    - Allowed czmil_update_cpf_return_status to update the new user_data field.
    - Added {USER DATA DESCRIPTION to the CPF file header : This field will be an ASCII text field describing the use of the user_data
      field of the CPF shot record.  The field will be left blank if the user_data field is not being used.  This may be added, by an
      application, to previous versioned CPF file headers since older versions of the CZMIL API will ignore it.
    - Added code to handle multi-line header information in the [APPLICATION DEFINED FIELDS] section of the CPF,CWF, and CSF headers.
    - Increased the CPF_HEADER_SIZE, CWF_HEADER_SIZE, and CSF_HEADER_SIZE constants from 65536 to 131072 to handle the added
      {USER DATA DESCRIPTION and possibly more and larger [APPLICATION DEFINED FIELDS] in the headers.
    - Added czmil_idl_delete_field_from_cwf_header, czmil_idl_delete_field_from_cpf_header, czmil_idl_delete_field_from_csf_header,
      czmil_delete_field_from_cwf_header, czmil_delete_field_from_cpf_header, and czmil_delete_field_from_csf_header.
    - Fixed bugs in czmil_update_field_in_cwf_header, czmil_update_field_in_cpf_header,and czmil_update_field_in_csf_header.
    - Changed the default Well-known Text to remove the COMPD_CS prefix and trailing bracket (]).  This caused a problem for QGIS and
      wasn't adding anything but a name.
    - Stripped trailing CR/LF from the last line of multi-line fields in the CWF, CPF, CSF, and CIF headers so that, if we update the
      headers we won't keep adding blank lines.
    - Fixed bug caused by using czmil_short_log2 with increased scale size for interest point.


    Version 3.01
    Jan C. Depner (PFM Software)
    08/30/16

    - Added function czmil_compute_pd_cube to compute the probability of detection of a cube given the CPF record with d_index_cube
      populated and a false alarm probability index value of 0 to 6.


    Version 3.02
    Jan C. Depner (PFM Software)
    10/17/16

    - I finally got a better handle on WKT CRS and discovered that composite CRS's (COMPD_CS) are perfectly acceptable.  GDAL/OGR
      handles them just fine.  The fact that QGIS seems to barf on things that GDAL/OGS can easily translate to Proj.4 strings is
      neither here nor there.  Consequently, I've re-corrected the default WKT CRS and will, hopefully, supply a WGS84/WGS84E and
      NAD83/NAVD88 examples to Optech to be added to HydroFusion.
    - Added filter_reason to the fields that can be updated with czmil_update_cpf_return_status.


    Version 3.03
    Jan C. Depner (PFM Software)
    12/07/16

    - Added a few new filter reasons and processing modes to czmil_macros.h.
    - 75 years to the day since my dad saw his ship strafed and bombed (USS Pennsylvania).  Luckily, it was in dry dock and
      he wasn't on it or I might not have been around to write this software.


    Version 3.04
    Jan C. Depner (PFM Software)
    05/31/17

    - Added optech_classification and classification[9][2] to the HF_CZMIL_CSF_SUB_Data structure.
    - Added classification to the HF_CZMIL_XYZ_SUB_Data structure.


    Version 3.10
    06/19/17
    Brett Goldenbloome (Leidos), Jan Depner (PFM Software)

    - After using the bit_pack and bit_unpack functions for 18 years with no apparent problems, Brett Goldenbloome
      at Leidos, Newport RI discovered that, in extremely rare circumstances, the functions would access one byte
      past the end of the buffer.  I have added an "if" statement (shudder) to correct the problem.


    Version 3.11
    07/07/17
    Michael Novoa (Optimal GEO)

    - updated 'czmil_get_short_proc_mode_string' to include the same classes as 'czmil_get_proc_mode_string'.
    

    Version 3.12
    08/29/17
    Jan Depner (PFM Software)

    - In earlier versions of the CZMIL API we were using an ip_rank of 0 to signify water surface.  When we switched
      to v3.00 Optech International added CZMIL_OPTECH_CLASS_HYBRID processing mode and we had some confusion about
      how to designate water surface, water processed, land processed, etc.  When reading a CPF record, if the
      "classification" is set to anything other than zero, we do nothing.  If the "classification" is set to 0 AND
      the "ip_rank" is 0, we hard-code the "classification" to be 41 (this is a "Water surface" classification based
      on the "Proposed LAS Enhancements to Support Topo-Bathy Lidar", July 17, 2013).  This simplifies water surface
      detection in CZMIL applications.


    Version 3.13
    04/20/18
    Jan Depner (PFM Software), Peter Benton (Teledyne)

    - Added IDL_DEBUG constant so I only have to change czmil_internals.h to turn on debugging log for IDL.
    - Added CZMIL_NO_T0 and CZMIL_IN_USER_EXCLUSION_ZONE filter reasons.


    Version 3.14
    06/04/18
    Jan Depner (PFM Software)

    - Replaced CZMIL_RETURN_USER_FLAG with CZMIL_RETURN_REPROCESSED status constant.  CZMIL_RETURN_USER_FLAG
      was never used and we needed a way to keep track of points that the user has reprocessed.


    Version 3.15
    08/06/19
    Jan Depner (PFM Software)

    - Added the CZMIL_URBAN_? flags.  These aren't used internally but in other programs for flagging and
      filtering data.


    Version 3.16
    08/18/19
    Jan Depner (PFM Software)

    - Modified the CZMIL_URBAN_? flags to include a "soft hit" flag for channels with 5 or more valid returns.

</pre>*/
