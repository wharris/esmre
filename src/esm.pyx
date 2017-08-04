cimport aho_corasick

cdef aho_corasick.ac_error_code decref_result_object(void *item, void* data):
    aho_corasick.Py_DecRef(<object>item)
    return aho_corasick.AC_SUCCESS

cdef aho_corasick.ac_error_code append_result(void* data,
                                              aho_corasick.ac_result* result):
    result_list = <list>data
    result_tuple = ((result.start, result.end), <object>result.object)
    result_list.append(result_tuple)
    return aho_corasick.AC_SUCCESS

cdef text_as_bytes(text):
    if type(text) is unicode:
        return text.encode('utf8')
    else:
        return text

cdef class Index:
    cdef aho_corasick.ac_index _index
    def __cinit__(self):
        self._index = aho_corasick.ac_index_new()
        if self._index is NULL:
            raise MemoryError()

    def __dealloc__(self):
        aho_corasick.ac_index_free(self._index, decref_result_object)


    def enter(self, keyword, obj=None):
        if obj is None:
            obj = keyword

        keyword = text_as_bytes(keyword)

        if aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call enter after fix")

        if aho_corasick.ac_index_enter(self._index,
                                       keyword, len(keyword),
                                       <void*>obj) != aho_corasick.AC_SUCCESS:
            raise MemoryError()
        aho_corasick.Py_IncRef(obj)

    def fix(self):
        if aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call fix repeatedly")

        if aho_corasick.ac_index_fix(self._index) != aho_corasick.AC_SUCCESS:
            raise MemoryError()

    def query(self, phrase):
        if not aho_corasick.ac_index_fixed(self._index):
            raise TypeError("Can't call query before fix")

        phrase = text_as_bytes(phrase)

        result_list = []

        status = aho_corasick.ac_index_query_cb(self._index,
                                          phrase,
                                          len(phrase),
                                          append_result,
                                          <void*>result_list)
        if status != aho_corasick.AC_SUCCESS:
            raise MemoryError()

        return result_list
