find_program(MSGMERGE msgmerge REQUIRED)
find_program(MSGFMT msgfmt REQUIRED)

set(POT_FILENAME ${CMAKE_CURRENT_SOURCE_DIR}/abaddon.pot)
set(MO_FILENAME abaddon.mo)
set(LOCALE_DIRECTORY ${PROJECT_BINARY_DIR}/locale)

list(APPEND PO_NAMES pt_BR)

foreach(PO_NAME IN LISTS PO_NAMES)
	list(APPEND PO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${PO_NAME}.po)
endforeach()

# Updates all the translation
foreach(PO_SOURCE PO_NAME IN ZIP_LISTS PO_SOURCES PO_NAMES)
    add_custom_target("update-${PO_NAME}" DEPENDS ${PO_SOURCE})
    add_custom_command(
        TARGET "update-${PO_NAME}" PRE_BUILD
        COMMAND ${MSGMERGE} -U -N ${PO_SOURCE} ${POT_FILENAME}
        DEPENDS ${POT_FILENAME}
    )
    add_dependencies(abaddon "update-${PO_NAME}")
endforeach()

# Compiles all the translation files onto locales directory
foreach(PO_SOURCE PO_NAME IN ZIP_LISTS PO_SOURCES PO_NAMES)
    file(MAKE_DIRECTORY ${LOCALE_DIRECTORY}/${PO_NAME}/LC_MESSAGES/)
    add_custom_command(
        OUTPUT ${LOCALE_DIRECTORY}/${PO_NAME}/LC_MESSAGES/${MO_FILENAME}
        COMMAND ${MSGFMT} --output-file=${LOCALE_DIRECTORY}/${PO_NAME}/LC_MESSAGES/${MO_FILENAME}
                      ${PO_SOURCE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${PO_SOURCE}
    )
    add_custom_target("compile-${PO_NAME}" DEPENDS ${LOCALE_DIRECTORY}/${PO_NAME}/LC_MESSAGES/${MO_FILENAME})
    add_dependencies(abaddon "compile-${PO_NAME}")
endforeach()