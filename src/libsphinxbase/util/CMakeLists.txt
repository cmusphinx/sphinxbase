SET(SOURCES	bio.c
		bitarr.c
		bitvec.c
		blas_lite.c
		case.c
		ckd_alloc.c
		cmd_ln.c
		dtoa.c
		err.c
		errno.c
		f2c_lite.c
		filename.c
		genrand.c
		glist.c
		hash_table.c
		heap.c
		listelem_alloc.c
		logmath.c
		matrix.c
		mmio.c
		pio.c
		priority_queue.c
		profile.c
		sbthread.c
		slamch.c
		slapack_lite.c
		strfuncs.c
	)
	
add_library(util OBJECT ${SOURCES})