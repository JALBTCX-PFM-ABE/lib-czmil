
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


/*  Trying to make sure we use the correct inline statement in different environments.  Not sure about the __APPLE__ stuff so
    it's really just a placeholder that is set the same as for the GNU C compiler.  */

#if (defined _WIN32) && (defined _MSC_VER)
#define CZMIL_INLINE static __inline
#elif defined(__APPLE__)
#define CZMIL_INLINE static inline
#else
#define CZMIL_INLINE static inline
#endif


/*!  Array of pre-computed cosine values for each integer degree of latitude.  These are used to scale the 
     longitude differences (in 10,000ths of an arcsecond) so that we maintain our approximate 3mm of resolution
     as we move away from the equator and we don't blow out our bit-packed longitude difference.  The values 
     were computed using the following code:
     <br>
     for (i = 89 ; i > 0 ; i--) fprintf (stderr, "%.9f, ", cos ((double) i / 57.2957795147195));
     for (i = 0 ; i < 90 ; i++) fprintf (stderr, "%.9f, ", cos ((double) i / 57.2957795147195));
     <br>
*/

static const double cos_array[179] =
  {0.017452406, 0.034899497, 0.052335956, 0.069756474, 0.087155743, 0.104528463, 0.121869343, 0.139173101, 0.156434465,
   0.173648178, 0.190808995, 0.207911691, 0.224951054, 0.241921896, 0.258819045, 0.275637356, 0.292371705, 0.309016994,
   0.325568154, 0.342020143, 0.358367950, 0.374606593, 0.390731129, 0.406736643, 0.422618262, 0.438371147, 0.453990500,
   0.469471563, 0.484809620, 0.500000000, 0.515038075, 0.529919264, 0.544639035, 0.559192903, 0.573576436, 0.587785252,
   0.601815023, 0.615661475, 0.629320391, 0.642787610, 0.656059029, 0.669130606, 0.681998360, 0.694658370, 0.707106781,
   0.719339800, 0.731353702, 0.743144825, 0.754709580, 0.766044443, 0.777145961, 0.788010754, 0.798635510, 0.809016994,
   0.819152044, 0.829037573, 0.838670568, 0.848048096, 0.857167301, 0.866025404, 0.874619707, 0.882947593, 0.891006524,
   0.898794046, 0.906307787, 0.913545458, 0.920504853, 0.927183855, 0.933580427, 0.939692621, 0.945518576, 0.951056516,
   0.956304756, 0.961261696, 0.965925826, 0.970295726, 0.974370065, 0.978147601, 0.981627183, 0.984807753, 0.987688341,
   0.990268069, 0.992546152, 0.994521895, 0.996194698, 0.997564050, 0.998629535, 0.999390827, 0.999847695, 1.000000000,
   0.999847695, 0.999390827, 0.998629535, 0.997564050, 0.996194698, 0.994521895, 0.992546152, 0.990268069, 0.987688341,
   0.984807753, 0.981627183, 0.978147601, 0.974370065, 0.970295726, 0.965925826, 0.961261696, 0.956304756, 0.951056516,
   0.945518576, 0.939692621, 0.933580427, 0.927183855, 0.920504853, 0.913545458, 0.906307787, 0.898794046, 0.891006524,
   0.882947593, 0.874619707, 0.866025404, 0.857167301, 0.848048096, 0.838670568, 0.829037573, 0.819152044, 0.809016994,
   0.798635510, 0.788010754, 0.777145961, 0.766044443, 0.754709580, 0.743144825, 0.731353702, 0.719339800, 0.707106781,
   0.694658370, 0.681998360, 0.669130606, 0.656059029, 0.642787610, 0.629320391, 0.615661475, 0.601815023, 0.587785252,
   0.573576436, 0.559192903, 0.544639035, 0.529919264, 0.515038075, 0.500000000, 0.484809620, 0.469471563, 0.453990500,
   0.438371147, 0.422618262, 0.406736643, 0.390731129, 0.374606593, 0.358367950, 0.342020143, 0.325568154, 0.309016994,
   0.292371705, 0.275637356, 0.258819045, 0.241921896, 0.224951054, 0.207911691, 0.190808995, 0.173648178, 0.156434465,
   0.139173101, 0.121869343, 0.104528463, 0.087155743, 0.069756474, 0.052335956, 0.034899497, 0.017452406};


/*!  Power of 2 lookup table so we don't have to use the pow function over and over when we open files.  So far we only need to go up
     to 2^32.  */

static const uint64_t power2[33] =
  {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576,
   2097152, 4194304, 8388608, 16777216, 33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, 2147483648, 4294967296};


/*!  Integer log base 2 lookup table (see czmil_int_log2 and czmil_short_log2 below).  */

static const int32_t logTable256[256] =
  {-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
   5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
   6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
   7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};


/*!  Bit pack/unpack masks.  */

static const uint8_t mask[8] = {0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe}, notmask[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01}; 



/*******************************************************************************************/
/*!

 - Function:    clean_exit

 - Purpose:     Exit from the application after first cleaning up memory and orphaned files.
                This will only be called in the case of an abnormal exit (like a failed malloc).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - ret            =    Value to be passed to exit ();  If set to -999 we were
                                      called from the SIGINT signal handler so we must return
                                      to allow it to SIGINT itself.

 - Returns:
                - void

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static void czmil_clean_exit (int32_t ret)
{
  int32_t i;

  for (i = 0 ; i < CZMIL_MAX_FILES ; i++)
    {
      /*  If we were in the process of creating a file we need to remove it since it isn't finished.  */

      if (cpf[i].fp != NULL)
        {
          if (cpf[i].created)
            {
              fclose (cpf[i].fp);
              remove (cpf[i].path);
            }
        }

      if (cwf[i].fp != NULL)
        {
          if (cwf[i].created)
            {
              fclose (cwf[i].fp);
              remove (cwf[i].path);
            }
        }

      if (csf[i].fp != NULL)
        {
          if (csf[i].created)
            {
              fclose (csf[i].fp);
              remove (csf[i].path);
            }
        }

      if (cif[i].fp != NULL)
        {
          if (cif[i].created)
            {
              fclose (cif[i].fp);
              remove (cif[i].path);
            }
        }
    }


  /*  If we were called from the SIGINT handler return there, otherwise just exit.  */

#if (defined _WIN32) && (defined _MSC_VER)
  if (ret == -999 && _getpid () > 1) return;
#else
  if (ret == -999 && getpid () > 1) return;
#endif

  exit (ret);
}



/*******************************************************************************************/
/*!

 - Function:    sigint_handler

 - Purpose:     Simple little SIGINT handler.  Allows us to clean up the files if we were 
                creating a new CZMIL file and someone does a CTRL-C.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        02/25/09

 - Arguments:
                - sig            =    The signal

 - Returns:
                - void

 - Caveats:     The way to do this was borrowed from Martin Cracauer's "Proper handling of
                SIGINT/SIGQUIT", http://www.cons.org/cracauer/sigint.html

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

static void czmil_sigint_handler (int sig)
{
  czmil_clean_exit (-999);

  signal (SIGINT, SIG_DFL);

#ifdef _WIN32
  raise (sig);
#else
  kill (getpid (), SIGINT);
#endif
}



/*******************************************************************************************/
/*!

  - Module Name:        get_string

  - Programmer(s):      Jan C. Depner

  - Date Written:       December 1994

  - Purpose:            Parses the input string for the equals sign and returns everything to
                        the right between the first and last non-blank character (inclusive).

  - Arguments:
                        - *in     =   Input string
                        - *out    =   Output string

  - Return Value:
                        - void

  - Caveats:            THIS FUNCTION DOES NOT HANDLE NEWLINE OR CARRIAGE RETURN!  If you
                        are reading a line from a file to pass to this function you must
                        either use ngets instead of fgets or strip the \n and, on Windows,
                        \r from the line.

                        This function is static, it is only used internal to the API and is not
                        callable from an external program.

*********************************************************************************************/

static void czmil_get_string (const char *in, char *out)
{
  int32_t              i, start, length;
  const char           *ptr;


  start = 0;
  length = 0;


  ptr = strchr (in, '=') + 1;


  /*  Search for first non-blank character.   */

  for (i = 0 ; i < strlen (ptr) ; i++)
    {
      if (ptr[i] != ' ')
        {
          start = i;
          break;
        }
    }


  /*  Search for last non-blank character.    */

  for (i = strlen (ptr) ; i >= 0 ; i--)
    {
      if (ptr[i] != ' ' && ptr[i] != 0)
        {
          length = (i + 1) - start;
          break;
        }
    }

  strncpy (out, &ptr[start], length);
  out[length] = 0;
}



/*******************************************************************************************/
/*!

 - Function:    int_log2

 - Purpose:     This is used to get the number of bits needed to store an unsigned 32 bit
                integer value.  The alternative is to use log functions, like this: <br>
                - bits = NINT (log10 ((double) v) / LOG2 + 0.5);
                Precomputing LOG2 saves a bit of time but the single log10 function is
                computationally expensive.  The lookup table method takes only about 7
                operations to find the log of a 32-bit value.  Just call this function and
                add one to the return value to get the number of bits needed, thusly: <br>
                - bits = int_log2 (v) + 1;
                This method was derived from Daniel Lemire's blog (http://lemire.me/blog/).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - v               =   integer value whose base 2 log we will compute

 - Returns:
                - log base 2 of the input integer

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

CZMIL_INLINE int32_t czmil_int_log2 (uint32_t v)
{
  uint32_t     r, t, tt;


  if ((tt = v >> 16))
    {
      r = (t = tt >> 8) ? 24 + logTable256[t] : 16 + logTable256[tt];
    }
  else
    {
      r = (t = v >> 8) ? 8 + logTable256[t] : logTable256[v];
    }

  return (r);
}


 
/*******************************************************************************************/
/*!

 - Function:    short_log2

 - Purpose:     This is used to get the number of bits needed to store an unsigned 16 bit
                integer value.  The alternative is to use log functions, like this: <br>
                - bits = NINT (log10 ((double) v) / LOG2 + 0.5);
                Precomputing LOG2 saves a bit of time but the single log10 function is
                computationally expensive.  The lookup table method takes only about 3
                operations to find the log of a 32-bit value.  Just call this function and
                add one to the return value to get the number of bits needed, thusly: <br>
                - bits = short_log2 (v) + 1;
                This method was derived from Daniel Lemire's blog (http://lemire.me/blog/).

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        06/13/12

 - Arguments:
                - v               =   16 bit integer value whose base 2 log we will compute

 - Returns:
                - log base 2 of the input integer

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

CZMIL_INLINE int32_t czmil_short_log2 (uint16_t v)
{
  uint32_t     r, t, v2;

  v2 = (uint32_t) v;

  r = (t = v2 >> 8) ? 8 + logTable256[t] : logTable256[v2];

  return (r);
}



/*******************************************************************************************/
/*!

 - Function:    czmil_bit_pack

 - Purpose:     Packs the unsigned, 32 bit value 'value' into 'numbits' bits in 'buffer'
                starting at bit position 'start'.  The majority of this code is based on
                Appendix C of Naval Ocean Research and Development Activity Report #236,
                'Data Base Structure to Support the Production of the Digital Bathymetric
                Data Base', Nov. 1989, James E. Braud, John L. Breckenridge, James E. Current,
                Jerry L. Landrum.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        1991?

 - Arguments:
                - buffer          =   pointer to uint8_t buffer to pack values into
                - start           =   start bit position in the buffer
                - numbits         =   number of bits to store
                - value           =   unsigned, 32 bit integer number to store in the buffer

 - Returns:
                - void

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

CZMIL_INLINE void czmil_bit_pack (uint8_t buffer[], uint32_t start, uint32_t numbits, int32_t value)
{
  int32_t                start_byte, end_byte, start_bit, end_bit, i;


  i = start + numbits;


  /*  Right shift the start and end by 3 bits, this is the same as        */
  /*  dividing by 8 but is faster.  This is computing the start and end   */
  /*  bytes for the field.                                                */

  start_byte = start >> 3;
  end_byte = i >> 3;


  /*  AND the start and end bit positions with 7, this is the same as     */
  /*  doing a mod with 8 but is faster.  Here we are computing the start  */
  /*  and end bits within the start and end bytes for the field.          */

  start_bit = start & 7;
  end_bit = i & 7;


  /*  Compute the number of bytes covered.                                */
     
  i = end_byte - start_byte - 1;


  /*  If the value is to be stored in one byte, store it.                 */

  if (start_byte == end_byte)
    {
      /*  Rather tricky.  We are masking out anything prior to the start  */
      /*  bit and after the end bit in order to not corrupt data that has */
      /*  already been stored there.                                      */

      buffer[start_byte] &= mask[start_bit] | notmask[end_bit];


      /*  Now we mask out anything in the value that is prior to the      */
      /*  start bit and after the end bit.  This is, of course, after we  */
      /*  have shifted the value left past the end bit.                   */

      buffer[start_byte] |= (value << (8 - end_bit)) & (notmask[start_bit] & mask[end_bit]);
    }


  /*  If the value covers more than 1 byte, store it.                     */

  else
    {
      /*  Here we mask out data prior to the start bit of the first byte. */

      buffer[start_byte] &= mask[start_bit];


      /*  Get the upper bits of the value and mask out anything prior to  */
      /*  the start bit.  As an example of what's happening here, if we   */
      /*  wanted to store a 14 bit field and the start bit for the first  */
      /*  byte is 3, we would be storing the upper 5 bits of the value in */
      /*  the first byte.                                                 */

      buffer[start_byte++] |= (value >> (numbits - (8 - start_bit))) & notmask[start_bit];


      /*  Loop while decrementing the byte counter.                       */

      while (i--)
        {
          /*  Clear the entire byte.                                      */

          buffer[start_byte] &= 0;


          /*  Get the next 8 bits from the value.                         */

          buffer[start_byte++] |= (value >> ((i << 3) + end_bit)) & 255;
        }


      /*  If the end bit is an exact multiple of 8 we are done.  If we    */
      /*  try to mess around with the next byte it will cause a memory    */
      /*  overflow.                                                       */

      if (end_bit)
        {
          /*  For the last byte we mask out anything after the end bit.   */

          buffer[start_byte] &= notmask[end_bit];


          /*  Get the last part of the value and stuff it in the end      */
          /*  byte.  The left shift effectively erases anything above     */
          /*  8 - end_bit bits in the value so that it will fit in the    */
          /*  last byte.                                                  */

          buffer[start_byte] |= (value << (8 - end_bit));
        }
    }
}



/*******************************************************************************************/
/*!

 - Function:    czmil_bit_unpack

 - Purpose:     Unpacks an unsigned, 32 bit value from 'numbits' bits in 'buffer' starting at
                bit position 'start'.  The value is assumed to be unsigned.  The majority of
                this code is based on Appendix C of Naval Ocean Research and Development
                Activity Report #236, 'Data Base Structure to Support the Production of the
                Digital Bathymetric Data Base', Nov. 1989, James E. Braud, John L. Breckenridge,
                James E. Current, Jerry L. Landrum.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        1991?

 - Arguments:
                - buffer          =   pointer to uint8_t buffer to unpack values from
                - start           =   start bit position in the buffer
                - numbits         =   number of bits to retrieve

 - Returns:
                - value           =   unsigned, 32 bit integer value retrieved from the buffer

 - Caveats      Note that the value that is output from this function is an unsigned 32
                bit integer.  Even though you may have passed in a signed 32 bit value to
                czmil_bit_pack it will not be sign extended on the way out.  If you just
                happened to store it in 32 bits you can just typecast it to a signed
                32 bit number and, lo and behold, you have a nice, signed number.  
                Otherwise, you have to do the sign extension yourself.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

*********************************************************************************************/

CZMIL_INLINE uint32_t czmil_bit_unpack (uint8_t buffer[], uint32_t start, uint32_t numbits)
{
  int32_t                start_byte, end_byte, start_bit, end_bit, i;
  uint32_t               value;


  i = start + numbits;


  /*  Right shift the start and end by 3 bits, this is the same as        */
  /*  dividing by 8 but is faster.  This is computing the start and end   */
  /*  bytes for the field.                                                */

  start_byte = start >> 3;
  end_byte = i >> 3;


  /*  AND the start and end bit positions with 7, this is the same as     */
  /*  doing a mod with 8 but is faster.  Here we are computing the start  */
  /*  and end bits within the start and end bytes for the field.          */

  start_bit = start & 7;
  end_bit = i & 7;


  /*  Compute the number of bytes covered.                                */

  i = end_byte - start_byte - 1;


  /*  If the value is stored in one byte, retrieve it.                    */

  if (start_byte == end_byte)
    {
      /*  Mask out anything prior to the start bit and after the end bit. */

      value = (uint32_t) buffer[start_byte] & (notmask[start_bit] & mask[end_bit]);


      /*  Now we shift the value to the right.                            */

      value >>= (8 - end_bit);
    }


  /*  If the value covers more than 1 byte, retrieve it.                  */

  else
    {
      /*  Here we mask out data prior to the start bit of the first byte  */
      /*  and shift to the left the necessary amount.                     */

      value = (uint32_t) (buffer[start_byte++] & notmask[start_bit]) << (numbits - (8 - start_bit));


      /*  Loop while decrementing the byte counter.                       */

      while (i--)
        {
          /*  Get the next 8 bits from the buffer.                        */

          value += (uint32_t) buffer[start_byte++] << ((i << 3) + end_bit);
        }


      /*  If the end bit is an exact multiple of 8 we are done.  If we    */
      /*  try to mess around with the next byte it will cause a memory    */
      /*  overflow.                                                       */

      if (end_bit)
        {
          /*  For the last byte we mask out anything after the end bit    */
          /*  and then shift to the right (8 - end_bit) bits.             */

          value += (uint32_t) (buffer[start_byte] & mask[end_bit]) >> (8 - end_bit);
        }
    }

  return (value);
}



/*******************************************************************************************/
/*!

 - Function:    czmil_double_bit_pack

 - Purpose:     Packs the unsigned, 64 bit value 'value' into 'numbits' bits in 'buffer'
                starting at bit position 'start'.  The majority of this code is based on
                Appendix C of Naval Ocean Research and Development Activity Report #236,
                'Data Base Structure to Support the Production of the Digital Bathymetric
                Data Base', Nov. 1989, James E. Braud, John L. Breckenridge, James E. Current,
                Jerry L. Landrum.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        1991?

 - Arguments:
                - buffer          =   pointer to uint8_t buffer to pack values into
                - start           =   start bit position in the buffer
                - numbits         =   number of bits to store
                - value           =   unsigned, 64 bit integer number to store in the buffer

 - Returns:
                - void

 - Caveats:     This function is static, it is only used internal to the API and is not
                callable from an external program.

                <b>NEVER call this function with numbits less than 33!</b>

*********************************************************************************************/

CZMIL_INLINE void czmil_double_bit_pack (uint8_t buffer[], uint32_t start, uint32_t numbits, int64_t value)
{
  int32_t            high_order, low_order;


  high_order = (int32_t) (((uint64_t) value) >> 32);
  low_order  = (int32_t) (value & UINT32_MAX);


  czmil_bit_pack (buffer, start, numbits - 32, high_order);
  czmil_bit_pack (buffer, start + (numbits - 32), 32, low_order);
}



/*******************************************************************************************/
/*!

 - Function:    czmil_double_bit_unpack

 - Purpose:     Unpacks an unsigned, 64 bit value from 'numbits' bits in 'buffer' starting at
                bit position 'start'.  The value is assumed to be unsigned.  The majority of
                this code is based on Appendix C of Naval Ocean Research and Development
                Activity Report #236, 'Data Base Structure to Support the Production of the
                Digital Bathymetric Data Base', Nov. 1989, James E. Braud, John L. Breckenridge,
                James E. Current, Jerry L. Landrum.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        1991?

 - Arguments:
                - buffer          =   pointer to uint8_t buffer to unpack values from
                - start           =   start bit position in the buffer
                - numbits         =   number of bits to retrieve

 - Returns:
                - value           =   unsigned, 64 bit integer value retrieved from the buffer

 - Caveats      Note that the value that is output from this function is an unsigned 64
                bit integer.  Even though you may have passed in a signed 64 bit value to
                czmil_double_bit_pack it will not be sign extended on the way out.  If you
                just happened to store it in 64 bits you can just typecast it to a signed
                64 bit number and, lo and behold, you have a nice, signed number.  
                Otherwise, you have to do the sign extension yourself.

                This function is static, it is only used internal to the API and is not
                callable from an external program.

                <b>NEVER call this function with numbits less than 33!</b>

*********************************************************************************************/

CZMIL_INLINE uint64_t czmil_double_bit_unpack (uint8_t buffer[], uint32_t start, uint32_t numbits)
{
  uint64_t          result;
  uint32_t          high_order, low_order;


  high_order = czmil_bit_unpack (buffer, start, numbits - 32);
  low_order  = czmil_bit_unpack (buffer, start + (numbits - 32), 32);


  result = ((uint64_t) high_order) << 32;
  result |= (uint64_t) low_order;

  return (result);
}



/*******************************************************************************************/
/*!

  - Module Name:        czmil_ngets

  - Programmer(s):      Jan C. Depner

  - Date Written:       May 1999

  - Purpose:            This is an implementation of fgets that strips the line feed (or
                        carriage return/line feed) off of the end of the string if present.

  - Arguments:          See fgets

  - Return Value:       See fgets

  - Caveats:            This function is static, it is only used internal to the API and is not
                        callable from an external program.

*********************************************************************************************/

static char *czmil_ngets (char *s, int32_t size, FILE *stream)
{
  if (fgets (s, size, stream) == NULL) return (NULL);

  while (strlen(s) > 0 && (s[strlen(s) - 1] == '\n' || s[strlen(s) - 1] == '\r')) s[strlen(s) - 1] = '\0';

  if (s[strlen (s) - 1] == '\n') s[strlen (s) - 1] = 0;


  return (s);
}
