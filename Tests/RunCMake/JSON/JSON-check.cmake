# This file does a regex file compare on the generated
# Android.mk files from the AndroidMK test

macro(compare_file_to_expected file expected_file)
  file(READ "${file}" JSON)
  # clean up new lines
  string(REGEX REPLACE "\r\n" "\n" JSON "${JSON}")
  string(REGEX REPLACE "\n+$" "" JSON "${JSON}")
  # read in the expected regex file
  file(READ "${expected_file}" expected)
  # clean up new lines
  string(REGEX REPLACE "\r\n" "\n" expected "${expected}")
  string(REGEX REPLACE "\n+$" "" expected "${expected}")
  # compare the file to the expected regex and if there is not a match
  # put an error message in RunCMake_TEST_FAILED
  if(NOT "${JSON}" MATCHES "${expected}")
    set(RunCMake_TEST_FAILED
      "${file} does not match ${expected_file}:

JSON contents = [\n${JSON}\n]
Expected = [\n${expected}\n]")
  endif()
endmacro()

compare_file_to_expected(
  "${RunCMake_BINARY_DIR}/JSON-build/myexp.json"
  "${RunCMake_TEST_SOURCE_DIR}/expectedBuildJSON.txt")
compare_file_to_expected(
  "${RunCMake_BINARY_DIR}/JSON-build/CMakeFiles/Export/share/myexp.json"
  "${RunCMake_TEST_SOURCE_DIR}/expectedInstallJSON.txt")
