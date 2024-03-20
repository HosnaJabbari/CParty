#ifndef VIENNA_RNA_PACKAGE_STRING_UTILS_H
#define VIENNA_RNA_PACKAGE_STRING_UTILS_H

#ifdef VRNA_WARN_DEPRECATED
# if defined(__clang__)
#  define DEPRECATED(func, msg) func __attribute__ ((deprecated("", msg)))
# elif defined(__GNUC__)
#  define DEPRECATED(func, msg) func __attribute__ ((deprecated(msg)))
# else
#  define DEPRECATED(func, msg) func
# endif
#else
# define DEPRECATED(func, msg) func
#endif

/**
 *  @file     ViennaRNA/utils/strings.h
 *  @ingroup  utils, string_utils
 *  @brief    General utility- and helper-functions for RNA sequence and structure strings used throughout the ViennaRNA Package
 */

/**
 *  @addtogroup   string_utils
 *  @{
 *  @brief  Functions to parse, convert, manipulate, create, and compare (nucleic acid sequence) strings.
 */

#include <stdarg.h>
#include <ViennaRNA/datastructures/basic.h>

/**
 * @brief Stringify a macro after expansion
 */
#define XSTR(s) STR(s)

/**
 * @brief Stringify a macro argument
 */
#define STR(s) #s

#ifndef FILENAME_MAX_LENGTH

/**
 *  @brief Maximum length of filenames that are generated by our programs
 *
 *  This definition should be used throughout the complete ViennaRNA package
 *  wherever a static array holding filenames of output files is declared.
 */
#define FILENAME_MAX_LENGTH   80

/**
 *  @brief Maximum length of id taken from fasta header for filename generation
 *
 *  this has to be smaller than FILENAME_MAX_LENGTH since in most cases,
 *  some suffix will be appended to the ID
 */
#define FILENAME_ID_LENGTH    42

#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#ifndef HAVE_STRDUP
char *strdup(const char *s);


#endif
#endif

/**
 *  @brief Safely create a formatted string
 *
 *  This function is a safe implementation for creating a formatted character array,
 *  similar to @em sprintf.
 *  Internally, it uses the @em asprintf function if available to dynamically allocate
 *  a large enough character array to store the supplied content. If @em asprintf is
 *  not available, mimic it's behavior using @em vsnprintf.
 *
 *  @note The returned pointer of this function should always be passed to @em free() to
 *  release the allocated memory
 *
 *  @see vrna_strdup_vprintf(), vrna_strcat_printf()
 *
 *  @param  format  The format string (See also asprintf)
 *  @param  ...     The list of variables used to fill the format string
 *  @return         The formatted, null-terminated string, or NULL if something has gone wrong
 */
char *
vrna_strdup_printf(const char *format,
                   ...);


/**
 *  @brief Safely create a formatted string
 *
 *  This function is the @em va_list version of vrna_strdup_printf()
 *
 *  @note The returned pointer of this function should always be passed to @em free() to
 *  release the allocated memory
 *
 *  @see vrna_strdup_printf(), vrna_strcat_printf(), vrna_strcat_vprintf()
 *
 *  @param  format  The format string (See also asprintf)
 *  @param  argp    The list of arguments to fill the format string
 *  @return         The formatted, null-terminated string, or NULL if something has gone wrong
 */
char *
vrna_strdup_vprintf(const char  *format,
                    va_list     argp);


/**
 *  @brief Safely append a formatted string to another string
 *
 *  This function is a safe implementation for appending a formatted character array,
 *  similar to a cobination of @em strcat and @em sprintf.
 *  The function automatically allocates enough memory to store both, the previous
 *  content stored at @p dest and the appended format string. If the @p dest pointer
 *  is NULL, the function allocate memory only for the format string.
 *  The function returns the number of characters in the resulting string or -1
 *  in case of an error.
 *
 *  @see vrna_strcat_vprintf(), vrna_strdup_printf(), vrna_strdup_vprintf()
 *
 *  @param  dest    The address of a char *pointer where the formatted string is to be appended
 *  @param  format  The format string (See also sprintf)
 *  @param  ...     The list of variables used to fill the format string
 *  @return         The number of characters in the final string, or -1 on error
 */
int
vrna_strcat_printf(char       **dest,
                   const char *format,
                   ...);


/**
 *  @brief Safely append a formatted string to another string
 *
 *  This function is the @em va_list version of vrna_strcat_printf()
 *
 *  @see vrna_strcat_printf(), vrna_strdup_printf(), vrna_strdup_vprintf()
 *
 *  @param  dest    The address of a char *pointer where the formatted string is to be appended
 *  @param  format  The format string (See also sprintf)
 *  @param  args    The list of argument to fill the format string
 *  @return         The number of characters in the final string, or -1 on error
 */
int
vrna_strcat_vprintf(char        **dest,
                    const char  *format,
                    va_list     args);


/**
 *  @brief Split a string into tokens using a delimiting character
 *
 *  This function splits a string into an array of strings using a single
 *  character that delimits the elements within the string. The default
 *  delimiter is the ampersand @c '&' and will be used when @c NULL is
 *  passed as a second argument. The returned list is NULL terminated, i.e.
 *  the last element is @c NULL. If the delimiter is not found, the returned
 *  list contains exactly one element: the input string.
 *
 *  For instance, the following code:
 *
 * @code{.c}
 * char **tok = vrna_strsplit("GGGG&CCCC&AAAAA", NULL);
 *
 * for (char **ptr = tok; *ptr; ptr++) {
 *  printf("%s\n", *ptr);
 *  free(*ptr);
 * }
 * free(tok);
 * @endcode
 *  produces this output:
 *
 * @verbatim
 * GGGG
 * CCCC
 * AAAAA
 * @endverbatim
 *  and properly free's the memory occupied by the returned element array.
 *
 *  @note This function internally uses @em strtok_r() and is therefore
 *  considered to be thread-safe. Also note, that it is the users responsibility
 *  to free the memory of the array and that of the individual element strings!
 *
 *  @param  string    The input string that should be split into elements
 *  @param  delimiter The delimiting character. If @c NULL, the delimiter is @c "&"
 *  @return           A @c NULL terminated list of the elements in the string
 */
char **
vrna_strsplit(const char  *string,
              const char  *delimiter);


char *
vrna_strjoin(const char **strings,
             const char *delimiter);

/**
 *  @brief Create a random string using characters from a specified symbol set
 *
 *  @param l        The length of the sequence
 *  @param symbols  The symbol set
 *  @return         A random string of length 'l' containing characters from the symbolset
 */
char *
vrna_random_string(int        l,
                   const char symbols[]);


/**
 *  @brief Calculate hamming distance between two sequences
 *
 *  @param s1   The first sequence
 *  @param s2   The second sequence
 *  @return     The hamming distance between s1 and s2
 */
int
vrna_hamming_distance(const char  *s1,
                      const char  *s2);


/**
 *  @brief Calculate hamming distance between two sequences up to a specified length
 *
 *  This function is similar to vrna_hamming_distance() but instead of comparing both sequences
 *  up to their actual length only the first 'n' characters are taken into account
 *  @param  s1  The first sequence
 *  @param  s2  The second sequence
 *  @param  n   The length of the subsequences to consider (starting from the 5' end)
 *  @return     The hamming distance between s1 and s2
 */
int
vrna_hamming_distance_bound(const char  *s1,
                            const char  *s2,
                            int         n);


/**
 *  @brief Convert an input sequence (possibly containing DNA alphabet characters) to RNA alphabet
 *
 *  This function substitudes <i>T</i> and <i>t</i> with <i>U</i> and <i>u</i>, respectively
 *
 *  @param sequence The sequence to be converted
 */
void
vrna_seq_toRNA(char *sequence);


/**
 *  @brief Convert an input sequence to uppercase
 *
 *  @param sequence The sequence to be converted
 */
void
vrna_seq_toupper(char *sequence);


/**
 *  @brief  Remove gap characters from a nucleotide sequence
 *
 *  @param  sequence  The original, null-terminated nucleotide sequence
 *  @return           A copy of the input sequence with all gap characters removed
 */
char *
vrna_seq_ungapped(const char *seq);


/**
 *  @brief Add a separating '&' character into a string according to cut-point position
 *
 *  If the cut-point position is less or equal to zero, this function just
 *  returns a copy of the provided string. Otherwise, the cut-point character
 *  is set at the corresponding position
 *
 *  @param  string    The original string
 *  @param  cp        The cut-point position
 *  @return           A copy of the provided string including the cut-point character
 */
char *
vrna_cut_point_insert(const char  *string,
                      int         cp);


/**
 *  @brief  Remove a separating '&' character from a string
 *
 *  This function removes the cut-point indicating '&' character from a string
 *  and memorizes its position in a provided integer variable. If not '&' is
 *  found in the input, the integer variable is set to -1. The function returns
 *  a copy of the input string with the '&' being sliced out.
 *
 *  @param  string  The original string
 *  @param  cp      The cut-point position
 *  @return         A copy of the input string with the '&' being sliced out
 */
char *
vrna_cut_point_remove(const char  *string,
                      int         *cp);


/**
 *  @}
 */

#ifndef VRNA_DISABLE_BACKWARD_COMPATIBILITY

/**
 *  @brief Convert an input sequence to uppercase
 *  @deprecated   Use vrna_seq_toupper() instead!
 */
DEPRECATED(void  str_uppercase(char *sequence), "Use vrna_seq_toupper() instead");

/**
 *  @brief Convert a DNA input sequence to RNA alphabet
 *
 *  @deprecated Use vrna_seq_toRNA() instead!
 */
DEPRECATED(void str_DNA2RNA(char *sequence), "Use vrna_seq_toRNA() instead");

/**
 *  @brief Create a random string using characters from a specified symbol set
 *
 *  @deprecated Use vrna_random_string() instead!
 */
DEPRECATED(char *random_string(int        l,
                               const char symbols[]),
           "Use vrna_random_string() instead");

/**
 *  @brief Calculate hamming distance between two sequences
 *
 *  @deprecated Use vrna_hamming_distance() instead!
 */
DEPRECATED(int hamming(const char *s1,
                       const char *s2),
           "Use vrna_hamming_distance() instead");

/**
 *  @brief Calculate hamming distance between two sequences up to a specified length
 *
 *  @deprecated Use vrna_hamming_distance_bound() instead!
 */
DEPRECATED(int hamming_bound(const char *s1,
                             const char *s2,
                             int        n),
           "Use vrna_hamming_distance_bound() instead");

#endif

#endif
