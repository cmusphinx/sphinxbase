SET(SOURCES	fe_interface.c
			fe_internal.h
			fe_noise.c
			fe_noise.h
			fe_prespch_buf.c
			fe_prespch_buf.h
			fe_sigproc.c
			fe_type.h
			fe_warp.c
			fe_warp.h
			fe_warp_affine.c
			fe_warp_affine.h
			fe_warp_inverse_linear.c
			fe_warp_inverse_linear.h
			fe_warp_piecewise_linear.c
			fe_warp_piecewise_linear.h
			fixlog.c
			yin.c
	)
	
add_library(fe OBJECT ${SOURCES})

set_property(TARGET fe PROPERTY POSITION_INDEPENDENT_CODE TRUE)