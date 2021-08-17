add_subdirectory("include")

set(C_SUBLIBS "knetfile;bgzf;kalloc;keigen;kexpr;khmm;kmath;knhx;kson;kstring;ksw;kthread;kurl;kopen;ksa")  # order matters!

foreach(C_SUBLIB ${C_SUBLIBS})
	add_subdirectory("${C_SUBLIB}")
endforeach()

pass_through_cpack_vars()
