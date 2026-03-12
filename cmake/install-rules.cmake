if(PROJECT_IS_TOP_LEVEL)
  set(
      CMAKE_INSTALL_INCLUDEDIR "include/infix-parser-${PROJECT_VERSION}"
      CACHE STRING ""
  )
  set_property(CACHE CMAKE_INSTALL_INCLUDEDIR PROPERTY TYPE PATH)
endif()

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# find_package(<package>) call for consumers to find this project
# should match the name of variable set in the install-config.cmake script
set(package infix-parser)

install(
    DIRECTORY
    include/
    "${PROJECT_BINARY_DIR}/export/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    COMPONENT infix-parser_Development
)

install(
    TARGETS infix-parser_infix-parser
    EXPORT infix-parserTargets
    RUNTIME #
    COMPONENT infix-parser_Runtime
    LIBRARY #
    COMPONENT infix-parser_Runtime
    NAMELINK_COMPONENT infix-parser_Development
    ARCHIVE #
    COMPONENT infix-parser_Development
    INCLUDES #
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

write_basic_package_version_file(
    "${package}ConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)

# Allow package maintainers to freely override the path for the configs
set(
    infix-parser_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/${package}"
    CACHE STRING "CMake package config location relative to the install prefix"
)
set_property(CACHE infix-parser_INSTALL_CMAKEDIR PROPERTY TYPE PATH)
mark_as_advanced(infix-parser_INSTALL_CMAKEDIR)

install(
    FILES cmake/install-config.cmake
    DESTINATION "${infix-parser_INSTALL_CMAKEDIR}"
    RENAME "${package}Config.cmake"
    COMPONENT infix-parser_Development
)

install(
    FILES "${PROJECT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${infix-parser_INSTALL_CMAKEDIR}"
    COMPONENT infix-parser_Development
)

install(
    EXPORT infix-parserTargets
    NAMESPACE infix-parser::
    DESTINATION "${infix-parser_INSTALL_CMAKEDIR}"
    COMPONENT infix-parser_Development
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
