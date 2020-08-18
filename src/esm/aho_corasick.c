/* aho_corasick.c - Aho Corasick implementations
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

/* This file contains implementations of some of the algorithms for efficient
 * string matching described in the paper, 'Aho, A.V, and Corasick, M. J.
 * Efficient String Matching: An Aid to Bibliographic Search. Comm. ACM 18:6
 * (June 1975), 333-340', hereafter referred to as 'the paper.'
 *
 * The paper describes algorithms (Algorithm 2 and Algorithm 3) for
 * contructing the functions of a finite state machine from a set of keywords
 * and an algorithm (Algorithm 1) for running that machine against an input
 * string to output the occurences of the keywords in the input string.
 *
 * The paper also describes an algorithm (Algorithm 4) for eliminating the
 * failure function to create a deterministic finite automaton that makes
 * exactly one state transition per input symbol, but I have chosen not to
 * implement that algorithm.
 *
 * I have moved the code that implements the last part of Algorithm 2 (the
 * part that completes the goto function for the root state) into the function
 * that implements Algorithm 3. I have also made the loop over the set of
 * keywords the responsibilty of the user, leaving only Algorithm 2's 'enter'
 * procedure. These changes allow for a simple API where the user can create
 * an index with ac_index_new, add keywords with associated object to the
 * index with ac_index_enter (Algorithm 2's 'enter' procedure), then fix the
 * index with ac_index_fix (Algorithm 3 prefixed with the end of Algorithm 2)
 * before querying it with ac_index_query (Algorithm 1) and freeing it with
 * ac_index_free (not covered in the paper).
 */

#include "aho_corasick.h"
#include "ac_list.h"
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>

/**
 * Interface states for an index objects.
 */
typedef enum ac_index_state {
	/**
	 * The index is 'unfixed'. New keywords can be entered but the index
	 * cannot be queried.
	 */
	AC_INDEX_UNFIXED,

	/**
	 * The index in 'fixed'. The index can be queried but new keywords
	 * cannot be entered.
	 */
	AC_INDEX_FIXED
} ac_index_state;

/**
 * Structure for index objects.
 */
struct ac_index_s {
	/** The interface state of the index. */
	ac_index_state index_state;

	/** The top state node of the index's patten matching machine. */
	struct ac_state *state_0;
};

/**
 * Structure for a state in an Aho-Corasick pattern matching machine. The
 * structure holds the data for the the goto, failure and output functions for
 * a single state. In the paper, states are identified by a sequential integer
 * but this implementation uses the pointer to an ac_state structure.
 *
 * The data for the output function is split into two lists. One (outputs) is
 * built when keywords are entered into the index. The other (extra_outputs)
 * is built when the index is fixed.
 */
typedef struct ac_state {
	/**
	 * List of ac_goto structures used to evaluate the goto function for
	 * this state.
	 */
	ac_list *gotos;

	/**
	 * List of ac_output structures that are part of the result of the
	 * output function for this state. Items are added to these lists when
	 * keywords are entered.
	 */
	ac_list *outputs;

	/**
	 * List of ac_output structures that are part of the result of the
	 * output function for this state. Items are added to this list when
	 * the index is fixed.
	 */
	ac_list *extra_outputs;

	/**
	 * Result of failure function for this state.
	 */
	struct ac_state *failure;
} ac_state;

// --------------------------------------------------------------------------
// Goto list

/**
 * Structure mapping a symbol to a state, forming part of the goto function
 * data for a state.
 */
typedef struct ac_goto {
	char symbol;
	ac_state *state;
} ac_goto;

/**
 * Shared workspace used by ac_goto_list_free_item.
 */
typedef struct ac_goto_list_free_data {
	/**
	 * List of states to free. When the index is freed, as we iterate over
	 * the goto lists to free them, we add to a list of of state objects
	 * that will need to be freed later.
	 */
	ac_list *states;

	/**
	 * State that should not be added to the list above. When a goto item
	 * is encountered that points to this state it is not added to the
	 * list. This lets us cover the special case goto function of the root
	 * state, which can goto itself.
	 */
	ac_state *state;
} ac_goto_list_free_data;

/**
 * List free function for goto list items. This will be called once per
 * ac_goto in the list. The data should be an ac_goto_list_free_data as
 * explained above. The states in the list are added to the list in data
 * except where the state matches the state in data.
 *
 * Returns AC_SUCCESS if successful or AC_FAILURE if a failure is encountered
 * when adding a state to the list.
 */
ac_error_code ac_goto_list_free_item(void *item, void *data)
{
	ac_goto *goto_item = (ac_goto *) item;
	ac_goto_list_free_data *free_data = (ac_goto_list_free_data *) data;

	/* Add the state from the goto item to the list unless it is excluded.
	 */
	if (goto_item->state != free_data->state &&
	    ac_list_add(free_data->states, goto_item->state) != AC_SUCCESS) {
		return AC_FAILURE;
	}

	free(item);
	return AC_SUCCESS;
}

/**
 * Free a goto list, appending the states that do not match the state argument
 * to the list at states. Returns AC_SUCCESS if successful or AC_FAILURE
 * otherwise.
 */
ac_error_code
ac_goto_list_free(ac_list *self, ac_list *states, ac_state *state)
{
	ac_goto_list_free_data free_data = {states, state};
	return ac_list_free(self, ac_goto_list_free_item, &free_data);
}

/**
 * Search a goto list for the state associated with the given symbol. Return
 * a pointer to the state if it is found or NULL if not.
 */
ac_state *ac_goto_list_get(ac_list *self, char symbol)
{
	ac_list_item *list_item = self->first;
	ac_goto *item = NULL;

	while (list_item) {
		item = (ac_goto *) list_item->item;

		if (item->symbol == symbol) {
			return item->state;
		}
		list_item = list_item->next;
	}

	return NULL;
}

/**
 * Determine whether a goto list has an association for the given symbol.
 */
bool ac_goto_list_has(ac_list *self, char symbol)
{
	return ac_goto_list_get(self, symbol) != NULL;
}

/**
 * Associates the given symbol with the given state in a goto list. Returns
 * AC_SUCCESS if successful or AC_FAILURE if an error was encountered.
 */
ac_error_code ac_goto_list_add(ac_list *self, char symbol, ac_state *state)
{
	ac_goto *new_item;

	if (!(new_item = malloc(sizeof(ac_goto)))) {
		return AC_FAILURE;
	}
	new_item->symbol = symbol;
	new_item->state = state;

	if (ac_list_add(self, new_item) != AC_SUCCESS) {
		free(new_item);
		return AC_FAILURE;
	}

	return AC_SUCCESS;
}

// --------------------------------------------------------------------------
// Output list

/**
 * Structure holding the data for one item in the output function for a state.
 */
typedef struct ac_output {
	/**
	 * The length of the keyword that matches at the state. We use this to
	 * generate the index span of the query string that contains the
	 * keyword.
	 */
	ac_offset offset;

	/** An object assocated with the state. */
	void *object;
} ac_output;

/**
 * Structure holding the function pointer needed to free the the object
 * associated with an ac_output.
 */
struct ac_output_list_free_item_data {
	ac_free_function free_fn;
};

/**
 * List item free function for ac_output items. This funciton will be called
 * once for each item in each output list. The item argument points to the
 * ac_output being freed. The data argument points to a struct
 * ac_output_list_free_item_data holding a function for freeing the associated
 * object.
 *
 * Returns the result of the function for freeing the associated object.
 */
ac_error_code ac_output_list_free_item(void *item, void *data)
{
	ac_output *output = (ac_output *) item;
	ac_error_code result;

	struct ac_output_list_free_item_data *item_data = data;
	result = item_data->free_fn(output->object, NULL);
	// TODO: let user pass data to free function
	free(output);
	return result;
}

/**
 * Free the output list, calling object_free on the associated object of
 * each output item.
 */
void ac_output_list_free(ac_list *self, ac_free_function object_free)
{
	struct ac_output_list_free_item_data data = {.free_fn = object_free};
	(void) ac_list_free(self, ac_output_list_free_item, &data);
}

/**
 * Add an offset and an associated object to the output list. Returns
 * AC_SUCCESS if successful, or AC_FAILURE if the there was an error was
 * encountered.
 */
ac_error_code ac_output_list_add(ac_list *self, ac_offset offset, void *object)
{
	ac_output *new_item;

	if (!(new_item = malloc(sizeof(ac_output)))) {
		return AC_FAILURE;
	}

	new_item->offset = offset;
	new_item->object = object;

	if (ac_list_add(self, new_item) != AC_SUCCESS) {
		free(new_item);
		return AC_FAILURE;
	}

	return AC_SUCCESS;
}

/**
 * Add the contents of the output list in other to the output list in self.
 * Returns AC_SUCCESS if successful or AC_FAILURE if an error was encountered.
 */
ac_error_code ac_output_list_add_list(ac_list *self, ac_list *other)
{
	ac_list_item *list_item = other->first;
	ac_output *item = NULL;

	while (list_item) {
		item = (ac_output *) list_item->item;

		if (ac_output_list_add(self, item->offset, item->object) !=
		    AC_SUCCESS) {
			return AC_FAILURE;
		}

		list_item = list_item->next;
	}

	return AC_SUCCESS;
}

// --------------------------------------------------------------------------
// Callbacks

/**
 * Call the callback with an single result. Returns AC_SUCCESS if successful,
 * or AC_FAILURE if the there was an error was encountered.
 */
ac_error_code ac_cb_output(ac_result_callback result_cb,
                           void *result_cb_data,
                           ac_offset start,
                           ac_offset end,
                           void *object)
{
	ac_result new_item = {.start = start, .end = end, .object = object};

	return result_cb(result_cb_data, &new_item);
}

/**
 * Send each item in the output list to the callback.
 * Returns AC_SUCCESS if successful or AC_FAILURE if an error was encountered.
 */
ac_error_code ac_cb_outputs(ac_result_callback result_cb,
                            void *result_cb_data,
                            ac_list *outputs,
                            ac_offset end)
{
	ac_list_item *list_item = NULL;
	ac_output *item = NULL;

	list_item = outputs->first;

	while (list_item) {
		item = (ac_output *) list_item->item;
		if (ac_cb_output(result_cb,
		                 result_cb_data,
		                 end - item->offset + 1,
		                 end + 1,
		                 item->object) != AC_SUCCESS) {
			return AC_FAILURE;
		}

		list_item = list_item->next;
	}

	return AC_SUCCESS;
}

// --------------------------------------------------------------------------
// State object

/**
 * Construct a new state object. The goto list and the both output lists are
 * initially empty, and the failure state is initially NULL. Returns a pointer
 * to the new state object if successful or NULL if an error was encountered
 * while trying to allocate heap space for the object or while constructing
 * one of its lists.
 */
ac_state *ac_state_new(void)
{
	ac_state *self;

	if (!(self = malloc(sizeof(ac_state)))) {
		return NULL;
	}

	if (!(self->gotos = ac_list_new())) {
		return NULL;
	}

	if (!(self->outputs = ac_list_new())) {
		return NULL;
	}

	if (!(self->extra_outputs = ac_list_new())) {
		return NULL;
	}

	self->failure = NULL;
	return self;
}

/**
 * Free the state object. Any states in the goto list will be added to the
 * state queue in the children argument. Relavent associated objects from the
 * output list will be freed by calling the object_free argument. Returns
 * AC_SUCCESS if sucessful, or AC_FAILURE if an error was encountered.
 */
ac_error_code
ac_state_free(ac_state *self, ac_list *children, ac_free_function object_free)
{
	if (!self) {
		return AC_FAILURE;
	}

	if (ac_goto_list_free(self->gotos, children, self) != AC_SUCCESS) {
		return AC_FAILURE;
	}

	// We need to call object_free for the associated objects in outputs,
	// but must not call object_free for the extra_outputs.
	ac_output_list_free(self->outputs, object_free);
	ac_output_list_free(self->extra_outputs, ac_list_free_keep_item);

	free(self);

	return AC_SUCCESS;
}

// --------------------------------------------------------------------------
// State queue

/**
 * Free the state queue.
 */
void ac_state_queue_free(ac_list *self)
{
	(void) ac_list_free(self, ac_list_free_keep_item, NULL);
}

/**
 * Pop the next item from the state queue. Returns a pointer to the popped
 * item if successful or NULL if the queue was empty.
 */
ac_state *ac_state_queue_get(ac_list *self)
{
	ac_state *result = NULL;
	ac_list_item *next = NULL;

	if (self && self->first) {
		result = (ac_state *) self->first->item;
		next = self->first->next;
		free(self->first);
		self->first = next;
	}

	if (self->first == NULL) {
		self->last = NULL;
	}

	return result;
}

// --------------------------------------------------------------------------
// Index object

ac_index ac_index_new(void)
{
	ac_index self;

	if (!(self = malloc(sizeof(struct ac_index_s)))) {
		return NULL;
	}

	if (!(self->state_0 = ac_state_new())) {
		return NULL;
	}

	self->index_state = AC_INDEX_UNFIXED;

	return self;
}

ac_error_code ac_index_free(ac_index self, ac_free_function object_free)
{
	ac_list *queue = NULL;
	ac_state *state = NULL;
	ac_error_code result = AC_SUCCESS;

	if (!self) {
		return AC_FAILURE;
	}

	if (!(queue = ac_list_new())) {
		return AC_FAILURE;
	}

	// Free all the state nodes by following the goto function tree breadth
	// first, starting with state_0.
	state = self->state_0;

	while (state) {
		// Free the state and enqueue the states from the goto list.
		if (ac_state_free(state, queue, object_free) != AC_SUCCESS) {
			result = AC_FAILURE;
		}

		state = ac_state_queue_get(queue);
	}

	ac_state_queue_free(queue); // The queue should be empty.

	self->state_0 = NULL;
	free(self);

	return result;
}

ac_error_code
ac_index_enter(ac_index self, char *keyword, ac_offset size, void *object)
{
	// This is an implementation of the enter procedure of 'Algorithm 2.
	// Construction of the goto function.' from the paper.

	ac_state *state = self->state_0;
	ac_offset j = 0;
	ac_state *new_state = NULL;

	// You can't enter strings into a fixed index.
	if (self->index_state != AC_INDEX_UNFIXED) {
		return AC_FAILURE;
	}

	// Make sure that the goto tree has a path that spells out the keyword.
	// First skip the front of the the keyword if a matching path already
	// exists...
	while (j < size &&
	       (new_state = ac_goto_list_get(state->gotos, keyword[j]))) {

		state = new_state;
		++j;
	}

	// ... then build the nodes for the rest of the keyword, if any.
	while (j < size) {
		if (!(new_state = ac_state_new())) {
			return AC_FAILURE;
		}

		if (ac_goto_list_add(state->gotos, keyword[j], new_state) !=
		    AC_SUCCESS) {
			return AC_FAILURE;
		}

		state = new_state;
		++j;
	}

	// Now add an output for the keyword and associated object.
	if (ac_output_list_add(state->outputs, size, object) != AC_SUCCESS) {
		return AC_FAILURE;
	}

	return AC_SUCCESS;
}

/**
 * Fix the index, making it ready to be queried. This is an implementation of
 * the last part of Algorithm 2 from the paper combined with an implementation
 * of 'Algorithm 3. Construction of the failure function.' from the paper.
 * Returns AC_SUCCESS if the index was successfully fixed or AC_FAILURE if an
 * error was encountered or if the index was not 'unfixed'.
 */
ac_error_code ac_index_fix(ac_index self)
{
	// This is an implementation of the last part of Algorithm 2 from the
	// paper combined with an implementation of 'Algorithm 3. Construction
	// of the failure function.' from the paper.

	int symbol;
	ac_state *state = NULL;
	ac_state *r = NULL;
	ac_list *queue = NULL;
	ac_list_item *list_item = NULL;
	ac_goto *item = NULL;

	// You can't fix an index that is already fixed.
	if (self->index_state != AC_INDEX_UNFIXED) {
		return AC_FAILURE;
	}

	// Mark the index as being fixed.
	self->index_state = AC_INDEX_FIXED;

	// Make a temporary queue of states.
	if (!(queue = ac_list_new())) {
		return AC_FAILURE;
	}

	// Look at all the symbols. If state_0 has a goto for a symbol, add the
	// state to the queue and point the failure state back to state_0 - the
	// first part of Algorithm 3. Otherwise make state_0 goto itself for
	// that symbol - the last part of Algorithm 2.
	// TODO: Improve efficiency of state_0 to state_0 gotos.
	// NOTE: ac_goto_list_get is a linear scan, so we want to have symbols
	// that occur frequently in the query phrase to be near the start of
	// the list. By iterating from 0 instead of CHAR_MIN (-127), we put the
	// common 7-bit ASCII codes near the start of the list.
	for (symbol = 0; symbol <= CHAR_MAX - CHAR_MIN; symbol++) {
		// Passing int symbol as a char is a deliberate overflow here.
		if ((state = ac_goto_list_get(self->state_0->gotos, symbol))) {
			if (ac_list_add(queue, state) != AC_SUCCESS) {
				return AC_FAILURE;
			}
			state->failure = self->state_0;
		} else {
			if (ac_goto_list_add(self->state_0->gotos,
			                     symbol,
			                     self->state_0) != AC_SUCCESS) {
				return AC_FAILURE;
			}
		}
	}

	// Do the second part of Algorithm 3. Burn through the queue, enqueing
	// states from goto list in order to traverse the goto tree
	// breadth-first.
	// ...
	while ((r = ac_state_queue_get(queue))) {
		list_item = r->gotos->first;

		while (list_item) {
			item = (ac_goto *) list_item->item;
			symbol = item->symbol;

			if (ac_list_add(queue, item->state) != AC_SUCCESS) {
				return AC_FAILURE;
			}

			// ... For each goto state, find the failure function
			// by following the failure function back until there
			// is a goto defined. We will always find a defined
			// goto because state_0 has a goto defined for every
			// symbol (by now). ...
			state = r->failure;

			while (!ac_goto_list_has(state->gotos, symbol)) {
				state = state->failure;
			}

			item->state->failure =
			    ac_goto_list_get(state->gotos, symbol);

			// ... Add the outputs for the failure state to the
			// outputs. We use the extra_outputs list because the
			// outputs are already referenced.
			if (ac_output_list_add_list(
			        item->state->extra_outputs,
			        item->state->failure->outputs)) {
				return AC_FAILURE;
			};

			if (ac_output_list_add_list(
			        item->state->extra_outputs,
			        item->state->failure->extra_outputs)) {
				return AC_FAILURE;
			};

			list_item = list_item->next;
		}
	}

	// Free the temporary queue.
	ac_state_queue_free(queue);

	return AC_SUCCESS;
}

bool ac_index_fixed(ac_index self)
{
	return self->index_state == AC_INDEX_FIXED;
}

ac_error_code ac_index_query_cb(ac_index self,
                                char *phrase,
                                ac_offset size,
                                ac_result_callback result_cb,
                                void *result_cb_data)
{
	// This function is an implementation of 'Algorithm 1. Pattern matching
	// machine.' from the paper.
	ac_state *state = self->state_0;
	ac_state *next = NULL;
	ac_offset j = 0;

	// You can't query an index that isn't fixed.
	if (self->index_state != AC_INDEX_FIXED) {
		return AC_FAILURE;
	}

	// You must not provide a NULL callback.
	if (!result_cb) {
		return AC_FAILURE;
	}

	// Run the pattern matching machine. Iterate over the symbols in the
	// phrase. ...
	for (; j < size; ++j) {

		// ... If there is no goto for the symbol. Follow the failure
		// functions until there is. We will always find our way to a
		// state with a goto defined for the symbol because, once the
		// index is fixed, state_0 has gotos defined for every symbol.
		// ...
		while (!(next = ac_goto_list_get(state->gotos, phrase[j]))) {
			state = state->failure;
		}

		state = next;

		// ... Add the outputs for the state. If there is no match, the
		// state will be state_0 which always has no outputs. ...
		if (ac_cb_outputs(
		        result_cb, result_cb_data, state->outputs, j) !=
		    AC_SUCCESS) {
			return AC_FAILURE;
		};

		if (ac_cb_outputs(
		        result_cb, result_cb_data, state->extra_outputs, j) !=
		    AC_SUCCESS) {
			return AC_FAILURE;
		};
	}

	return AC_SUCCESS;
}

// TODO: Add ac_index_unfix method to unfix the index.
