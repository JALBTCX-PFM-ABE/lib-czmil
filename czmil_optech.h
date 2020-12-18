
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



#ifndef __CZMIL_OPTECH_H__
#define __CZMIL_OPTECH_H__

#ifdef  __cplusplus
extern "C" {
#endif


  /*!  The following structures are used exclusively by HydroFusion to define fields for the czmil_create_cwf_file,
       czmil_create_cpf_file, and czmil_create_csf_file.  Since only the HydroFusion program will be using the
       czmil_create functions this should be of no interest to other application programmers.  The following structure
       is used by the czmil_create_cwf_file function to define certain fields for the CWF file.  The CWH refers to a
       preliminary CZMIL Waveform Header file that was used for testing.  This defines the record/index section of the
       CWH file.  */

  typedef struct
  {
    uint32_t      shot_id;                                   /*!<  The shot ID (not synonymous with record number).  */
    uint64_t      timestamp;                                 /*!<  Timestamp of the shot in microseconds from January 1, 1970.  */
    uint8_t       T0[80];                                    /*!<  The T0 waveform as 64, 10 bit values.  */
    float         scan_angle;                                /*!<  Scanner angle.  */
    uint8_t       number_of_packets[9];                      /*!<  Number of packets per channel.  */
    uint64_t      address;                                   /*!<  The index into the preliminary CZMIL 10 bit, packed waveform
                                                                   data file (not used).  */
  } CZMIL_CWH_Data;


  /*!

      CZMIL Waveform Raw data structure.

      This structure is used to pass the raw waveform data to the czmil_write_cwf_record function.  */

  typedef struct
  {
    uint32_t             shot_id;                            /*!<  MCWP shot ID number.  */
    uint64_t             timestamp;                          /*!<  Microseconds from January 01, 1970.  */
    uint8_t              T0[80];                             /*!<  64, 10 bit packed T0 waveform values.  */
    float                scan_angle;                         /*!<  Scan angle.  */
    uint8_t              number_of_packets[9];               /*!<  Number of packets for each channel (shallow 1 through 7, IR, deep).  */


    /*  Added in version 2.0  */

    uint16_t             validity_reason[9];                 /*!< (0-15) Per channel waveform validity reason (see czmil_macros.h).  */
  } CZMIL_WAVEFORM_RAW_Data;


  /*  These structures are used by Optech's HydroFusion when calling external programs.  */

  /*
    Inter-process shared memory data structure:
    The key is defined as "P" if the field is set by the external program or "H" if it's set by HydroFusion.
  */

  typedef struct
  {                                                          /*    Key                         Definition  */
    char                 pfm_name[1024];                     /*!<  (H)   Name of PFM structure (handle file) from which data is to be retrieved.  */
    char                 file_name[100][1024];               /*!<  (H)   Array of fully qualified file names for which data is to be retrieved.  */
    uint16_t             flightline_number[100];             /*!<  (H)   HydroFusion flightline number for the associated file name.  */
    uint8_t              file_count;                         /*!<  (H)   Number of files in array of file names.  */
    double               poly_x[2000];                       /*!<  (H)   Array of longitudes of the requested polygon vertices.  */
    double               poly_y[2000];                       /*!<  (H)   Array of latitudes of the requested polygon vertices.  */
    int32_t              poly_count;                         /*!<  (H)   Number of vertices in the polygon.  */
    char                 shared_memory_key[512];             /*!<  (P)   Shared memory key for data transfer shared memory block.  */
    int32_t              shared_memory_count;                /*!<  (P)   Number of points loaded into data transfer shared memory block.  */
    uint8_t              finished_flag;                      /*!<  (H)   Set to 1 when HydroFusion has attached the data transfer shared memory block.  */
    uint8_t              proc_state;                         /*!<  (P)   Set to 1, 2, or 3 depending on program.  */
    uint8_t              proc_percent;                       /*!<  (P)   Percent of process doen for the proc_state.  */
    char                 error_string[4096];                 /*!<  (P)   Error string on any type of error (shared_memory_count will be set to -1).  */
  } HF_CZMIL_IPC_SHARED_MEMORY;


  /*  PFM XYZ subset data structure.  */

  typedef struct
  {
    uint16_t         flightline_number;                      /*!<  Flightline number passed from HF.  */
    uint32_t         shot_id;                                /*!<  Record number.  */
    uint8_t          channel_number;
    uint8_t          return_number;
    double           latitude;
    double           longitude;                              
    double           elevation;
    float            reflectance;
    float            kd;
    uint8_t          optech_classification;
    float            interest_point;
    uint32_t         classification;
  } HF_CZMIL_XYZ_SUB_Data;


  /*  PFM CSF subset data structure.  */

  typedef struct
  {
    CZMIL_CSF_Data   csf;                                    /*!<  CSF record.  */
    uint16_t         status[9][2];                           /*!<  Status fields of first and last (not counting CZMIL_REPROCESSING_BUFFER) returns
                                                                   from associated CPF records.  */
    uint16_t         flightline_number;                      /*!<  Flightline number passed from HF.  */
    uint32_t         shot_id;                                /*!<  Record number.  */
    uint8_t          optech_classification[9];               /*!<  Processing mode per channel.  */
    uint32_t         classification[9][2];                   /*!<  Classification value of first and last (not counting CZMIL_REPROCESSING_BUFFER) returns
                                                                   from associated CPF records.  */
  } HF_CZMIL_CSF_SUB_Data;


  /*!  CZMIL Public function declarations (HydroFusion use only).  */

  CZMIL_DLL int32_t czmil_create_cwf_file (char *idl_path, int32_t path_length, CZMIL_CWF_Header *cwf_header, int32_t io_buffer_size);
  CZMIL_DLL int32_t czmil_create_cpf_file (char *idl_path, int32_t path_length, CZMIL_CPF_Header *cpf_header, int32_t io_buffer_size);
  CZMIL_DLL int32_t czmil_create_csf_file (char *idl_path, int32_t path_length, CZMIL_CSF_Header *cpf_header, int32_t io_buffer_size);

  CZMIL_DLL int32_t czmil_abort_cpf_file (int32_t hnd);

  CZMIL_DLL int32_t czmil_idl_open_cwf_file (char *idl_path, int32_t path_length, CZMIL_CWF_Header *cwf_header, int32_t mode);
  CZMIL_DLL int32_t czmil_idl_open_cpf_file (char *idl_path, int32_t path_length, CZMIL_CPF_Header *cpf_header, int32_t mode);
  CZMIL_DLL int32_t czmil_idl_open_csf_file (char *idl_path, int32_t path_length, CZMIL_CSF_Header *csf_header, int32_t mode);

  CZMIL_DLL int32_t czmil_write_cwf_record_array (int32_t hnd, int32_t num_supplied, CZMIL_WAVEFORM_RAW_Data *waveform, uint8_t *data);
  CZMIL_DLL int32_t czmil_write_cwf_record (int32_t hnd, CZMIL_WAVEFORM_RAW_Data *waveform, uint8_t *data);
  CZMIL_DLL int32_t czmil_write_cpf_record_array (int32_t hnd, int32_t recnum, int32_t num_supplied, CZMIL_CPF_Data *record_array);
  CZMIL_DLL int32_t czmil_write_cpf_record (int32_t hnd, int32_t recnum, CZMIL_CPF_Data *record);
  CZMIL_DLL int32_t czmil_write_csf_record_array (int32_t hnd, int32_t num_supplied, CZMIL_CSF_Data *record_array);
  CZMIL_DLL int32_t czmil_write_csf_record (int32_t hnd, int32_t recnum, CZMIL_CSF_Data *record);

  CZMIL_DLL int32_t czmil_idl_add_field_to_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);
  CZMIL_DLL int32_t czmil_idl_add_field_to_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);
  CZMIL_DLL int32_t czmil_idl_add_field_to_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);

  CZMIL_DLL int32_t czmil_idl_get_field_from_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents);
  CZMIL_DLL int32_t czmil_idl_get_field_from_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents);
  CZMIL_DLL int32_t czmil_idl_get_field_from_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *contents);

  CZMIL_DLL int32_t czmil_idl_update_field_in_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);
  CZMIL_DLL int32_t czmil_idl_update_field_in_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);
  CZMIL_DLL int32_t czmil_idl_update_field_in_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length, char *idl_contents, int32_t contents_length);

  CZMIL_DLL int32_t czmil_idl_delete_field_from_cwf_header (int32_t hnd, char *idl_tag, int32_t tag_length);
  CZMIL_DLL int32_t czmil_idl_delete_field_from_cpf_header (int32_t hnd, char *idl_tag, int32_t tag_length);
  CZMIL_DLL int32_t czmil_idl_delete_field_from_csf_header (int32_t hnd, char *idl_tag, int32_t tag_length);


#ifdef  __cplusplus
}
#endif


#endif
