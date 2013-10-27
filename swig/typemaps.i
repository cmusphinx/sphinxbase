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

#if SWIGJAVA
%typemap(javainterfaces) TYPE "Iterable<"#TYPE"Iterator>"
%typemap(javainterfaces) TYPE##Iterator "Iterator<"#VALUE_TYPE">"
#endif

%inline %{
typedef struct {
  PREFIX##_iter_t *ptr;
} TYPE##Iterator;
%}

typedef struct {} TYPE;

%exception TYPE##Iterator##::next() {
  $action
  if (!arg1->ptr) {
#if SWIGJAVA
    jclass cls = (*jenv)->FindClass(jenv, "java/util/NoSuchElementException");
    (*jenv)->ThrowNew(jenv, cls, null);
    return $null;
#elif SWIGPYTHON
    SWIG_SetErrorObj(PyExc_StopIteration, SWIG_Py_Void());
    SWIG_fail;
#endif
  }
}

#if SWIGJAVA
%exception TYPE##Iterator::remove {
  jclass cls =
    (*jenv)->FindClass(jenv, "java/lang/UnsupportedOperationException");
  (*jenv)->ThrowNew(jenv, cls, null);
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
    }

    return NULL;
  }

#if SWIGJAVA
  bool hasNext() {
    return $self->ptr != NULL;
  }

  void remove() {
    // Dummy method, see %exception wrapping above
  }
#endif
}

#if SWIGPYTHON
%extend TYPE {
  TYPE##Iterator * __iter__() {
    return new_##TYPE##Iterator(PREFIX##_iter($self));
  }
}
#elif SWIGJAVA

%extend TYPE {
  TYPE##Iterator * iterator() {
    return new_##TYPE##Iterator(PREFIX##_iter($self));
  }
}
#else
#warning "No wrappings for iterable types will be generated"
#endif

%enddef
