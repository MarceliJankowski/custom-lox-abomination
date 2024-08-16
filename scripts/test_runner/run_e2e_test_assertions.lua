#!/usr/bin/env luajit

--------------------------------------------------
--                  UTILITIES                   --
--------------------------------------------------
local e2e_test_filepath -- forward declaration

-- @desc log invalid argument error `message` to stderr and terminate execution
local function invalid_arg_error(message)
  assert(type(message) == "string", "Expected string 'message' argument")

  io.stderr:write("[INVALID_ARG_ERROR] - " .. message .. "\n")
  os.exit(1)
end

-- @desc log syntax error `message` with `line`/`column` position and terminate execution
local function syntax_error(message, line, column)
  assert(
    type(message) == "string" and type(line) == "number" and type(column) == "number",
    "Expected string 'message', number 'line', and number 'column' arguments"
  )

  io.stderr:write(
    "[ASSERT_INSTRUCTION_SYNTAX_ERROR] - "
      .. e2e_test_filepath
      .. ":"
      .. line
      .. ":"
      .. column
      .. " - "
      .. message
      .. "\n"
  )
  os.exit(1)
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

  io.stderr:write("[ASSERTION_FAILURE] - " .. e2e_test_filepath)
  if line then
    io.stderr:write(":" .. line .. ":" .. column)
  end
  io.stderr:write(" - " .. message .. "\n")

  os.exit(1)
end

-- @desc decode `assertion_argument` escape sequences
-- @return string with decoded (enabled) escape sequences
local function decode_assertion_argument_escape_sequences(assertion_argument)
  assert(type(assertion_argument) == "string", "Expected number 'input_string' argument")

  return assertion_argument:gsub("\\\\", "\\"):gsub("\\n", "\n"):gsub("\\t", "\t")
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
  or invalid_arg_error("Invalid e2e_test_exit_code argument '" .. e2e_test_exit_code .. "' (not a number)")
if e2e_test_exit_code < 0 or e2e_test_exit_code > 255 then
  invalid_arg_error("Out-of-bounds e2e_test_exit_code argument '" .. e2e_test_exit_code .. "' (valid range: 0-255)")
end

local e2e_test_filehandle = io.open(e2e_test_filepath, "r")
if not e2e_test_filehandle then
  invalid_arg_error("Failed to open e2e_test_filepath argument '" .. e2e_test_filepath .. "'")
end

--------------------------------------------------
--           RUN E2E TEST ASSERTIONS            --
--------------------------------------------------

local expected_exit_code = 0 -- default value

local assert_instruction_common_prefix = "#ASSERT_"
local assert_exit_code_instruction = assert_instruction_common_prefix .. "EXIT_CODE"
local assert_stderr_instruction = assert_instruction_common_prefix .. "STDERR"
local assert_stdout_instruction = assert_instruction_common_prefix .. "STDOUT"
local assert_instruction_argument_delimiter = " "

do -- execute e2e_test_filepath assertions against test output
  local e2e_testfile_line_number = 0
  local stdout_assertion_slice_start_index = 1
  local stderr_assertion_slice_start_index = 1
  local is_assert_exit_code_instruction_detected = false
  for e2e_testfile_line in assert(e2e_test_filehandle):lines() do
    e2e_testfile_line_number = e2e_testfile_line_number + 1

    local assert_instruction_start_index, assert_instruction_end_index, assert_instruction =
      string.find(e2e_testfile_line, "(" .. assert_instruction_common_prefix .. "[%w_]*)")
    if not assert_instruction then
      goto continue
    end

    local argument_delimiter_start_index = assert_instruction_end_index + 1
    local argument_delimiter_end_index = argument_delimiter_start_index
    local argument_delimiter =
      string.sub(e2e_testfile_line, argument_delimiter_start_index, argument_delimiter_end_index)
    if not argument_delimiter then
      syntax_error(
        "Instruction '"
          .. assert_instruction
          .. "' is missing '"
          .. assert_instruction_argument_delimiter
          .. "'argument delimiter",
        e2e_testfile_line_number,
        assert_instruction_end_index
      )
    elseif argument_delimiter ~= assert_instruction_argument_delimiter then
      syntax_error(
        "Invalid argument delimiter '"
          .. argument_delimiter
          .. "' (expected '"
          .. assert_instruction_argument_delimiter
          .. "')",
        e2e_testfile_line_number,
        argument_delimiter_start_index
      )
    end

    local argument_start_index = argument_delimiter_end_index + 1
    local argument = string.sub(e2e_testfile_line, argument_start_index)
    if not argument then
      syntax_error(
        "Instruction '" .. assert_instruction .. "' is missing argument",
        e2e_testfile_line_number,
        argument_delimiter_end_index
      )
    end

    if assert_instruction == assert_exit_code_instruction then
      if is_assert_exit_code_instruction_detected == true then
        syntax_error(
          "Second '" .. assert_instruction .. "' instruction detected (only one allowed)",
          e2e_testfile_line_number,
          assert_instruction_start_index
        )
      end
      is_assert_exit_code_instruction_detected = true

      local exit_code_argument = string.match(argument, "^%d+$")
      if not exit_code_argument then
        syntax_error(
          "Invalid '" .. assert_instruction .. "' argument '" .. argument .. "' (expected signless integer)",
          e2e_testfile_line_number,
          argument_start_index
        )
      end

      exit_code_argument = assert(tonumber(exit_code_argument))
      if exit_code_argument < 0 or exit_code_argument > 255 then
        syntax_error(
          "Out-of-bounds '" .. assert_instruction .. "' argument '" .. argument .. "' (valid range: 0-255)",
          e2e_testfile_line_number,
          argument_start_index
        )
      end

      expected_exit_code = exit_code_argument
    elseif assert_instruction == assert_stdout_instruction then
      local processed_argument = decode_assertion_argument_escape_sequences(argument)
      local stdout_assertion_slice_end_index = stdout_assertion_slice_start_index + processed_argument:len() - 1

      local stdout_assertion_slice =
        e2e_test_stdout:sub(stdout_assertion_slice_start_index, stdout_assertion_slice_end_index)
      stdout_assertion_slice_start_index = stdout_assertion_slice_end_index + 1

      if stdout_assertion_slice ~= processed_argument .. "\n" then
        assertion_failure(
          "Stdout '" .. stdout_assertion_slice .. "' doesn't equal expected '" .. processed_argument .. "'",
          e2e_testfile_line_number,
          assert_instruction_start_index
        )
      end
    elseif assert_instruction == assert_stderr_instruction then
      local processed_argument = decode_assertion_argument_escape_sequences(argument)
      local stderr_assertion_slice_end_index = stderr_assertion_slice_start_index + processed_argument:len() - 1

      local stderr_assertion_slice =
        e2e_test_stderr:sub(stderr_assertion_slice_start_index, stderr_assertion_slice_end_index)
      stderr_assertion_slice_start_index = stderr_assertion_slice_end_index + 1

      if stderr_assertion_slice ~= processed_argument .. "\n" then
        assertion_failure(
          "Stderr '" .. stderr_assertion_slice .. "' doesn't equal expected '" .. processed_argument .. "'",
          e2e_testfile_line_number,
          assert_instruction_start_index
        )
      end
    else
      syntax_error(
        "Invalid '" .. assert_instruction .. "' instruction",
        e2e_testfile_line_number,
        assert_instruction_start_index
      )
    end

    ::continue::
  end
end

if e2e_test_exit_code ~= expected_exit_code then
  assertion_failure("Exit code '" .. e2e_test_exit_code .. "' doesn't equal expected '" .. expected_exit_code .. "'")
end

assert(assert(e2e_test_filehandle):close(), "Failed to close '" .. e2e_test_filepath .. "'")
