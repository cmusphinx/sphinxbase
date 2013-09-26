%include <exception.i>

#if SWIGPYTHON
%include python.i
#elif SWIGJAVA
%include java.i
#endif

// Define typemaps to wrap error codes returned by some functions,
// into runtime exceptions.
%typemap(in, numinputs=0, noblock=1) int *errcode {
  int errcode;
  $1 = &errcode;
}

%typemap(argout) int *errcode {
  if (*$1 < 0) {
    char buf[64];
    sprintf(buf, "$symname returned %d", *$1);
    SWIG_exception(SWIG_RuntimeError, buf);
  }
}

// Macro to construct iterable objects.
%define iterable(TYPE, PREFIX, VALUE_TYPE)

%inline %{
typedef struct {
  PREFIX##_iter_t *ptr;
} TYPE##Iterator;
%}

typedef struct {} TYPE;

#if SWIGPYTHON
%exception TYPE##Iterator##::next() {
  $action
  if (!arg1->ptr) {
    PyErr_SetString(PyExc_StopIteration, "");
    SWIG_fail;
  }
}
#endif

%extend TYPE##Iterator {
  TYPE##Iterator(PREFIX##_iter_t *ptr) {
    TYPE##Iterator *iter = ckd_malloc(sizeof *iter);
    iter->ptr = ptr;
    return iter;
  }

  ~TYPE##Iterator() {
    PREFIX##_iter_free($self->ptr);
    ckd_free($self);
  }

  VALUE_TYPE * next() {
    if ($self->ptr) {
      VALUE_TYPE *value = next_##TYPE##Iterator($self->ptr);
      $self->ptr = PREFIX##_iter_next($self->ptr);
      return value;
    } else {
      return NULL;
    }
  }
}

%extend TYPE {
  TYPE##Iterator * __iter__() {
    return new_##TYPE##Iterator(PREFIX##_iter($self));
  }
}
%enddef
