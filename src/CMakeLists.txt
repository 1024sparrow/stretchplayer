######################################################################
### StretchPlayer Build Script (CMake)                             ###
######################################################################

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

######################################################################
### CHECK FOR SANE OPTIONS                                         ###
######################################################################

IF( NOT ( AUDIO_SUPPORT_JACK OR AUDIO_SUPPORT_ALSA ) )
  MESSAGE(FATAL_ERROR "Must enable support for at least ONE API")
ENDIF( NOT ( AUDIO_SUPPORT_JACK OR AUDIO_SUPPORT_ALSA ) )

######################################################################
### REQUIRED LIBRARIES                                             ###
######################################################################

IF( AUDIO_SUPPORT_JACK )
  FIND_PACKAGE(JACK REQUIRED)
  INCLUDE_DIRECTORIES(${JACK_INCLUDE_DIRS})
  SET(LIBS ${LIBS} ${JACK_LIBRARIES})
ENDIF( AUDIO_SUPPORT_JACK )

IF( AUDIO_SUPPORT_ALSA )
  FIND_PACKAGE(ALSA REQUIRED)
  INCLUDE_DIRECTORIES(${ALSA_INCLUDE_DIRS})
  SET(LIBS ${LIBS} ${ALSA_LIBRARIES})
ENDIF( AUDIO_SUPPORT_ALSA )

FIND_PACKAGE(LibSndfile REQUIRED)
INCLUDE_DIRECTORIES(${LibSndfile_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${LibSndfile_LIBRARIES})

FIND_PACKAGE(LibMpg123 REQUIRED)
INCLUDE_DIRECTORIES(${LibMpg123_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${LibMpg123_LIBRARIES})

FIND_PACKAGE(RubberBand REQUIRED)
INCLUDE_DIRECTORIES(${RubberBand_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${RubberBand_LIBRARIES})

FIND_PACKAGE(Threads)
SET(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

######################################################################
### LIBRARY SOURCES AND BUILD                                      ###
######################################################################

LIST(APPEND sp_cpp
  main.cpp
  Configuration.cpp
  Engine.cpp
  AudioSystem.cpp
  jack_memops.c
  bams_format.c
  RubberBandServer.cpp
  )

LIST(APPEND sp_hpp
  Configuration.hpp
  Engine.hpp
  AudioSystem.hpp
  jack_memops.h
  bams_format.h
  RubberBandServer.hpp
  RingBuffer.hpp
  )

# Add files for audio API's:

IF( JACK_FOUND )
  LIST(APPEND sp_cpp JackAudioSystem.cpp)
  LIST(APPEND sp_hpp JackAudioSystem.hpp)
ENDIF( JACK_FOUND )

IF( ALSA_FOUND )
  LIST(APPEND sp_cpp AlsaAudioSystem.cpp)
  LIST(APPEND sp_hpp AlsaAudioSystem.hpp)
ENDIF( ALSA_FOUND )

LIST(APPEND sp_cpp FakeAudioSystem.cpp)
LIST(APPEND sp_cpp FakeAudioSystem.hpp)

INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_BINARY_DIR}
  )

ADD_EXECUTABLE(stretchplayer-cli
  ${sp_cpp}
  ${sp_hpp}
  )

TARGET_LINK_LIBRARIES(stretchplayer-cli
    ${LIBS}
    )

INSTALL(TARGETS stretchplayer-cli RUNTIME DESTINATION bin)

######################################################################
### CONFIGURATION SUMMARY                                          ###
######################################################################

MESSAGE("\n"
"Configuration Summary for StretchPlayer-cli\n"
"---------------------------------------\n"
)

MACRO(lib_report name)
  IF(${name}_FOUND)
    message("ENABLED..... ${name}")
  ELSE(${name}_FOUND)
    message("disabled.... ${name}")
  ENDIF(${name}_FOUND)
ENDMACRO(lib_report)

lib_report(JACK)
lib_report(ALSA)
lib_report(LibSndfile)
lib_report(LibMpg123)
lib_report(RubberBand)
