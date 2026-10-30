corrosion_import_crate(MANIFEST_PATH ./Cargo.toml)
#corrosion_link_libraries(CInterfaceOxTTL CInterfaceOxTTL_CHelperlib)
target_sources(CInterfaceClipsSerializer INTERFACE
        FILE_SET HEADERS
        BASE_DIRS include
	FILES
	include/CInterfaceClipsSerializer.h
)

#corrosion_install(TARGETS CInterfaceOxTTL EXPORT CInterfaceOxTTLtargets)
