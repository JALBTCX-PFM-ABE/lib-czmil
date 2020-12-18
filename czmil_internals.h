
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



#ifndef __CZMIL_INTERNALS_H__
#define __CZMIL_INTERNALS_H__

#ifdef  __cplusplus
extern "C" {
#endif


#undef CZMIL_DEBUG
#define CZMIL_DEBUG_OUTPUT stderr

#undef IDL_LOG


#ifdef IDL_LOG

  /*  Temporary (I hope) log file pointer.  */

  FILE *log_fp = NULL;
  char log_path[1024];

#endif


#ifndef DPRINT
  #define DPRINT      fprintf (stderr, "%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);fflush (stderr);
#endif

#ifndef NVFFL
  #define NVFFL         __FILE__,__FUNCTION__,__LINE__
#endif

#ifndef NINT
  #define NINT(a)       ((a)<0.0 ? (int32_t) ((a) - 0.5) : (int32_t) ((a) + 0.5))
#endif

#ifndef NINT64
  #define NINT64(a)     ((a)<0.0 ? (int64_t) ((a) - 0.5) : (int64_t) ((a) + 0.5))
#endif

#ifndef MAX
  #define MAX(x,y)      (((x) > (y)) ? (x) : (y))
#endif

#ifndef MIN
  #define MIN(x,y)      (((x) < (y)) ? (x) : (y))
#endif


#define       CZMIL_CWF_HEADER_SIZE               131072      /*!<  CWF header size.  */
#define       CZMIL_CPF_HEADER_SIZE               131072      /*!<  CPF header size.  */
#define       CZMIL_CSF_HEADER_SIZE               131072      /*!<  CSF header size.  */
#define       CZMIL_CIF_HEADER_SIZE               16384       /*!<  CIF header size.  */
#define       CZMIL_CAF_HEADER_SIZE               16384       /*!<  CAF header size.  */


  /*  Waveform compression types.  */ 

#define       CZMIL_BIT_PACKED                    0
#define       CZMIL_FIRST_DIFFERENCE              1
#define       CZMIL_SECOND_DIFFERENCE             2
#define       CZMIL_SHALLOW_CENTRAL_DIFFERENCE    3



  /*  These are generic size fields used by all applicable records.  */

#define TIME_BITS                 32              /*!<  Time is stored in the CWF, CPF, and CSF files as offsets in microseconds
                                                        from the [START TIMESTAMP] in the respective file header.  Using 32 bits
                                                        allows us to have a file that is 4,294 seconds (71.5 minutes) in length.
                                                        At 140 knots that would be a flightline that is approximately 310
                                                        kilometers long.  Since we're never supposed to be more than 30 kilometers
                                                        from our base station this shouldn't be a problem.
                                                        <b>*** WARNING WARNING WARNING -- If you increase this value you have to
                                                        change all the timestamp storing and retrieving code to use czmil_double_bit
                                                        pack and unpcak! -- WARNING WARNING WARNING ***</b>  */
#define ANGLE_SCALE               10000.0L        /*!<  Ten thousandths of a degree.  */
#define SCAN_ANGLE_BITS           22              /*!<  0 to 4,194,303 ( / ANGLE_SCALE = 0.0 to 419.4303 degrees (no negatives).  */


  /*  These are default constant values used for CWF waveform compression/decompression.  */

#define CWF_BUFFER_SIZE_BYTES     2               /*!<  Number of bytes used to store the size of the CWF bit-packed, compressed buffer.  
                                                        <b>*** WARNING WARNING WARNING -- DO NOT CHANGE! -- WARNING WARNING WARNING ***</b>  */
#define CWF_TYPE_BITS             3               /*!<  Number of bits used to store compressed CWF compression type.  See
                                                        czmil_compress_cwf_record for a description of how we're compressing
                                                        the different types.  */
#define CWF_TYPE_1_START_BITS     10              /*!<  Number of bits used to store first difference compressed CWF start
                                                        value.  */
#define CWF_TYPE_2_START_BITS     10              /*!<  Number of bits used to store second difference compressed CWF start
                                                        value.  */
#define CWF_TYPE_2_OFFSET_BITS    11              /*!<  Number of bits used to store second difference compressed CWF offset
                                                        value.  We're adding one bit for the worst case scenario of a large
                                                        difference in both directions.  */
#define CWF_TYPE_3_OFFSET_BITS    11              /*!<  Number of bits used to store shallow channel minus central shallow
                                                        channel difference compressed CWF offset value.  We're adding one
                                                        bit for the worst case scenario of a large difference in both
                                                        directions.  */
#define CWF_DELTA_BITS            4               /*!<  Number of bits used to store the number of bits we're using to store
                                                        CWF compressed delta values.  */
#define CWF_PACKET_NUMBER_BITS    7               /*!<  The packet number should never exceed 127 (we hope).  */
#define CWF_RANGE_BITS            10              /*!<  Number of bits used to store ranges.  */
#define CWF_RANGE_SCALE           16.0L           /*!<  Range scale value.  The range can only be a number between 0 and 63.  We're
                                                        going to have to use 10 bits to store it and we need at least .1 resolution
                                                        so we may as well use all of the 10 bit capacity.  */
#define CWF_SHOT_ID_BITS          25              /*!<  Number of bits used to store the shot ID (33,554,432 shots ~= 56 minutes
                                                        which is a good bit more than the time bits allows so this should never
                                                        max out).  */
#define CWF_VALIDITY_REASON_BITS  4               /*!<  Number of bits used for validity reason.  */


  /*  These are default bit/byte field sizes and scale factors used for CPF point cloud data compression/decompression.  */

#define CPF_BUFFER_SIZE_BYTES     2               /*!<  Number of bytes used to store the size of the CPF bit-packed, compressed buffer.  
                                                        <b>*** WARNING WARNING WARNING -- DO NOT CHANGE! -- WARNING WARNING WARNING ***</b>  */
#define CPF_LATITUDE_SCALE        72000000.0L     /*!<  Scale value to convert latitudes into units of 20,000ths of an arcsecond.  We're using
                                                        20,000ths here as opposed to 10,000ths for the CPF lat_diff scale because the
                                                        return latitudes will be an offset from this offset.  That could cause our
                                                        return latitude resolution to be approximately 6mm instead of 3mm.  */
#define CPF_LONGITUDE_SCALE       72000000.0L     /*!<  Scale value to convert longitudes into units of 20,000ths of an arcsecond.  We're using
                                                        20,000ths here as opposed to 10,000ths for the CPF lon_diff scale because the
                                                        return longitudes will be an offset from this offset.  That could cause our
                                                        return longitude resolution to be approximately 6mm instead of 3mm.  */
#define CPF_LAT_DIFF_SCALE        36000000.0L     /*!<  Scale value to convert latitude differences from the reference latitude into units of
                                                        10,000ths of an arcsecond.  */
#define CPF_LON_DIFF_SCALE        36000000.0L     /*!<  Scale value to convert longitude differences from the reference longitude into units of
                                                        10,000ths of an arcsecond.  */
#define CPF_LAT_BITS              28              /*!<  Number of bits used to store the offset of the reference latitude
                                                        from the base latitude in the header.  This value is store in 20,000ths
                                                        of an arcsecond and equates to a max offset distance of approximately
                                                        +/- 207 kilometers with a resolution of approximately 1.5mm.  The reason that we're
                                                        using 20,000ths of an arcsecond here is that we will store all other latitudes as
                                                        offsets from this latitude in 10,000ths of an arcsecond.  That will give us a
                                                        resolution of approximately 3mm for all other latitudes.  The relevant equations
                                                        being:
                                                            - 1/20,000 * 30.866666 meters per arcsecond = 0.0015433333 meters = 1.54mm
                                                            - 2^28 = 268,435,456 twenty thousandths of an arcsecond
                                                            - 268,435,456 / 20,000 = 13,421.7728 arcseconds
                                                            - 13,421.7728 arcseconds / 60 = 223.6962 arcminutes
                                                            - 223.6962 arcminutes * 1852 meters per arcminute = 414,285.3871 meters
                                                            - 414,285.3871 / 1000 meters per kilometer = 414.2854 kilometers
                                                            - 414.2854 kilometers / 2 = +/- 207.1427 kilometers  */
#define CPF_LON_BITS              28              /*!<  Number of bits used to store the offset of the reference longitude
                                                        from the base longitude in the header.  This value is store in 20,000ths
                                                        of an arcsecond times the cosine of the latitude degrees and equates to a max
                                                        offset distance of approximately +/- 207 kilometers with a resolution of
                                                        approximately 1.5mm.  The reason that we're using 20,000ths of an arcsecond here is
                                                        that we will store all other longitudes as offsets from this latitude in 10,000ths of
                                                        an arcsecond.  That will give us a resolution of approximately 3mm for all other
                                                        longitudes.    Remember that an arcminute of longitude at the equator is 1852 meters but
                                                        that the size of an arcminute of longitude varies approximately as the cosine of the
                                                        latitude (in this case the absolute value of the integer latitude degrees).  The
                                                        relevant equations being:
                                                            - 1/20,000 * 30.866666 meters per arcsecond (at the equator) = 0.0015433333 meters = 1.54mm
                                                            - 2^28 = 268,435,456 twenty thousandths of an arcsecond
                                                            - 268,435,456 / 20,000 = 13,421.7728 arcseconds
                                                            - 13,421.7728 arcseconds / 60 = 223.6962 arcminutes
                                                            - 223.6962 arcminutes * 1852 meters per arcminute = 414,285.3871 meters
                                                            - 414,285.3871 / 1000 meters per kilometer = 414.2854 kilometers
                                                            - 414.2854 kilometers / 2 = +/- 207.1427 kilometers  */
#define CPF_LAT_DIFF_BITS         18              /*!<  Number of bits used to store the offset of the return or bare earth latitude from the
                                                        reference latitude.  This value is store in 10,000ths of an arcsecond and equates to a max
                                                        offset distance of approximately +/-400 meters with a resolution of approximately 3mm.
                                                        Since maximum flying altitude is 1000 meters and we will never go more than about 65
                                                        meters below the water surface we can assume that the largest vertical distance between
                                                        the reference position and any of the return positions won't exceed 1100 meters.  Since the 
                                                        laser is fired at an angle of ~20 degrees we can compute the maximum distance of any return
                                                        point from the reference will be tangent of 20 times 1100, or ~400 meters.  */
#define CPF_LON_DIFF_BITS         18              /*!<  Number of bits used to store the offset of the return or bare earth longitude from the
                                                        reference longitude.  This value is store in 10,000ths of an arcsecond times
                                                        the cosine of the latitude degrees and equates to a max offset distance of
                                                        approximately +/-400 meters with a resolution of approximately 3mm.  Since maximum flying
                                                        altitude is 1000 meters and we will never go more than about 65 meters below the water
                                                        surface we can assume that the largest vertical distance between the reference position
                                                        and any of the return positions won't exceed 1100 meters.  Since the laser is fired at an
                                                        angle of ~20 degrees we can compute the maximum distance of any return point from the
                                                        reference will be tangent of 20 times 1100, or ~400 meters.  */
#define CPF_ELEV_BITS             22              /*!<  0 to 4,194,303 ( / CPF_ELEV_SCALE = 4194.303, / 2.0L = +/- 2097.151 meters ).  Null
                                                        elevations will be stored as the max value (i.e. 4,194,303).  */
#define CPF_ELEV_SCALE            1000.0L         /*!<  Elevation scale factor (millimeters).  */
#define CPF_UNCERT_BITS           14              /*!<  0 to 16,383 ( / CPF_UNCERT_SCALE = 0.0 to 16.383 meters).  */
#define CPF_UNCERT_SCALE          1000.0L         /*!<  Uncertainty scale factor (millimeters).  */
#define CPF_REFLECTANCE_BITS      14              /*!<  0 to 16,383 ( / CPF_REFLECTANCE_SCALE = 0.0 to 1.6383, reflectance range is 0.0 to 1.0)  */
#define CPF_REFLECTANCE_SCALE     10000.0L        /*!<  Ten thousandths of whatever units reflectance is in ;-)  */
#define CPF_RETURN_STATUS_BITS    8               /*!<  Number of bits used for per return status.  */
#define CPF_USER_DATA_BITS        8               /*!<  Number of bits used to store the shot user data (in v1 this was an unused shot status field,
                                                        then, in v2 it was an unused spare field).  */
#define CPF_CLASS_BITS            8               /*!<  Number of bits used for return classification.  */
#define CPF_OFF_NADIR_ANGLE_BITS  21              /*!<  0 to 2,097,151
                                                        2,097,151 / ANGLE_SCALE = 209.7151
                                                        209.7151 / 2 = +/-104.8575 degrees.  */
#define CPF_INTEREST_POINT_SCALE  100.0L          /*!<  Scale value for interest points.  */
#define CPF_KD_SCALE              10000.0L        /*!<  Scale value for Kd (diffused attenuation coefficient or light clarity).  */
#define CPF_KD_BITS               14              /*!<  Number of bits used to store Kd (0.0000 to 1.0000 times CPF_KD_SCALE).  */
#define CPF_LASER_ENERGY_SCALE    10000.0L        /*!<  Scale value for laser energy.  */
#define CPF_LASER_ENERGY_BITS     16              /*!<  Number of bits used to store laser energy (0.0000 to 6.5535 times CPF_LASER_ENERGY_SCALE).  */
#define CPF_PROBABILITY_BITS      14              /*!<  0 to 16,383 ( / CPF_PROBABILITY_SCALE = 0.0 to 1.6383, probability range is 0.0 to 1.0)  */
#define CPF_PROBABILITY_SCALE     10000.0L        /*!<  Ten thousandths of whatever units probability is in ;-)  */
#define CPF_RETURN_FILTER_REASON_BITS 6           /*!<  Number of bits used for per return filter reason.  */
#define CPF_OPTECH_CLASSIFICATION_BITS 6          /*!<  Number of bits used for Optech waveform processing mode.  */
#define CPF_IP_RANK_BITS          1               /*!<  Number of bits used for the ip_rank field.  Please see CZMIL_Return_Data structure in czmil.h
                                                        for a more complete explanation.  */
#define CPF_D_INDEX_BITS          10              /*!<  Number of bits used to store the d_index value.  0 to 1,023.  */
#define CPF_D_INDEX_CUBE_BITS     10              /*!<  Number of bits used to store prediction of the normalized signal amplitude from a cube on
                                                        the seafloor in the deep channel.  0 to 1,023.  */


  /*  These are default constant values used for CSF file packing/unpacking.  */

#define CSF_LATITUDE_SCALE        36000000.0L     /*!<  Scale value to convert latitudes into units of 10,000ths of an arcsecond.  */
#define CSF_LONGITUDE_SCALE       36000000.0L     /*!<  Scale value to convert longitudes into units of 10,000ths of an arcsecond.  */
#define CSF_LAT_BITS              27              /*!<  Number of bits used to store the offset of the reference latitude
                                                        from the base latitude in the header.  This value is store in 10,000ths
                                                        of an arcsecond and equates to a max offset distance of approximately
                                                        +/- 207 kilometers with a resolution of approximately 3mm.    The relevant
                                                        equations being:
                                                            - 1/10,000 * 30.866666 meters per arcsecond = 0.0030866666 meters = 3.08mm
                                                            - 2^27 = 134,217,728 ten thousandths of an arcsecond
                                                            - 134,217,728 / 10,000 = 13,421.7728 arcseconds
                                                            - 13,421.7729 arcseconds / 60 = 223.6962 arcminutes
                                                            - 223.6962 arcminutes * 1852 meters per arcminute = 414,285.3871 meters
                                                            - 414,285.3871 / 1000 meters per kilometer = 414.2854 kilometers
                                                            - 414.2854 kilometers / 2 = +/- 207.1427 kilometers  */
#define CSF_LON_BITS              27              /*!<  Number of bits used to store the offset of the reference longitude
                                                        from the base longitude in the header.  This value is store in 10,000ths
                                                        of an arcsecond times the cosine of the latitude degrees and equates to a max
                                                        offset distance of approximately +/- 207 kilometers with a resolution of
                                                        approximately 3mm.  Remember that an arcminute of longitude at the equator is 
                                                        1852 meters but that the size of an arcminute of longitude varies approximately
                                                        as the cosine of the latitude (in this case the absolute value of the integer
                                                        latitude degrees).  The relevant equations being:
                                                            - 1/10,000 * 30.866666 meters per arcsecond = 0.0030866666 meters = 3.08mm
                                                            - 2^27 = 134,217,728 ten thousandths of an arcsecond
                                                            - 134,217,728 / 10,000 = 13,421.7728 arcseconds
                                                            - 13,421.7729 arcseconds / 60 = 223.6962 arcminutes
                                                            - 223.6962 arcminutes * 1852 meters per arcminute = 414,285.3871 meters
                                                            - 414,285.3871 / 1000 meters per kilometer = 414.2854 kilometers
                                                            - 414.2854 kilometers / 2 = +/- 207.1427 kilometers  */
#define CSF_ALTITUDE_BITS         22              /*!<  0 to 4,194,303 ( / CSF_ALTITUDE_SCALE = 4194.303, / 2.0L = +/- 2097.151
                                                        meters ).  */
#define CSF_ALTITUDE_SCALE        1000.0L         /*!<  Altitude scale factor (millimeters).  */  
#define CSF_ROLL_PITCH_BITS       20              /*!<  0 to 1,048,575 ( / ANGLE_SCALE = 104.8575,
                                                        / 2.0L = +/- 52.42875 degrees.  */
#define CSF_HEADING_BITS          22              /*!<  0 to 4,194,303 ( / ANGLE_SCALE = 0.0 to 419.4303 degrees (no
                                                        negatives).  */
#define CSF_RANGE_BITS            21              /*!<  Number of bits used to store LiDAR ranges per channel (not to be confused with MCWP ranges).
                                                        This is similar to a slant range in sonar.
                                                        0 to 2,097,151 ( / CSF_RANGE_SCALE = 2097.151 meters ).  */
#define CSF_RANGE_SCALE           1000.0L         /*!<  LiDAR range (and range in water) scale factor (millimeters).  */
#define CSF_INTENSITY_BITS        14              /*!<  0 to 16,383 ( / CSF_INTENSITY_SCALE = 0.0 to 1.6383, intensity range is 0.0 to 1.0)  */
#define CSF_INTENSITY_SCALE       10000.0L        /*!<  Ten thousandths of whatever units intensity is in ;-)  */


  /*!

      - CZMIL CIF file header structure.  This definition isn't in czmil.h because it doesn't need to be seen outside of the API.
        Header key definitions are as follows:

  */

  typedef struct
  {
    char              version[128];               /*!<  Library version information  */
    char              file_type[128];             /*!<  File type  */
    uint64_t          creation_timestamp;         /*!<  File creation microseconds from 01-01-1970  */
    uint32_t          number_of_records;          /*!<  Number of records in file  */
    uint32_t          header_size;                /*!<  Size of the ASCII header in bytes  */
    uint16_t          cpf_address_bits;           /*!<  Number of bits used to store the CPF record byte address.  */
    uint16_t          cwf_address_bits;           /*!<  Number of bits used to store the CWF record byte address.  */
    uint16_t          cpf_buffer_size_bits;       /*!<  Number of bits used to store the size of the CPF buffer.  This is 
                                                        triply redundant but it will save one fread call (we won't have to
                                                        read the buffer size from the CPF file before we read the rest of the
                                                        buffer) per shot read.  The added bytes in the CIF file are pretty
                                                        much under the radar at 10,000 shots per second.  */
    uint16_t          cwf_buffer_size_bits;       /*!<  Number of bits used to store the size of the CWF buffer.  Same logic
                                                        as above.  */


    /*  The following information is computed from the information read from the CIF file ASCII header.  */

    uint16_t          record_size_bytes;          /*!<  Size in bytes of each CIF record.  */
  } CZMIL_CIF_Header;


  /*  The CIF data structure.  */

  typedef struct
  {
    int64_t           cpf_address;                /*!< Byte address of the associated CPF record in the CPF file  */
    int64_t           cwf_address;                /*!< Byte address of the associated CWF record in the CWF file  */
    uint16_t          cpf_buffer_size;            /*!< Size of CPF buffer at cpf_address  */
    uint16_t          cwf_buffer_size;            /*!< Size of CWF buffer at cwf_address  */
  } CZMIL_CIF_Data;


  /*!  This is the structure we use to keep track of important formatting data for an open CZMIL CIF file.  */

  typedef struct
  {
    char              path[1024];                 /*!<  Fully qualified CIF file name.  */
    FILE              *fp;                        /*!<  CIF file pointer.  */
    CZMIL_CIF_Header  header;                     /*!<  CIF file header.  */
    uint8_t           created;                    /*!<  Set if we created the CIF file.  */
    int64_t           pos;                        /*!<  Position of the CIF file pointer after last I/O operation.  */
    int64_t           prev_pos;                   /*!<  Position of the CIF file pointer prior to the last I/O operation.  */
    CZMIL_CIF_Data    record;                     /*!<  Last CIF record read.  This may save us from re-reading the CIF
                                                        record for the CWF waveforms if we just read the CPF point.  */
    uint16_t          major_version;              /*!<  Major version number (broken out of the version string).  */
    uint16_t          minor_version;              /*!<  Minor version number (broken out of the version string).  */


    /*  The following is related to the CIF I/O buffer.  These are used for an internal buffer on creation or a setvbuf buffer
        when opening as CZMIL_READONLY_SEQUENTIAL.  */

    uint32_t          io_buffer_size;             /*!<  CIF I/O buffer size used.  */
    uint32_t          io_buffer_address;          /*!<  Location within the I/O buffer at which we will place our next compressed
                                                        block of index data.  This is only used during creation of the CIF file.  */
    uint8_t           *io_buffer;                 /*!<  The actual I/O buffer that will be allocated on creation/open and freed on close.  */
  } INTERNAL_CZMIL_CIF_STRUCT;


  /*!  This is the structure we use to keep track of important formatting data for an open CZMIL CWF file.  */

  typedef struct
  {
    char              path[1024];                 /*!<  Fully qualified CWF file name.  */
    FILE              *fp;                        /*!<  CWF file pointer.  */
    CZMIL_CWF_Header  header;                     /*!<  CWF file header.  */
    uint8_t           at_end;                     /*!<  Set if the CWF file position is at the end of the file.  */
    uint8_t           created;                    /*!<  Set if we created the CWF file.  */
    uint8_t           modified;                   /*!<  Set if the CWF file header has been modified.  */
    uint8_t           write;                      /*!<  Set if the last action to the CWF file was a write.  */
    int32_t           mode;                       /*!<  File open mode (CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL).  */
    int64_t           pos;                        /*!<  Position of the CWF file pointer after last I/O operation.  */
    INTERNAL_CZMIL_CIF_STRUCT cif;                /*!<  This structure is used when creating the CIF file during creation of the CWF and CPF files.  */
    int32_t           cif_hnd;                    /*!<  Handle for the associated CIF file.  */
    char              app_tags[CZMIL_CWF_HEADER_SIZE];
                                                  /*!<  Place to hold application defined header fields when we read the header so that we can
                                                        write it back out if we modify any header fields.  */
    int32_t           app_tags_pos;               /*!<  Current position within the application defined header fields buffer.  */


    /*  The following information is read from the CWF file ASCII header but is not placed in the header structure since we have no need
        of this information outside of the API.  */

    uint16_t          buffer_size_bytes;          /*!<  Number of bytes used to store the size of the compressed CWF buffer.  */
    uint16_t          type_bits;                  /*!<  Number of bits used to store compressed CWF compression type.  See
                                                        czmil_compress_cwf_packet for a description of how we're compressing
                                                        the different types.  */
    uint16_t          type_1_start_bits;          /*!<  Number of bits used to store first difference compressed CWF start value.  */
    uint16_t          type_2_start_bits;          /*!<  Number of bits used to store second difference compressed CWF start value.  */
    uint16_t          delta_bits;                 /*!<  Number of bits used to store the number of bits we're using to store
                                                        CWF compressed delta values.  */
    uint16_t          czmil_max_packets;          /*!<  Maximum number of 64 sample packets allowed.  This is also the number
                                                        of bits used to store the _ndx bits.  */
    uint16_t          time_bits;                  /*!<  Number of bits used to store a bit packed timestamp.  */
    float             angle_scale;                /*!<  Scale factor for scan angle.  */
    uint16_t          scan_angle_bits;            /*!<  Number of bits used to store scan angle.  */
    uint16_t          packet_number_bits;         /*!<  Number of bits used to store the packet number.  */
    uint16_t          range_bits;                 /*!<  Number of bits used to store the ranges.  */
    float             range_scale;                /*!<  Scale value for ranges.  */
    uint16_t          shot_id_bits;               /*!<  Number of bits used to store the shot ID.  */
    uint16_t          validity_reason_bits;       /*!<  Number of bits used to store validity reason.  */


    /*  The following information is computed from the information read from the CWF file ASCII header but is not placed in the
        header structure since we have no need of this information outside of the API.  */

    uint16_t          type_0_bytes;               /*!<  Number of bytes used to store bit packed 10 bit values.  */
    uint16_t          type_1_offset_bits;         /*!<  Number of bits used to store first difference compressed CWF offset value.  */
    uint16_t          type_1_offset;              /*!<  This is 2 raised to the type_1_start_bits + 1 power.  This is the
                                                        offset value added to the offset value so that the first offset will
                                                        be a positive value.  This way we don't have to sign extend the 
                                                        offset.  This is also used for the type 3 compression offset.  */
    uint16_t          type_2_offset_bits;         /*!<  Number of bits used to store second difference compressed CWF offset value.  */
    uint16_t          type_2_offset;              /*!<  This is 2 raised to the type_2_start_bits + 1 power.  This is the
                                                        offset value added to the offset value so that the first offset will
                                                        be a positive value.  This way we don't have to sign extend the 
                                                        offset.  */
    uint16_t          type_3_offset_bits;         /*!<  Number of bits used to store shallow channel difference compressed CWF offset value.  */
    uint16_t          type_1_header_bits;         /*!<  Bits used for the CWF type 1 compressed CWF packet header.  */
    uint16_t          type_2_header_bits;         /*!<  Bits used for the CWF type 2 compressed CWF packet header.  */
    uint16_t          type_3_header_bits;         /*!<  Bits used for the CWF type 3 compressed CWF packet header.  */
    uint32_t          time_max;                   /*!<  Maximum time offset (usecs) from start timestamp.  Computed from time_bits.  */
    uint16_t          num_packets_bits;           /*!<  Number of bits used to store the number of packets per channel.  Computed from
                                                        czmil_max_packets.  */
    uint16_t          packet_number_max;          /*!<  Maximum allowable packet number.  Computed from packet_number_bits.  */
    uint32_t          range_max;                  /*!<  Maximum scaled range value.  Computed from range_bits.  */
    uint32_t          shot_id_max;                /*!<  Maximum shot ID number.  Computed from shot_id_bits.  */
    uint32_t          validity_reason_max;        /*!<  Maximum validity reason value.  Computed from validity_reason_bits.  */
    uint16_t          major_version;              /*!<  Major version number (broken out of the version string).  */
    uint16_t          minor_version;              /*!<  Minor version number (broken out of the version string).  */


    /*  The following is related to the CWF I/O buffer.  These are used for an internal buffer on creation or a setvbuf buffer
        when opening as CZMIL_READONLY_SEQUENTIAL.  */

    uint32_t          io_buffer_size;             /*!<  CWF I/O buffer size used.  */
    uint32_t          io_buffer_address;          /*!<  Location within the I/O buffer at which we will place our next compressed
                                                        block of waveform data.  */
    uint8_t           *io_buffer;                 /*!<  The actual I/O buffer that will be allocated on creation and freed on close.  */
    int64_t           create_file_pos;            /*!<  This is the apparent file position that we will use to create the CIF file as
                                                        we create the CWF file.  */
  } INTERNAL_CZMIL_CWF_STRUCT;


  /*!  This is the structure we use to keep track of important formatting data for an open CZMIL CPF file.  */

  typedef struct
  {
    char              path[1024];                 /*!<  Fully qualified CPF file name.  */
    FILE              *fp;                        /*!<  CPF file pointer.  */
    CZMIL_CPF_Header  header;                     /*!<  CPF file header.  */
    uint8_t           at_end;                     /*!<  Set if the CPF file position is at the end of the file.  */
    uint8_t           modified;                   /*!<  Set if the CPF file has been modified.  */
    uint8_t           created;                    /*!<  Set if we created the CPF file.  */
    uint8_t           write;                      /*!<  Set if the last action to the CPF file was a write.  */
    int32_t           mode;                       /*!<  File open mode (CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL).  */
    int64_t           pos;                        /*!<  Position of the CPF file pointer after last I/O operation.  */
    int32_t           cif_hnd;                    /*!<  Handle for the associated CIF file.  */
    char              app_tags[CZMIL_CPF_HEADER_SIZE];
                                                  /*!<  Place to hold application defined header fields when we read the header so that we can
                                                        write it back out if we modify any header fields.  */
    int32_t           app_tags_pos;               /*!<  Current position within the application defined header fields buffer.  */


    /*  The following information is read from the CPF file ASCII header but is not placed in the header structure since we have no
        need of this information outside of the API.  */

    uint16_t          buffer_size_bytes;          /*!<  Number of bytes used to store the size of the CPF bit-packed, compressed buffer.  */
    uint16_t          time_bits;                  /*!<  Number of bits used to store a bit packed timestamp.  */
    float             angle_scale;                /*!<  Scale Factor for off_nadir_angle.  */
    uint16_t          off_nadir_angle_bits;       /*!<  Number of bits used to store the off nadir angle.  */
    uint16_t          heading_bits;               /*!<  Number of bits used to store heading.  */
    uint16_t          lat_bits;                   /*!<  Number of bits used to store reference latitude offset from header base latitude.  */
    uint16_t          lon_bits;                   /*!<  Number of bits used to store reference longitude offset from header base longitude.  */
    uint16_t          lat_diff_bits;              /*!<  Number of bits used to store latitude offset from reference latitude.  */
    uint16_t          lon_diff_bits;              /*!<  Number of bits used to store longitude offset from reference longitude.  */
    double            lat_scale;                  /*!<  Latitude scaling factor (converts to 20,000ths of an arcsecond).  */
    double            lon_scale;                  /*!<  Longitude scaling factor (converts to 20,000ths of an arcsecond).  */
    double            lat_diff_scale;             /*!<  Latitude difference scaling factor (converts to 10,000ths of an arcsecond).  */
    double            lon_diff_scale;             /*!<  Longitude difference scaling factor (converts to 10,000ths of an arcsecond).  */
    uint16_t          elev_bits;                  /*!<  Number of bits used to store elevations.  */
    float             elev_scale;                 /*!<  Elevation scale factor.  */
    uint16_t          uncert_bits;                /*!<  Number of bits used to store horizontal and vertical uncertainties.  */
    float             uncert_scale;               /*!<  Uncertainty scale factor.  */
    uint16_t          reflectance_bits;           /*!<  Number of bits used to store reflectance.  */
    float             reflectance_scale;          /*!<  Reflectance scale factor.  */
    uint16_t          return_status_bits;         /*!<  Number of bits used to store return status flags.  */
    uint16_t          user_data_bits;             /*!<  Number of bits used to store the user_data field (this was the spare field in v2 and the
                                                        shot status field in v1 but it was never used n either case).  */
    uint16_t          class_bits;                 /*!<  Number of bits used to store return classification.  */
    uint16_t          return_max;                 /*!<  Maximum number of returns per channel.  */
    uint16_t          czmil_max_packets;          /*!<  Maximum number of 64 sample packets allowed in the CWF waveform file.  This is used to compute 
                                                        the number of bits used for interest points.  */
    float             interest_point_scale;       /*!<  Interest point scale factor.  */
    uint16_t          kd_bits;                    /*!<  Number of bits used to store Kd value.  */
    float             kd_scale;                   /*!<  Kd scale factor.  */
    uint16_t          laser_energy_bits;          /*!<  Number of bits used to store laser energy value.  */
    float             laser_energy_scale;         /*!<  Laser energy scale factor.  */
    uint16_t          probability_bits;           /*!<  Number of bits used to store probability.  */
    float             probability_scale;          /*!<  Probability scale factor.  */
    uint16_t          return_filter_reason_bits;  /*!<  Number of bits used to store return filter reason.  */
    uint16_t          optech_classification_bits; /*!<  Number of bits used to store Optech waveform processing mode.  */
    uint16_t          d_index_bits;               /*!<  Number of bits used to store d_index.  */
    uint16_t          d_index_cube_bits;          /*!<  Number of bits used to store d_index_cube.  */


    /*  The following information is computed from the above information read from the CPF file ASCII header but is not placed in
        the header structure since we have no need of this information outside of the API.  */

    uint32_t          lat_max;                    /*!<  Maximum allowed latitude difference of the reference latitude from the base latitude after
                                                        scaling and adding the offset.  */
    int32_t           lat_offset;                 /*!<  Latitude difference offset (half of lat_max).  */
    uint32_t          lon_max;                    /*!<  Maximum allowed longitude difference of the reference longitude from the base longitude after
                                                        scaling and adding the offset.  */
    int32_t           lon_offset;                 /*!<  Longitude difference offset (half of lon_max).  */
    uint32_t          lat_diff_max;               /*!<  Maximum allowed latitude difference from the reference latitude after scaling and adding
                                                        the offset.  */
    int32_t           lat_diff_offset;            /*!<  Latitude difference offset (half of lat_diff_max).  */
    uint32_t          lon_diff_max;               /*!<  Maximum allowed longitude difference from the reference longitude after scaling and adding
                                                        the offset.  */
    int32_t           lon_diff_offset;            /*!<  Longitude difference offset (half of lon_diff_max).  */
    uint32_t          elev_max;                   /*!<  Maximum allowed elevation difference value after adding the offset.  */
    int32_t           elev_offset;                /*!<  Elevation difference offset (half of elev_max).  */
    uint32_t          uncert_max;                 /*!<  Maximum uncertainty value that will be stored in a CPF record.  */
    uint32_t          reflectance_max;            /*!<  Maximum reflectance value.  */
    uint32_t          class_max;                  /*!<  Maximum return classification value after adding the offset.  */
    uint32_t          off_nadir_angle_max;        /*!<  Maximum off nadir angle value after adding the offset.  */
    int32_t           off_nadir_angle_offset;     /*!<  Off nadir angle offset (half of off_nadir_angle_max).  */
    uint32_t          time_max;                   /*!<  Maximum time offset (usecs) from start timestamp.  Computed from time_bits.  */
    uint32_t          return_status_max;          /*!<  Maximum status value.  Computed from return_status_bits.  */
    uint16_t          return_bits;                /*!<  Number of bits used to store the number of returns per channel.  */
    uint16_t          interest_point_bits;        /*!<  Number of bits used to store the interest point
                                                        (based on CZMIL_MAX_PACKETS * 64 samples/packet).  */
    uint32_t          interest_point_max;         /*!<  Maximum size of scaled interest point value (2^interest_point_bits).  */
    uint16_t          ip_rank_bits;               /*!<  Number of bits used to store the ip_rank field of the CZMIL_Return_Data structure (see
                                                        czmil.h)  */
    uint16_t          major_version;              /*!<  Major version number (broken out of the version string).  */
    uint16_t          minor_version;              /*!<  Minor version number (broken out of the version string).  */
    uint32_t          laser_energy_max;           /*!<  Maximum laser energy value that will be stored in a CPF record (2^laser_energy_bits *
                                                        laser_energy_scale).  */
    uint32_t          probability_max;            /*!<  Maximum probability value.  */
    uint32_t          return_filter_reason_max;   /*!<  Maximum return filter reason value.  Computed from return_filter_reason_bits.  */
    uint32_t          optech_classification_max;  /*!<  Maximum Optech waveform processing mode value.  Computed from optech_classification_bits.  */
    uint32_t          d_index_max;                /*!<  Maximum d_index value that will be stored in a CPF record.  */
    uint16_t          d_index_cube_max;           /*!<  Maximum d_index_cube value.  */


    /*  The following is related to the CPF I/O buffer.  These are used for an internal buffer on creation or a setvbuf buffer
        when opening as CZMIL_READONLY_SEQUENTIAL.  */

    uint8_t           buffer[sizeof (CZMIL_CPF_Data)];
                                                  /*!<  This is the single record buffer.  This is used when updating a CPF record.  In most cases
                                                        an application will read a CPF record, modify the modifiable parts of the record (status,
                                                        classification, datum offset, etc.) then write the record.  Since storing floating point
                                                        numbers like the latitude requires a little lost precision every time we do it we don't
                                                        want to continuously read and write these values (they should never change anyway).  We
                                                        also don't want to read the buffer over again if we just read it.  So, we share the buffer
                                                        space between the czmil_read_cpf_record and czmil_write_cpf_record functions and keep
                                                        track of the last record number read so we don't, in many cases, have to read the buffer.
                                                        The actual data will never be sizeof (CZMIL_CPF_Data) in size since we are unpacking it
                                                        but this way we don't have to worry about blowing this up.  Also, we don't allocate the
                                                        memory because memory allocation invokes a system wide mutex.  */
    int32_t           last_record_read;           /*!<  Last record number read in czmil_read_cpf_record.  Used in czmil_write_cpf_record.  */
    uint32_t          io_buffer_size;             /*!<  CPF I/O buffer size.  */
    uint32_t          io_buffer_address;          /*!<  Location within the I/O buffer at which we will place our next compressed
                                                        block of point data.  */
    uint8_t           *io_buffer;                 /*!<  The actual I/O buffer that will be allocated on creation/open and freed on close.  */
    int64_t           create_file_pos;            /*!<  This is the apparent file position that we will use to update the CIF file as
                                                        we create the CPF file.  */
  } INTERNAL_CZMIL_CPF_STRUCT;


  /*!  This is the structure we use to keep track of important formatting data for an open CZMIL CSF file.  */

  typedef struct
  {
    char              path[1024];                 /*!<  Fully qualified CSF file name.  */
    FILE              *fp;                        /*!<  CSF file pointer.  */
    CZMIL_CSF_Header  header;                     /*!<  CSF file header.  */
    uint8_t           at_end;                     /*!<  Set if the CSF file position is at the end of the file.  */
    uint8_t           created;                    /*!<  Set if we created the CSF file.  */
    uint8_t           modified;                   /*!<  Set if the CSF file header has been modified.  */
    uint8_t           write;                      /*!<  Set if the last action to the CSF file was a write.  */
    int32_t           mode;                       /*!<  File open mode (CZMIL_UPDATE, CZMIL_READONLY, or CZMIL_READONLY_SEQUENTIAL).  */
    int64_t           pos;                        /*!<  Position of the CSF file pointer after last I/O operation.  */
    char              app_tags[CZMIL_CSF_HEADER_SIZE];  /*!<  Place to hold application defined header fields when we read the
                                                              header so that we can write it back out if we modify any header
                                                              fields.  */
    int32_t           app_tags_pos;               /*!<  Current position within the application defined header fields buffer.  */


    /*  The following information is read from the CSF file ASCII header but is not placed in the header structure since we have no
        need of this information outside of the API.  */

    uint16_t          time_bits;                  /*!<  Number of bits used to store a bit packed timestamp.  */
    uint16_t          lat_bits;                   /*!<  Number of bits used to store reference latitude offset from header base latitude.  */
    uint16_t          lon_bits;                   /*!<  Number of bits used to store reference longitude offset from header base longitude.  */
    double            lat_scale;                  /*!<  Scale factor for latitude.  */
    double            lon_scale;                  /*!<  Scale factor for longitude.  */
    float             alt_scale;                  /*!<  Scale factor for altitude.  */
    uint16_t          alt_bits;                   /*!<  Number of bits used to store altitude.  */
    float             angle_scale;                /*!<  Scale factor for all angles (scan, roll, pitch, heading).  */
    uint16_t          scan_angle_bits;            /*!<  Number of bits used to store the scan angle.  */
    uint16_t          heading_bits;               /*!<  Number of bits used to store heading.  */
    uint16_t          roll_pitch_bits;            /*!<  Number of bits used to store roll and pitch.  */
    uint16_t          range_bits;                 /*!<  Number of bits used to store the ranges.  */
    float             range_scale;                /*!<  Scale value for ranges.  */
    uint16_t          intensity_bits;             /*!<  Number of bits used to store intensity.  */
    float             intensity_scale;            /*!<  Intensity scale factor.  */


    /*  The following information is computed from the information read from the CSF file ASCII header but is not placed in the
        header structure since we have no need of this information outside of the API.  */

    uint16_t          buffer_size;                /*!<  Number of bytes used to store bit-packed CSF record.  */
    uint32_t          lat_max;                    /*!<  Maximum allowed latitude difference of the reference latitude from the base latitude after
                                                        scaling and adding the offset.  */
    int32_t           lat_offset;                 /*!<  Latitude difference offset (half of lat_max).  */
    uint32_t          lon_max;                    /*!<  Maximum allowed longitude difference of the reference longitude from the base longitude after
                                                       scaling and adding the offset.  */
    int32_t           lon_offset;                 /*!<  Longitude difference offset (half of lon_max).  */
    uint32_t          alt_max;                    /*!<  Maximum scaled, offset altitude value.  */
    int32_t           alt_offset;                 /*!<  Offset value for altitude.  Computed from alt_bits.  */
    uint32_t          roll_pitch_max;             /*!<  Maximum roll and pitch value after adding the offset.  */
    int32_t           roll_pitch_offset;          /*!<  Roll and pitch offset (half of roll_pitch_max).  */
    uint32_t          time_max;                   /*!<  Maximum time offset (usecs) from start timestamp.  Computed from time_bits.  */
    uint32_t          range_max;                  /*!<  Maximum scaled range value.  Computed from range_bits.  */
    uint32_t          intensity_max;              /*!<  Maximum intensity value.  */
    uint16_t          major_version;              /*!<  Major version number (broken out of the version string).  */
    uint16_t          minor_version;              /*!<  Minor version number (broken out of the version string).  */


    /*  The following is related to the CSF I/O buffer.  These are used for an internal buffer on creation or a setvbuf buffer
        when opening as CZMIL_READONLY_SEQUENTIAL.  */

    uint8_t           buffer[sizeof (CZMIL_CSF_Data)];
                                                  /*!<  This is the single record buffer.  This is used when updating a CSF record.
                                                        The actual data will never be sizeof (CZMIL_CSF_Data) in size since we are packing it
                                                        but this way we don't have to worry about blowing this up.  Also, we don't allocate the
                                                        memory because memory allocation invokes a system wide mutex.  */
    uint32_t          io_buffer_size;             /*!<  CSF I/O buffer size.  */
    uint32_t          io_buffer_address;          /*!<  Location within the I/O buffer at which we will place our next compressed
                                                        block of SBET data.  */
    uint8_t           *io_buffer;                 /*!<  The actual I/O buffer that will be allocated on creation/open and freed on close.  */
  } INTERNAL_CZMIL_CSF_STRUCT;




  /*!  This is the structure we use to keep track of important formatting data for an open CZMIL CAF file.  */

  typedef struct
  {
    char              path[1024];                 /*!<  Fully qualified CAF file name.  */
    FILE              *fp;                        /*!<  CAF file pointer.  */
    CZMIL_CAF_Header  header;                     /*!<  CAF file header.  */
    uint8_t           at_end;                     /*!<  Set if the CAF file position is at the end of the file.  */
    uint8_t           created;                    /*!<  Set if we created the CAF file.  */
    uint8_t           modified;                   /*!<  Set if the CAF file header has been modified.  */
    uint8_t           write;                      /*!<  Set if the last action to the CAF file was a write.  */


    /*  The following information is read from the CAF file ASCII header but is not placed in the header structure since we have no
        need of this information outside of the API.  */

    uint16_t          buffer_size;                /*!<  Number of bytes used to store bit-packed CAF record.  */
    uint16_t          shot_id_bits;               /*!<  Number of bits used to store the CPF record number (default is CWF_SHOT_ID_BITS)  */
    uint16_t          channel_number_bits;        /*!<  Number of bits used to store the CPF channel number (always 4).  */
    uint16_t          optech_classification_bits; /*!<  Number of bits used to store the Optech waveform processing mode (default is
                                                        CPF_OPTECH_CLASSIFICATION_BITS).  */
    float             interest_point_scale;       /*!<  Interest point scale factor (default is CPF_INTEREST_POINT_SCALE).  */
    uint16_t          return_max;                 /*!<  Maximum number of returns per channel.  */
    uint16_t          czmil_max_packets;          /*!<  Maximum number of 64 sample waveform packets allowed.  */


    /*  The following information is computed from the information read from the CAF file ASCII header but is not placed in the
        header structure since we have no need of this information outside of the API.  */

    uint16_t          return_bits;                /*!<  Number of bits used to store the return number and number of returns per channel.  */
    uint16_t          interest_point_bits;        /*!<  Number of bits used to store the interest point (based on CZMIL_MAX_PACKETS *
                                                        64 samples/packet).  */
    uint32_t          interest_point_max;         /*!<  Maximum size of scaled interest point value (2^interest_point_bits).  */
    uint32_t          shot_id_max;                /*!<  Maximum record number/shot ID (2^shot_bits).  */
    uint32_t          channel_number_max;         /*!<  Maximum channel number (always 8).  */
    uint32_t          optech_classification_max;  /*!<  Maximum Optech waveform processing mode value.  Computed from optech_classification_bits.  */
    uint16_t          major_version;              /*!<  Major version number (broken out of the version string).  */
    uint16_t          minor_version;              /*!<  Minor version number (broken out of the version string).  */


    /*  The following is related to the CAF I/O buffer.  These are used for an internal buffer on creation or a setvbuf buffer
        when opening as CZMIL_READONLY_SEQUENTIAL.  */

    uint32_t          io_buffer_size;             /*!<  CAF I/O buffer size.  */
    uint32_t          io_buffer_address;          /*!<  Location within the I/O buffer at which we will place our next compressed
                                                        block of data.  */
    uint8_t           *io_buffer;                 /*!<  The actual I/O buffer that will be allocated on creation/open and freed on close.  */
  } INTERNAL_CZMIL_CAF_STRUCT;



  /*  These are default constant values used for CIF index file packing/unpacking.  It would probably be best to leave these as they 
      are since this works out to 16 bytes per record.  Every system I know of uses an I/O buffer that is a multiple of 16 bytes.  */

#define CIF_CWF_ADDRESS_BITS      48              /*!<  This MUST be a multiple of 8.  48 bits will allow us to address a file that is
                                                        up to about 280TB in size.  */
#define CIF_CPF_ADDRESS_BITS      48              /*!<  This MUST be a multiple of 8.  48 bits will allow us to address a file that is
                                                        up to about 280TB in size.  */
#define CIF_CWF_BUFFER_SIZE_BITS  CWF_BUFFER_SIZE_BYTES * 8
#define CIF_CPF_BUFFER_SIZE_BITS  CPF_BUFFER_SIZE_BYTES * 8
#define CZMIL_CIF_IO_BUFFER_SIZE  (CWF_BUFFER_SIZE_BYTES + CPF_BUFFER_SIZE_BYTES + CIF_CWF_ADDRESS_BITS / 8 + CIF_CPF_ADDRESS_BITS / 8) * 262144
                                                  /*!<  Default CIF I/O buffer size.  This is 512 * 512 * 16 or 26.2 seconds of data at 10000
                                                        shots/sec  */


  /*  These are default constant values used for CAF audit file packing/unpacking.  */

#define CAF_CHANNEL_NUMBER_BITS   4               /*!<  This will NEVER change.  CZMIL only has 9 channels.  */


  /*  CZMIL error handling variables.  */

  typedef struct 
  {
    int32_t           czmil;                      /*!<  Last CZMIL error condition encountered.  */
    char              info[2048];                 /*!<  Text information to be printed out.  */
  } CZMIL_ERROR_STRUCT;


#ifdef  __cplusplus
}
#endif


#endif
