
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "s3u_arraylist.h"
#include "ckd_alloc.h"

#define S3U_ARRAYLIST_INDEX(_al, _i)	((_al->head + _i) % _al->max)

static void
s3u_arraylist_expand(s3u_arraylist_t *_arraylist, int _resize);

static void
s3u_arraylist_expand_to_size(s3u_arraylist_t *_arraylist, int _resize);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_init(s3u_arraylist_t *_al)
{
  s3u_arraylist_init_size(_al, S3U_ARRAYLIST_DEFAULT_SIZE);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_init_size(s3u_arraylist_t *_al, int _size)
{
  assert(_al != NULL);

  _al->array = NULL;
  _al->head = 0;
  _al->count = 0;
  _al->max = 1;

  s3u_arraylist_expand_to_size(_al, _size);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_close(s3u_arraylist_t *_al)
{
  assert(_al != NULL);

  ckd_free(_al->array);
  _al->array = NULL;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_clear(s3u_arraylist_t *_al)
{
  int i;

  assert(_al != NULL);
  for (i = _al->max - 1; i >= 0; i--)
    _al->array[i] = NULL;
  _al->head = 0;
  _al->count = 0;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_set(s3u_arraylist_t *_al, int _index, void *_ptr)
{
  assert(_al != NULL);

  if (_index >= _al->max)
    s3u_arraylist_expand(_al, _index + 1);

  _al->array[S3U_ARRAYLIST_INDEX(_al, _index)] = _ptr;
  
  if (_index >= _al->count)
    _al->count = _index + 1;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void *
s3u_arraylist_get(s3u_arraylist_t *_al, int _index)
{
  assert(_al != NULL);
  assert(_index < _al->count);
  
  return _al->array[S3U_ARRAYLIST_INDEX(_al, _index)];
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
 
void *
s3u_arraylist_replace(s3u_arraylist_t *_al, int _index, void *_repl)
{
  void *rv;

  assert(_al != NULL);
  assert(_index < _al->count);

  rv = _al->array[S3U_ARRAYLIST_INDEX(_al, _index)];
  _al->array[S3U_ARRAYLIST_INDEX(_al, _index)] = _repl;
  
  return rv;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void *
s3u_arraylist_remove(s3u_arraylist_t *_al, int _pos)
{
  return NULL;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_insert(s3u_arraylist_t *_al, int _pos, void *_ptr)
{
  return;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_append(s3u_arraylist_t *_al, void *_ptr)
{
  assert(_al != NULL);

  if (_al->count == _al->max)
    s3u_arraylist_expand(_al, _al->count + 1);

  _al->array[S3U_ARRAYLIST_INDEX(_al, _al->count)] = _ptr;
  _al->count++;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void
s3u_arraylist_prepend(s3u_arraylist_t *_al, void *_ptr)
{
  assert(_al != NULL);

  if (_al->count == _al->max)
    s3u_arraylist_expand(_al, _al->count + 1);

  if (_al->head == 0)
    _al->head = _al->max - 1;
  else
    --_al->head;

  _al->array[_al->head] = _ptr;
  _al->count++;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void *
s3u_arraylist_pop(s3u_arraylist_t *_al)
{
  void *rv;

  assert(_al != NULL);
  assert(_al->count > 0);

  rv = _al->array[S3U_ARRAYLIST_INDEX(_al, _al->count - 1)];
  _al->array[S3U_ARRAYLIST_INDEX(_al, _al->count - 1)] = NULL;
  _al->count--;

  return rv;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void *
s3u_arraylist_dequeue(s3u_arraylist_t *_al)
{
  void *rv;

  assert(_al != NULL);
  assert(_al->count > 0);

  rv = _al->array[_al->head];
  _al->array[_al->head] = NULL;
  _al->head = (_al->head + 1) % _al->max;
  _al->count--;

  return rv;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

int
s3u_arraylist_count(s3u_arraylist_t *_al)
{
  return _al->count;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void **
s3u_arraylist_to_array(s3u_arraylist_t *_al)
{
  void **buffer = NULL;
  int i;

  if (_al->head + _al->count <= _al->max) {
    return &_al->array[_al->head];
  }
  else {
    buffer = (void **)ckd_calloc(sizeof(void *), _al->max);
    for (i = _al->count - 1; i >= 0; i--)
      buffer[i] = s3u_arraylist_get(_al, i);
    for (i = _al->count; i < _al->max; i++)
      buffer[i] = NULL;
    
    ckd_free(_al->array);
    _al->head = 0;
    _al->array = buffer;
    return buffer;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void
s3u_arraylist_expand(s3u_arraylist_t *_al, int _resize)
{
  int resize;

  assert(_al != NULL);
  
  if (_al->max < _resize) {
    resize = _al->max;
    while (resize < _resize)
      resize = resize * 2;

    s3u_arraylist_expand_to_size(_al, resize);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

static void
s3u_arraylist_expand_to_size(s3u_arraylist_t *_al, int _resize)
{
  void **buffer = NULL;
  int i;

  assert(_al != NULL);

  if (_resize <= _al->max)
    return;

  buffer = (void **)ckd_calloc(sizeof(void *), _resize);
  for (i = _al->count - 1; i >= 0; i--)
    buffer[i] = s3u_arraylist_get(_al, i);
  for (i = _al->count; i < _al->max; i++)
    buffer[i] = NULL;
    
  ckd_free(_al->array);
  _al->head = 0;
  _al->array = buffer;
  _al->max = _resize;
}

#undef S3U_ARRAYLIST_INDEX

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#if 0
int
s3_bin_search(int **_al, int _begin, int _end, int _target)
{
  assert(_begin >= 0);
  assert(_end >= 0);

  int mid;
  while (_begin < _end) {
    mid = (_begin + _end) / 2 + (_begin + _end) % 2;
    if (*_al[mid] < _target)
      _end = mid - 1;
    else
      _begin = mid;
  }

  return _begin;
}
#endif


