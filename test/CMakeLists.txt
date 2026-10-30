include(CTest)
add_executable(CInterfaceClipsSerializer_test_a
	test.c
)

target_link_libraries(CInterfaceClipsSerializer_test_a
	CInterfaceClipsSerializer
	NQuadsRDF
)

set(basename "CInterfaceClipsSerializer::")
set(testname "${basename}testA")
add_test(NAME ${testname}
	COMMAND CInterfaceClipsSerializer_test_a
	ARGS
	"--premise" "${CMAKE_CURRENT_SOURCE_DIR}/testinput.nq"
	"--conclusion" "${CMAKE_CURRENT_SOURCE_DIR}/testoutput.regex"
)
set_property(TEST "${testname}" PROPERTY LABELS "CInterfaceClipsSerializer")
