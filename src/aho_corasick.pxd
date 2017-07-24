cdef extern from "esm/aho_corasick.h":
    ctypedef enum ac_error_code:
        AC_SUCCESS = 0
        AC_FAILURE = 1
    ctypedef ac_error_code (*ac_free_function)(void *item, void *data)
    ctypedef int ac_offset
    ctypedef struct ac_result:
        ac_offset start
        ac_offset end
        void *object
    ctypedef ac_error_code (*ac_result_callback)(void* result_cb_data,
                                                 ac_result* result)
    ctypedef struct ac_index_s:
        pass
    ctypedef ac_index_s* ac_index

    ac_index ac_index_new()
    ac_error_code ac_index_free(ac_index self,
                                ac_free_function object_free)

    ac_error_code ac_index_enter(ac_index self,
                                 char* keyword,
                                 ac_offset size,
                                 void* object)

    ac_error_code ac_index_fix(ac_index self)
    bint ac_index_fixed(ac_index self)

    ac_error_code ac_index_query_cb(ac_index self,
                                    char* phrase,
                                    ac_offset size,
                                    ac_result_callback result_cb,
                                    void* result_cb_data)

cdef extern from "Python.h":
    void Py_IncRef(obj)
    void Py_DecRef(obj)
