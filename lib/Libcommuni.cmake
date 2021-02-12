project(LibCommuni)

set(LibCommuni_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/libcommuni/include/")
set(LibCommuni_SOURCE_DIR "${CMAKE_SOURCE_DIR}/lib/libcommuni/src/")

# ------------------------------------------------------------------------------------------------#
#  Core
# ------------------------------------------------------------------------------------------------#

set(CORE_PUB_HEADERS
        ${LibCommuni_INCLUDE_DIR}/IrcCore/irc.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/irccommand.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircconnection.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/irccore.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircfilter.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircglobal.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircmessage.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircnetwork.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircprotocol.h
        )

set(CORE_PRIV_HEADERS
        ${LibCommuni_INCLUDE_DIR}/IrcCore/irccommand_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircconnection_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircdebug_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircmessage_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircmessagecomposer_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircmessagedecoder_p.h
        ${LibCommuni_INCLUDE_DIR}/IrcCore/ircnetwork_p.h
        )

set(CORE_SOURCE_FILES
        ${LibCommuni_SOURCE_DIR}/core/irc.cpp
        ${LibCommuni_SOURCE_DIR}/core/irccommand.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircconnection.cpp
        ${LibCommuni_SOURCE_DIR}/core/irccore.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircfilter.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircmessage.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircmessage_p.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircmessagecomposer.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircmessagedecoder.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircnetwork.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircprotocol.cpp
        ${LibCommuni_SOURCE_DIR}/core/ircmessagedecoder_none.cpp
        ${LibCommuni_SOURCE_DIR}/3rdparty/mozilla/rdf_utils.c
        )


add_library(Communi_Core ${CORE_PRIV_HEADERS} ${CORE_PUB_HEADERS} ${CORE_SOURCE_FILES})
target_link_libraries(Communi_Core PRIVATE Qt5::Core Qt5::Network)
target_include_directories(Communi_Core PUBLIC "${LibCommuni_INCLUDE_DIR}/IrcCore/")
target_compile_definitions(Communi_Core PUBLIC IRC_STATIC IRC_NAMESPACE=Communi)
set_property(TARGET Communi_Core PROPERTY AUTOMOC ON)
set_property(SOURCE ${CORE_SOURCE_FILES} PROPERTY SKIP_AUTOMOC ON)

# ------------------------------------------------------------------------------------------------#
#  Model
# ------------------------------------------------------------------------------------------------#
file(GLOB_RECURSE MODEL_SOURCE_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${CMAKE_SOURCE_DIR}/lib/libcommuni/src/core/*.cpp"
        "${CMAKE_SOURCE_DIR}/lib/libcommuni/include/IrcModel/*.h"
        )

add_library(Communi_Model ${MODEL_SOURCE_FILES})
target_include_directories(Communi_Model PUBLIC "${LibCommuni_INCLUDE_DIR}/IrcModel/")
target_compile_definitions(Communi_Model PUBLIC IRC_STATIC IRC_NAMESPACE=Communi)
#set_property(TARGET Communi_Model PROPERTY AUTOMOC ON)

# ------------------------------------------------------------------------------------------------#
#  Util
# ------------------------------------------------------------------------------------------------#
file(GLOB_RECURSE UTIL_SOURCE_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${CMAKE_SOURCE_DIR}/lib/libcommuni/src/core/*.cpp"
        "${CMAKE_SOURCE_DIR}/lib/libcommuni/include/IrcUtil/*.h"
        )

add_library(Communi_Util ${UTIL_SOURCE_FILES})
target_include_directories(Communi_Util PUBLIC "${LibCommuni_INCLUDE_DIR}/IrcUtil/")
target_compile_definitions(Communi_Util PUBLIC IRC_STATIC IRC_NAMESPACE=Communi)
#set_property(TARGET Communi_Util PROPERTY AUTOMOC ON)

set(LibCommuni_INCLUDE_DIRS ${LibCommuni_INCLUDE_DIR}/IrcCore/ ${LibCommuni_INCLUDE_DIR}/IrcModel/ ${LibCommuni_INCLUDE_DIR}/IrcUtil/)
set(LibCommuni_LIBRARIES ${Communi_Core} ${Communi_Model} ${Communi_Util})

add_library(LibCommuni::LibCommuni INTERFACE IMPORTED)
set_target_properties(LibCommuni::LibCommuni PROPERTIES
        INTERFACE_LINK_LIBRARIES "${LibCommuni_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibCommuni_INCLUDE_DIRS}"
        )



