#!/usr/bin/env luajit

--------------------------------------------------
--             ASSERTION CONSTANTS              --
--------------------------------------------------

local ASSERTION_IDENTIFIER_PREFIX = "#ASSERT_"

local EXIT_CODE_ASSERTION_IDENTIFIER = ASSERTION_IDENTIFIER_PREFIX .. "EXIT_CODE"

local STDOUT_ASSERTION_IDENTIFIER = ASSERTION_IDENTIFIER_PREFIX .. "STDOUT"
local STDOUT_LINE_ASSERTION_IDENTIFIER = ASSERTION_IDENTIFIER_PREFIX .. "STDOUT_LINE"

local STDERR_ASSERTION_IDENTIFIER = ASSERTION_IDENTIFIER_PREFIX .. "STDERR"
local STDERR_LINE_ASSERTION_IDENTIFIER = ASSERTION_IDENTIFIER_PREFIX .. "STDERR_LINE"

local ASSERTION_ARG_DELIMITER = " "

--------------------------------------------------
--                  UTILITIES                   --
--------------------------------------------------
local e2e_test_filepath -- forward declaration

-- @desc log error consisting of `error_type` and `message` to stderr and terminate execution
local function log_error_and_terminate_execution(error_type, message)
  assert(
    type(error_type) == "string" and type(message) == "string",
    "Expected string 'error_type' and 'message' arguments"
  )

  io.stderr:write("[" .. error_type .. "] - " .. message .. "\n")
  os.exit(1)
end

-- @desc log Input/Output error `message` to stderr and terminate execution
local function io_error(message)
  assert(type(message) == "string", "Expected string 'message' argument")

  log_error_and_terminate_execution("IO_ERROR", message)
end

-- @desc log invalid argument error `message` to stderr and terminate execution
local function invalid_arg_error(message)
  assert(type(message) == "string", "Expected string 'message' argument")

  log_error_and_terminate_execution("INVALID_ARG_ERROR", message)
end

-- @desc log syntax error `message` with `line`/`column` position and terminate execution
local function syntax_error(message, line, column)
  assert(
    type(message) == "string" and type(line) == "number" and type(column) == "number",
    "Expected string 'message', number 'line', and number 'column' arguments"
  )

  log_error_and_terminate_execution(
    "ASSERTION_SYNTAX_ERROR",
    e2e_test_filepath .. ":" .. line .. ":" .. column .. " - " .. message
  )
end

-- @desc log assertion failure `message` with optional `line`/`column` position and terminate execution
local function assertion_failure(message, line, column)
  assert(type(message) == "string", "Expected string 'message' argument")
  if line ~= nil or column ~= nil then
    assert(
      type(line) == "number" and type(column) == "number",
      "Expected both 'line' and 'column' to be number arguments when either is present"
    )
  end

  message = " - " .. message
  if line then
    message = ":" .. line .. ":" .. column .. message
  end
  message = e2e_test_filepath .. message

  log_error_and_terminate_execution("ASSERTION_FAILURE", message)
end

-- @desc decode string `assertion_arg` escape sequences
-- @return string with `assertion_arg` escape sequences decoded (enabled)
local function decode_assertion_arg_escape_sequences(assertion_arg)
  assert(type(assertion_arg) == "string", "Expected string 'assertion_arg' argument")

  return assertion_arg:gsub("\\\\", "\\"):gsub("\\n", "\n"):gsub("\\t", "\t")
end

--------------------------------------------------
--        PROCESS COMMAND-LINE ARGUMENTS        --
--------------------------------------------------

e2e_test_filepath = arg[1] or invalid_arg_error("Missing e2e_test_filepath argument")
local e2e_test_stdout = arg[2] or invalid_arg_error("Missing e2e_test_stdout argument")
local e2e_test_stderr = arg[3] or invalid_arg_error("Missing e2e_test_stderr argument")
local e2e_test_exit_code = arg[4] or invalid_arg_error("Missing e2e_test_exit_code argument")

---@diagnostic disable-next-line: cast-local-type
e2e_test_exit_code = tonumber(e2e_test_exit_code)
  or invalid_arg_error("Invalid e2e_test_exit_code argument '" .. e2e_test_exit_code .. "' (failed numeric conversion)")
if e2e_test_exit_code < 0 or e2e_test_exit_code > 255 then
  invalid_arg_error("Out-of-bounds e2e_test_exit_code argument '" .. e2e_test_exit_code .. "' (valid range: 0-255)")
end

local e2e_test_filehandle, io_open_error = io.open(e2e_test_filepath, "rb")
if not e2e_test_filehandle then
  io_error("Failed to open e2e_test_filepath argument - " .. io_open_error)
end

--------------------------------------------------
--           RUN E2E TEST ASSERTIONS            --
--------------------------------------------------

do -- execute e2e_test_filepath assertions against e2e_test_filepath output
  local expected_e2e_test_exit_code = 0

  local e2e_test_stdout_slice_start_index = 1
  local e2e_test_stderr_slice_start_index = 1

  local is_exit_code_assertion_detected = false

  local e2e_testfile_line_number = 0
  for e2e_testfile_line in assert(e2e_test_filehandle):lines() do
    e2e_testfile_line_number = e2e_testfile_line_number + 1

    local assertion_identifier_start_index, assertion_identifier_end_index, assertion_identifier =
      string.find(e2e_testfile_line, "(" .. ASSERTION_IDENTIFIER_PREFIX .. "[%w_]*)")
    if not assertion_identifier then
      goto continue
    end

    local assertion_arg_delimiter_start_index = assertion_identifier_end_index + 1
    local assertion_arg_delimiter_end_index = assertion_arg_delimiter_start_index
    local assertion_arg_delimiter =
      string.sub(e2e_testfile_line, assertion_arg_delimiter_start_index, assertion_arg_delimiter_end_index)
    if not assertion_arg_delimiter then
      syntax_error(
        "Assertion '" .. assertion_identifier .. "' is missing '" .. ASSERTION_ARG_DELIMITER .. "' argument delimiter",
        e2e_testfile_line_number,
        assertion_identifier_end_index
      )
    elseif assertion_arg_delimiter ~= ASSERTION_ARG_DELIMITER then
      syntax_error(
        "Invalid argument delimiter '" .. assertion_arg_delimiter .. "' (expected '" .. ASSERTION_ARG_DELIMITER .. "')",
        e2e_testfile_line_number,
        assertion_arg_delimiter_start_index
      )
    end

    local assertion_arg_start_index = assertion_arg_delimiter_end_index + 1
    local assertion_arg = string.sub(e2e_testfile_line, assertion_arg_start_index)
      or syntax_error(
        "Assertion '" .. assertion_identifier .. "' is missing argument",
        e2e_testfile_line_number,
        assertion_arg_delimiter_end_index
      )

    if assertion_identifier == EXIT_CODE_ASSERTION_IDENTIFIER then
      if is_exit_code_assertion_detected == true then
        syntax_error(
          "Second '" .. assertion_identifier .. "' assertion detected (only one allowed)",
          e2e_testfile_line_number,
          assertion_identifier_start_index
        )
      end
      is_exit_code_assertion_detected = true

      local _ = string.match(assertion_arg, "^%d+$")
        or syntax_error(
          "Invalid '" .. assertion_identifier .. "' argument '" .. assertion_arg .. "' (expected signless integer)",
          e2e_testfile_line_number,
          assertion_arg_start_index
        )

      local exit_code_assertion_arg = assert(tonumber(assertion_arg))
      if exit_code_assertion_arg < 0 or exit_code_assertion_arg > 255 then
        syntax_error(
          "Out-of-bounds '" .. assertion_identifier .. "' argument '" .. assertion_arg .. "' (valid range: 0-255)",
          e2e_testfile_line_number,
          assertion_arg_start_index
        )
      end

      expected_e2e_test_exit_code = exit_code_assertion_arg
    elseif
      assertion_identifier == STDOUT_ASSERTION_IDENTIFIER or assertion_identifier == STDOUT_LINE_ASSERTION_IDENTIFIER
    then
      local stdout_assertion_arg = decode_assertion_arg_escape_sequences(assertion_arg)
      if assertion_identifier == STDOUT_LINE_ASSERTION_IDENTIFIER then
        stdout_assertion_arg = stdout_assertion_arg .. "\n"
      end

      local e2e_test_stdout_slice_end_index = e2e_test_stdout_slice_start_index + stdout_assertion_arg:len() - 1
      local e2e_test_stdout_slice =
        e2e_test_stdout:sub(e2e_test_stdout_slice_start_index, e2e_test_stdout_slice_end_index)
      e2e_test_stdout_slice_start_index = e2e_test_stdout_slice_end_index + 1

      if e2e_test_stdout_slice ~= stdout_assertion_arg then
        assertion_failure(
          "Expected stdout '" .. e2e_test_stdout_slice .. "' to equal '" .. stdout_assertion_arg .. "'",
          e2e_testfile_line_number,
          assertion_identifier_start_index
        )
      end
    elseif
      assertion_identifier == STDERR_ASSERTION_IDENTIFIER or assertion_identifier == STDERR_LINE_ASSERTION_IDENTIFIER
    then
      local stderr_assertion_arg = decode_assertion_arg_escape_sequences(assertion_arg)
      if assertion_identifier == STDERR_LINE_ASSERTION_IDENTIFIER then
        stderr_assertion_arg = stderr_assertion_arg .. "\n"
      end

      local e2e_test_stderr_slice_end_index = e2e_test_stderr_slice_start_index + stderr_assertion_arg:len() - 1
      local e2e_test_stderr_slice =
        e2e_test_stderr:sub(e2e_test_stderr_slice_start_index, e2e_test_stderr_slice_end_index)
      e2e_test_stderr_slice_start_index = e2e_test_stderr_slice_end_index + 1

      if e2e_test_stderr_slice ~= stderr_assertion_arg then
        assertion_failure(
          "Expected stderr '" .. e2e_test_stderr_slice .. "' to equal '" .. stderr_assertion_arg .. "'",
          e2e_testfile_line_number,
          assertion_identifier_start_index
        )
      end
    else
      syntax_error(
        "Invalid '" .. assertion_identifier .. "' assertion identifier",
        e2e_testfile_line_number,
        assertion_identifier_start_index
      )
    end

    ::continue::
  end

  local e2e_test_stdout_slice = e2e_test_stdout:sub(e2e_test_stdout_slice_start_index)
  if e2e_test_stdout_slice:len() > 0 then
    assertion_failure("Unexpected stdout '" .. e2e_test_stdout_slice .. "'")
  end

  local e2e_test_stderr_slice = e2e_test_stderr:sub(e2e_test_stderr_slice_start_index)
  if e2e_test_stderr_slice:len() > 0 then
    assertion_failure("Unexpected stderr '" .. e2e_test_stderr_slice .. "'")
  end

  if e2e_test_exit_code ~= expected_e2e_test_exit_code then
    assertion_failure(
      "Expected exit code '" .. e2e_test_exit_code .. "' to equal '" .. expected_e2e_test_exit_code .. "'"
    )
  end
end

--------------------------------------------------
--                   CLEAN UP                   --
--------------------------------------------------

local io_close_success, io_close_error = assert(e2e_test_filehandle):close()
if not io_close_success then
  io_error("Failed to close '" .. e2e_test_filepath .. "' - " .. io_close_error)
end
