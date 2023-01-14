/* C8L Library - (c) 2022 Thadeu de Paula - Licensed under MIT Licence.

// Dynamic Heap Arrays
//
// This library provides functions and function like algoriths to
// create, manage and free dynamic arrays with C.
//
// It basically allocates memory for a 3 size_t array plus the amount
// requested in operations for any specified type.
//
// So if you want an array of 5 chars (a string!):
//
//             |
// | 0 | 1 | 2 | 0 | 1 | 2 | 3 | 4 |
// < meta data | space to store your
//   in size_t |    data (char *)
//             |
//  Hidden     | Exposed to your app
//
*/

#include <stdio.h>
#include <stdlib.h>
#ifndef C8L_ARR_INCLUDED
#define C8L_ARR_INCLUDED

/*
/$ c8l_arrnew(ref_type, items) : ptr | NULL
// - ref_type : the variable or type used as size reference as in sizeof()
// - items    : initial number of items allocated in the array
// - returns 1 or 0 as success.
//
// If you can vaguely estimate the minimal of items, it is good
// to use this value as it can avoid excessive internal callings to
// realloc.
*/
#define c8l_arrnew(t,l) \
	M_c8l_arrnew(sizeof(t),(size_t)(l))


/* For inner usage only. The return value can be changed, what is not intended
 * for who uses this library
 */

#define V_c8l_arrlen(a)  (*(((size_t *)(a)) - 3))
#define V_c8l_arrcap(a)  (*(((size_t *)(a)) - 2))
#define V_c8l_arrtsz(a)  (*(((size_t *)(a)) - 1))


/*
/$ c8l_arrlen(array) : size_t
//
// Get the length of array, i.e., the used items.
*/
#define c8l_arrlen(a) ((void)NULL,V_c8l_arrlen(a))


#define c8l_arrcap(a) ((void)NULL,V_c8l_arrcap(a))

/*
/$ c8l_arrcapsz(array,items) : int 1|0
//
// Check if there is room for new `items` on `array` or try to allocate
// the needed space. Returns 1 in success (if there is space or it could be
// allocated) or 0 on error.
//
// On case of error, the error can be retrieved with errno/strerror
*/
#define c8l_arrcapsz(a,c) \
	M_c8l_arrcapsz((void *)&(a),(size_t)(c))

/*
/$ c8l_arrpush(array,value) : int 1|0
//
// Adds `value` at the end of `array` returning 1 on success or 0 on false.
// It internally does the array capacity management.
//
// On case of error, the error can be retrieved with errno/strerror
*/
#define c8l_arrpush(a,v) ( \
	V_c8l_arrlen(a)+1 <= V_c8l_arrcap(a) || c8l_arrcapsz((a),1) \
		? ((a)[V_c8l_arrlen(a)++]=(v),1) : 0 \
)

/*
/$ c8l_arrpop(array,default)
//
// Pops the last item of `array`, reducing its length. If the length of array
// is already 0 returns the specified `default` value.
*/
#define c8l_arrpop(a,default) ( \
	V_c8l_arrlen(a) > 0 \
		? (a)[(--V_c8l_arrlen(a))] \
		: (default) \
)

/*
/$ c8l_arrfree(array)
//
// Call free to the raw array and sets the pointer to NULL.
// It only try to free if the pointer is not NULL, so have no risk to
// double free error.
*/
#define c8l_arrfree(a) (     \
	(a) == NULL                \
		? NULL                   \
		: (                      \
			free((size_t *)(a)-3), \
			((a)=NULL) ) )

/*
/$ c8l_arrclear(array)
// Reset the array length to 0. Further pushes and pops will consider the
// updated length. It keeps the allocation size for subsequent operations.
// It is useful when you wan't to reuse the array to fill with other values and
// don't want to engage on the allocation process again.
// Don't mistake it by the free operation. After you end to use the array
// you still need to call `c8l_arrfree'.
*/
#define c8l_arrclear(a) ( \
	(V_c8l_arrlen(a) = 0) )


/* GENERIC TYPE FUNCTIONS
 *
 * The functions and macros below are intended to:
 * - do the more complex part of the above macros
 * - make possible to deal with different types (polymorphism)
 */

static void *M_c8l_arrnew(size_t sz, size_t l) {
	size_t *a = realloc(NULL, (sz*l) + sizeof(size_t) * 3);

	if (a == NULL) return NULL;

	a[0]=0;
	a[1]=l;
	a[2]=sz;
	return (void *) (((size_t *)a)+3);
}

#define c8l_shct_arrrsz \
	((size_t *)(*a)) - 3,(sizeof(size_t) * 3) + (nc * V_c8l_arrtsz(*a))

static int M_c8l_arrcapsz (void **a, size_t mincap) {
	size_t *new;
	size_t nc = V_c8l_arrcap(*a);

	mincap += V_c8l_arrlen(*a);
	while( (nc *= 2) < mincap );

	if (NULL == (new = realloc(c8l_shct_arrrsz))) return 0;

	*a = (void *) (new+3);
	V_c8l_arrcap(*a) = nc;

	return 1;
}

#endif /* C8L_ARR_INCLUDED */

/* vim: set fdm=indent fdn=1 ts=2 sts=2 sw=2: */
