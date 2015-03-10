find_package( XRootD REQUIRED )
find_package( LibRadosFs REQUIRED )
find_package( Boost REQUIRED COMPONENTS thread )

add_library( RadosOss SHARED
             RadosOss.cc RadosOss.hh
             RadosOssFile.cc RadosOssFile.hh
             RadosOssDir.cc RadosOssDir.hh
             RadosOssDefines.hh
)

include_directories( ${Boost_INCLUDE_DIR} ${XROOTD_INCLUDE_DIR} ${RADOS_FS_INCLUDE_DIR} )

add_definitions( -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 )

target_link_libraries( RadosOss ${RADOS_FS_LIB} )

if( Linux )
  set_target_properties( RadosOss PROPERTIES
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    SOVERSION ${VERSION_MAJOR}
    CLEAN_DIRECT_OUTPUT 1
  )
endif( Linux )

install( TARGETS RadosOss LIBRARY DESTINATION ${LIB_INSTALL_DIR} )
