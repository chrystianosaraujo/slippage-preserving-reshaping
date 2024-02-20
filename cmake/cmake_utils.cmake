# Returns whether the given DIR is empty or not
FUNCTION(IS_DIR_EMPTY DIR RET)
  file(GLOB RESULT "${DIR}/*")
  list(LENGTH RESULT RES_LEN)

  if(RES_LEN EQUAL 0)
    SET(${RET} TRUE PARENT_SCOPE)
  else()
    SET(${RET} FALSE PARENT_SCOPE)
  endif()
ENDFUNCTION()

# Runs external batch file
FUNCTION(RUN_BATCH_FILE BATCH_FN CLOSE_TERMINAL_AFTER)
  set(EXIT_ENABLED 0)
  if(${CLOSE_TERMINAL_AFTER})
    set(EXIT_ENABLED 1)
  endif()

  execute_process(COMMAND cmd /c "start ${BATCH_FN} ${EXIT_ENABLED} ${RET}")
ENDFUNCTION()

FUNCTION(ORGANIZE_SOURCE_STRUCTURE DIR)
  file(GLOB children RELATIVE ${DIR} ${DIR}/*)

  foreach(itr ${children})
    if(IS_DIRECTORY ${DIR}/${itr})
      ORGANIZE_SOURCE_STRUCTURE(${DIR}/${itr})
    else()
      string(REPLACE "/" "\\" groupname ${DIR})
      source_group(${groupname} FILES
                   ${DIR}/${itr})
    endif()
  endforeach()
ENDFUNCTION()
