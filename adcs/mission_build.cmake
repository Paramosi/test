###########################################################
#
# ADCS platform build setup
#
# This file is evaluated as part of the "prepare" stage
# and can be used to set up prerequisites for the build,
# such as generating header files
#
###########################################################

# The list of header files that control the ADCS configuration
set(ADCS_APP_PLATFORM_CONFIG_FILE_LIST
  adcs_fcncodes.h
  adcs_interface_cfg.h
  adcs_mission_cfg.h
  adcs_perfids.h
  adcs_msg.h
  adcs_msgdefs.h
  adcs_msgstruct.h
  adcs_tbl.h
  adcs_tbldefs.h
  adcs_tblstruct.h
  adcs_topicids.h
)

# Create wrappers around the all the config header files
# This makes them individually overridable by the missions, without modifying
# the distribution default copies
foreach(ADCS_APP_CFGFILE ${ADCS_APP_PLATFORM_CONFIG_FILE_LIST})
  get_filename_component(CFGKEY "${ADCS_APP_CFGFILE}" NAME_WE)
  if (DEFINED ADCS_APP_CFGFILE_SRC_${CFGKEY})
    set(DEFAULT_SOURCE GENERATED_FILE "${ADCS_APP_CFGFILE_SRC_${CFGKEY}}")
  else()
    set(DEFAULT_SOURCE FALLBACK_FILE "${CMAKE_CURRENT_LIST_DIR}/config/default_${ADCS_APP_CFGFILE}")
  endif()
  generate_config_includefile(
    FILE_NAME           "${ADCS_APP_CFGFILE}"
    ${DEFAULT_SOURCE}
  )
endforeach()
