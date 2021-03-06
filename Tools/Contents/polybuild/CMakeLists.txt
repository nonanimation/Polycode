INCLUDE(PolycodeIncludes)

FIND_PACKAGE(ZLIB)
INCLUDE_DIRECTORIES(
    ${ZLIB_INCLUDE_DIR}
    ${Polycode_SOURCE_DIR}/Tools/Dependencies/unzip11 
    Include)

SET(minizip_SRCS 
    ../../Dependencies/unzip11/ioapi.c
    ../../Dependencies/unzip11/ioapi.h
    ../../Dependencies/unzip11/zip.c
    ../../Dependencies/unzip11/zip.h
)

#IF(POLYCODE_BUILD_SHARED)
#    LINK_LIBRARIES(Polycore)
#    ADD_EXECUTABLE(polybuild ${minizip_SRCS} Source/polybuild.cpp Include/polybuild.h)
#ENDIF(POLYCODE_BUILD_SHARED)

#IF(POLYCODE_BUILD_STATIC)
ADD_EXECUTABLE(polybuild ${minizip_SRCS} Source/polybuild.cpp Include/polybuild.h)
IF(MSVC OR MINGW)
    TARGET_LINK_LIBRARIES(polybuild Polycore ${LUA_LIBRARIES} ${PHYSFS_LIBRARY} ${ZLIB_LIBRARIES})
ELSEIF(APPLE)
    TARGET_LINK_LIBRARIES(polybuild Polycore ${LUA_LIBRARIES} ${PHYSFS_LIBRARY} ${ZLIB_LIBRARIES} "-framework IOKit" "-framework Cocoa")
    SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pagezero_size 10000 -image_base 100000000" ) 
ELSE(MSVC OR MINGW)
    TARGET_LINK_LIBRARIES(polybuild Polycore ${LUA_LIBRARIES} ${PHYSFS_LIBRARY} ${ZLIB_LIBRARIES} dl)
ENDIF(MSVC OR MINGW)
#ENDIF(POLYCODE_BUILD_STATIC)

IF(POLYCODE_INSTALL_FRAMEWORK)

    # install exes
#    IF(POLYCODE_BUILD_SHARED)
#        INSTALL(TARGETS polybuild_dynamic DESTINATION ${POLYCODE_RELEASE_DIR}/Framework/Tools)
#    ENDIF(POLYCODE_BUILD_SHARED)
    
#    IF(POLYCODE_BUILD_STATIC)
        INSTALL(TARGETS polybuild DESTINATION Tools)
#    ENDIF(POLYCODE_BUILD_STATIC)

ENDIF(POLYCODE_INSTALL_FRAMEWORK)
