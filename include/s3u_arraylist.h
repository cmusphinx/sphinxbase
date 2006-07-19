
#ifndef _S3U_ARRAYLIST_H
#define _S3U_ARRAYLIST_H

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#define S3U_ARRAYLIST_DEFAULT_SIZE	16

/** \file s3u_arraylist.h
 * Defines a storage structure for generic, dynamic
 * sequences.  It also implements fast queue and stack operations.
 */

typedef struct {
  void **array;
  int head;
  int count;
  int max;
} s3u_arraylist_t;

/*----------------------------------
  | Initialization and destruction |
  ----------------------------------*/

/**
   Initializes the vector to a default size (1).  A vector must be initialized
   before it can be used.
  
   @param _arraylist The vector to be initialized.
 */
void
s3u_arraylist_init(s3u_arraylist_t *_arraylist);

/**
   Initializes the vector to a certain size.  A vector must be initialized
   before it can be used.
  
   @param _arraylist The vector to be initialized.
   @param _size The size to initialize the vector to.
 */
void
s3u_arraylist_init_size(s3u_arraylist_t *_arraylist, int _size);

/**
   Close the vector and free any internally allocated memory.  The vector
   structure itself is not freed.  The vector must be re-initialized before
   it can be used again.
  
   @param _arraylist The vector to be closed.
 */
void
s3u_arraylist_close(s3u_arraylist_t *_arraylist);

/**
   Clear the content of the vector and set the element count to 0.  The
   pointers (or data) contained in the vector are not freed.  The user is
   responsible for freeing them.  The array is immediately usable again.
  
   @param _arraylist The vector to be cleared.
 */
void
s3u_arraylist_clear(s3u_arraylist_t *_arraylist);

/*--------------------------
  | Array style operations |
  --------------------------*/

/** 
   Set the element at a particular index.  The element can be NULL.  The
   previous value is not saved nor freed.  The user is responsible for keeping
   track of the previous value if it is needed.
  
   This function automatically expands the size of the vector if needed.
   If the position is greater than the previous element count, the element
   count is updated.
  
   @param _arraylist The vector to be operated on.
   @param _pos The position to set the element.
   @param _ptr The pointer to the new element.
 */
void
s3u_arraylist_set(s3u_arraylist_t *_arraylist, int _pos, void *_ptr);

/**
   Get the element at a particular index.  Accessing out-of-bound indices will
   cause an error.
  
   @param _arraylist The vector to be operated on.
   @param _pos The position of the requested element.
   @return The pointer to the requested element.
 */
void *
s3u_arraylist_get(s3u_arraylist_t *_arraylist, int _pos);

/**
   Replace the element at a particular index (and return the previous value).
   Replacing out-of-bound indices will cause an error.

   @param _arraylist The vector to be operated on.
   @param _pos The position of the replacement.
   @param _ptr The pointer to the new element.
   @return The pointer of the previous value.
 */
void *
s3u_arraylist_replace(s3u_arraylist_t *_arraylist, int _pos, void *_ptr);

/*-------------------------
  | List style operations |
  -------------------------*/

/**
   Remove the element at a position and shift the remaining elements down.
   (NOT IMPLEMENTED)
   
   @param _arraylist The vector to be operated on.
   @param _pos The position of the element to be removed.
   @return The pointer of the removed element.
 */
void *
s3u_arraylist_remove(s3u_arraylist_t *_arraylist, int _pos);

/**
   Insert the element at a position and shift the remaining element up.  The
   size of the vector will expand if needed.
   (NOT IMPLEMENTED)

   @param _arraylist The vector to be operated on.
   @param _pos The position to insert the new element.
   @param _ptr The pointer to the new element.
 */
void
s3u_arraylist_insert(s3u_arraylist_t *_arraylist, int _pos, void *_ptr);

/**
   Append a new element to the end of the vector.  The size of the vector will
   expand if needed.

   @param _arraylist The vector to be operated on.
   @param _ptr The pointer to the new element.
 */
void
s3u_arraylist_append(s3u_arraylist_t *_arraylist, void *_ptr);

/**
   Prepend a new element to the head of the vector.  The size of the vector
   will expand if needed.

   @param _arraylist The vector to be operated on.
   @param _ptr The pointer to the new element.
 */
void
s3u_arraylist_prepend(s3u_arraylist_t *_arraylist, void *_ptr);

/**
   Remove the element at the end of the vector.

   @param _arraylist The vector to be operated on.
   @return The pointer to the removed element.
 */
void *
s3u_arraylist_pop(s3u_arraylist_t *_arraylist);

/**
   Remove the element at the front of the vector.

   @param _arraylist The vector to be operated on.
   @return The pointer to the removed element.
 */
void *
s3u_arraylist_dequeue(s3u_arraylist_t *_arraylist);

/*---------------------------
  | Miscellaneus operations |
  ---------------------------*/

/**
   Returns the element count.

   @param _arraylist The vector to count..
   @return The size of the vector.
 */
int
s3u_arraylist_count(s3u_arraylist_t *_arraylist);

/**
   Returns a <B>read-only</B> plain-old array of the elements in the vector.

   @param _arraylist The vector to be converted into array.
   @return The plain-old array.
   @return 0 for success, -1 for failure.
 */
void **
s3u_arraylist_to_array(s3u_arraylist_t *_arraylist);

#if 0
{ /* Fool indent. */
#endif
#ifdef __cplusplus
}
#endif
#endif
