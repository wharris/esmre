/* aho_corasick.h - declarations for Aho Corasick implementations
   Copyright (C) 2007 Tideway Systems Limited.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef AHO_CORASICK_H
#define AHO_CORASICK_H

#include <stdbool.h>

typedef enum { AC_SUCCESS = 0, AC_FAILURE = 1 } ac_error_code;

/**
 * Type for function pointers used for freeing complex item types.
 */
typedef ac_error_code (*ac_free_function)(void *item, void *data);

/**
 * Type for the index into a string of symbols or the length of a string of
 * symbols. We use 0-based indexing.
 */
typedef int ac_offset;

/**
 * Structure for a single query match, containing the span of the query phrase
 * that matched the keyword and a pointer associated with the keyword.
 */
typedef struct ac_result {
	/**
	 * The offset of the first symbol in the matching substring of the
	 * query phrase.
	 */
	ac_offset start;

	/** The offset of the symbol after the last symbol in the matching
	 * substring of the query phrase.
	 */
	ac_offset end;

	/** Pointer associated with the keyword. */
	void *object;
} ac_result;

/**
 * Type for result callback functions.
 */
typedef ac_error_code (*ac_result_callback)(void *result_cb_data,
                                            ac_result *result);

// Operations for index objects.

typedef struct ac_index_s *ac_index;

/**
 * Construct a new index. The state is initially 'unfixed', meaning that the
 * user can enter new keywords but can not query the index.
 *
 * @return a pointer to the ac_index or NULL if an error was encountered.
 */
ac_index ac_index_new(void);

/**
 * Free the index and all its subordinate objects. The provided object_free
 * function should do whatever necessary to free each associated object. It is
 * called once for each associated object that was given to ac_index_enter.
 *
 * @param self the index
 * @param object_free function that frees associated objects.
 * @return AC_SUCCESS if successful of AC_FAILURE is an error was encountered.
 */
ac_error_code ac_index_free(ac_index self, ac_free_function object_free);

/**
 * Add a keyword and an associated object to the index.
 *
 * @param self the index
 * @param keyword pointer to an array of symbols that comprise the keyword
 * @param size the number of symbols in the keyword
 * @param object pointer to an associated object that is passed to the result
 *               callback when the keyword is matched.
 * @return AC_SUCCESS if the keyword and object were added successfully or
 *         AC_FAILURE if an error was encountered or if the index has already
 *         been fixed.
 */
ac_error_code
ac_index_enter(ac_index self, char *keyword, ac_offset size, void *object);

/**
 * Fix the index, making it ready to be queried.
 *
 * @param self the index
 * @return AC_SUCCESS if the index was successfully fixed or AC_FAILURE if an
 *         error was encountered or if the index has already been fixed.
 */
ac_error_code ac_index_fix(ac_index self);

/**
 * Check if the index is fixed and ready to be queried. The index is initially
 * unfixed, meaning new keywords may be entered, but the index cannot be
 * queried. When an index is fixed (using ac_index_fix) it is ready to be
 * queried, but new keywords may not be entered.
 *
 * @param self the index
 * @return true if the index state is fixed, or false if the index is unfixed.
 */
bool ac_index_fixed(ac_index self);

/**
 * Query the index with the given phrase of the given size. Matching keyword
 * spans and associated objects are sent with result_cb_data to result_cb.
 *
 * @param self the index
 * @param phrase pointer to an array of symbols that will be searched
 * @param size the number of symbols in the the array
 * @param result_cb function that will be called whenever a match is found
 * @param result_cb_data pointer to a context object that will be passed to the
 *                       callback whenever a keyword is matched
 * @return AC_SUCCESS if the query was successful (even if there were no
 *         matches) or AC_FAILURE if an error was encountered
 */
ac_error_code ac_index_query_cb(ac_index self,
                                char *phrase,
                                ac_offset size,
                                ac_result_callback result_cb,
                                void *result_cb_data);

#endif // AHO_CORASICK_H
