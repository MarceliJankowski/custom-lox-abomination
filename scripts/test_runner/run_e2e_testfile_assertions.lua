#!/usr/bin/env luajit

--------------------------------------------------
--            COMPILE-TIME CONSTANTS            --
--------------------------------------------------

local ASSERTION_ID_PREFIX = "#ASSERT_"

local EXIT_CODE_ASSERTION_ID = ASSERTION_ID_PREFIX .. "EXIT_CODE"

local STDOUT_ASSERTION_ID = ASSERTION_ID_PREFIX .. "STDOUT"
local STDOUT_LINE_ASSERTION_ID = ASSERTION_ID_PREFIX .. "STDOUT_LINE"

local STDERR_ASSERTION_ID = ASSERTION_ID_PREFIX .. "STDERR"
local STDERR_LINE_ASSERTION_ID = ASSERTION_ID_PREFIX .. "STDERR_LINE"

local LEXICAL_ERROR_ASSERTION_ID = ASSERTION_ID_PREFIX .. "LEXICAL_ERROR"
local SYNTAX_ERROR_ASSERTION_ID = ASSERTION_ID_PREFIX .. "SYNTAX_ERROR"
local SEMANTIC_ERROR_ASSERTION_ID = ASSERTION_ID_PREFIX .. "SEMANTIC_ERROR"
local EXECUTION_ERROR_ASSERTION_ID = ASSERTION_ID_PREFIX .. "EXECUTION_ERROR"

local ASSERTION_ARG_DELIMITER_CHAR = " " -- this constant gets used in RegExprs (it shouldn't be a metacharacter)

local CLA_ERROR_TYPE_MAP = {
  [LEXICAL_ERROR_ASSERTION_ID] = "[LEXICAL_ERROR]",
  [SYNTAX_ERROR_ASSERTION_ID] = "[SYNTAX_ERROR]",
  [SEMANTIC_ERROR_ASSERTION_ID] = "[SEMANTIC_ERROR]",
  [EXECUTION_ERROR_ASSERTION_ID] = "[EXECUTION_ERROR]",
}

local CLA_COMPILATION_ERROR_CODE = 1
local CLA_EXECUTION_ERROR_CODE = 2

local CLA_M_S = " - "
local CLA_P_S = ":"

local M_S = CLA_M_S -- Message Separator
local P_S = CLA_P_S -- Position Separator

--------------------------------------------------
--                  UTILITIES                   --
--------------------------------------------------

-- forward declarations
local e2e_testfile_path

-- @desc log error consisting of `error_type` and `message` to stderr and terminate execution
local function log_error_and_terminate_execution(error_type, message)
  assert(type(error_type) == "string")
  assert(type(message) == "string")

  io.stderr:write("[" .. error_type .. "]" .. M_S .. message .. "\n")
  os.exit(1)
end

-- @desc log Input/Output error `message` to stderr and terminate execution
local function io_error(message)
  assert(type(message) == "string")

  log_error_and_terminate_execution("IO_ERROR", message)
end

-- @desc log invalid argument error `message` to stderr and terminate execution
local function invalid_arg_error(message)
  assert(type(message) == "string")

  log_error_and_terminate_execution("INVALID_ARG_ERROR", message)
end

-- @desc decode `assertion_message_arg` escape sequences
-- @return `assertion_message_arg` with its escape sequences decoded (enabled)
local function decode_assertion_message_arg_escape_sequences(assertion_message_arg)
  assert(type(assertion_message_arg) == "string")

  return assertion_message_arg:gsub("\\\\", "\\"):gsub("\\n", "\n"):gsub("\\t", "\t")
end

-- @desc read file at `filepath`
-- @return `filepath` file content
local function read_file(filepath)
  assert(type(filepath) == "string")

  local filehandle, io_open_error = io.open(filepath, "rb")
  if not filehandle then
    io_error("Failed to open '" .. filepath .. "'" .. M_S .. io_open_error)
  end

  ---@diagnostic disable-next-line: need-check-nil
  local content, io_read_error = filehandle:read("a")
  if not content then
    io_error("Failed to read '" .. filepath .. "'" .. M_S .. io_read_error)
  end

  local did_io_close_succeed, io_close_error = io.close(filehandle)
  if not did_io_close_succeed then
    io_error("Failed to close '" .. filepath .. "'" .. M_S .. io_close_error)
  end

  return content
end

--------------------------------------------------
--        PROCESS COMMAND-LINE ARGUMENTS        --
--------------------------------------------------

e2e_testfile_path = arg[1] or invalid_arg_error("Missing e2e_testfile_path argument")
local e2e_testfile_stdout_path = arg[2] or invalid_arg_error("Missing e2e_testfile_stdout_path argument")
local e2e_testfile_stderr_path = arg[3] or invalid_arg_error("Missing e2e_testfile_stderr_path argument")
local e2e_testfile_exit_code = arg[4] or invalid_arg_error("Missing e2e_testfile_exit_code argument")

local e2e_testfile_handle, io_open_error = io.open(e2e_testfile_path, "rb")
if not e2e_testfile_handle then
  io_error("Failed to open '" .. e2e_testfile_path .. "'" .. M_S .. io_open_error)
end

local e2e_testfile_stdout = read_file(e2e_testfile_stdout_path)
local e2e_testfile_stderr = read_file(e2e_testfile_stderr_path)

if not e2e_testfile_exit_code:match("^%d+$") then
  invalid_arg_error(
    "Invalid e2e_testfile_exit_code argument '" .. e2e_testfile_exit_code .. "' (expected signless integer)"
  )
end
---@diagnostic disable-next-line: cast-local-type
e2e_testfile_exit_code = assert(tonumber(e2e_testfile_exit_code))
if e2e_testfile_exit_code < 0 or e2e_testfile_exit_code > 255 then
  invalid_arg_error(
    "Out-of-bounds e2e_testfile_exit_code argument '" .. e2e_testfile_exit_code .. "' (valid range: 0-255)"
  )
end

--------------------------------------------------
--     EXECUTE e2e_testfile_path ASSERTIONS     --
--------------------------------------------------

do -- execute e2e_testfile_path assertions against e2e_testfile_path output
  local e2e_testfile_line_number = 0

  local e2e_testfile_stdout_slice_start_index = 1
  local e2e_testfile_stderr_slice_start_index = 1

  local expected_e2e_testfile_exit_code = 0 -- this var should only be modified through its setter function

  -- forward declarations
  local assertion_id
  local assertion_id_start_index
  local outer_scope_assertion_arg_descriptor
  local outer_scope_e2e_testfile_line

  --------------------------------------------------
  --             ASSERTION UTILITIES              --
  --------------------------------------------------

  -- @desc log assertion syntax error `message` with e2e_testfile_line_number/`column` position and terminate execution
  local function syntax_error(message, column)
    assert(type(message) == "string")
    assert(type(column) == "number")

    log_error_and_terminate_execution(
      "ASSERTION_SYNTAX_ERROR",
      e2e_testfile_path .. P_S .. e2e_testfile_line_number .. P_S .. column .. M_S .. message
    )
  end

  -- @desc log assertion failure `message` with optional e2e_testfile_line_number/`column` position and terminate execution
  local function assertion_failure(message, column)
    assert(type(message) == "string")
    if column ~= nil then
      assert(type(column) == "number")
    end

    message = M_S .. message
    if column then
      message = P_S .. e2e_testfile_line_number .. P_S .. column .. message
    end
    message = e2e_testfile_path .. message

    log_error_and_terminate_execution("ASSERTION_FAILURE", message)
  end

  -- @desc assert `string_descriptor` `string_a` and `string_b` equality
  local function assert_string_equality(string_descriptor, string_a, string_b)
    assert(type(string_descriptor) == "string")
    assert(type(string_a) == "string")
    assert(type(string_b) == "string")

    if string_a ~= string_b then
      assertion_failure(
        "Expected " .. string_descriptor .. " '" .. string_a .. "' to equal '" .. string_b .. "'",
        assertion_id_start_index
      )
    end
  end

  -- @desc process assertion argument
  -- @param assertion_arg_descriptor string describing argument being processed
  -- @param processed_assertion_end_index end index of currently processed assertion up to this invocation
  -- @param assertion_arg_processor callback invoked with assertion_arg_delimiter_end_index and assertion_arg_start_index
  -- @return value(s) returned by `assertion_arg_processor`
  local function process_assertion_arg(assertion_arg_descriptor, processed_assertion_end_index, assertion_arg_processor)
    assert(type(assertion_arg_descriptor) == "string")
    assert(type(processed_assertion_end_index) == "number")
    assert(type(assertion_arg_processor) == "function")

    outer_scope_assertion_arg_descriptor = assertion_arg_descriptor

    local assertion_arg_delimiter_start_index = processed_assertion_end_index + 1
    local assertion_arg_delimiter_end_index = assertion_arg_delimiter_start_index
      + ASSERTION_ARG_DELIMITER_CHAR:len()
      - 1
    local assertion_arg_delimiter =
      outer_scope_e2e_testfile_line:sub(assertion_arg_delimiter_start_index, assertion_arg_delimiter_end_index)
    if not assertion_arg_delimiter then
      syntax_error(
        "Assertion "
          .. assertion_id
          .. " is missing '"
          .. ASSERTION_ARG_DELIMITER_CHAR
          .. "' "
          .. assertion_arg_descriptor
          .. " argument delimiter",
        processed_assertion_end_index
      )
    elseif assertion_arg_delimiter ~= ASSERTION_ARG_DELIMITER_CHAR then
      syntax_error(
        "Invalid "
          .. assertion_id
          .. " "
          .. assertion_arg_descriptor
          .. " argument delimiter '"
          .. assertion_arg_delimiter
          .. "' (expected '"
          .. ASSERTION_ARG_DELIMITER_CHAR
          .. "')",
        assertion_arg_delimiter_start_index
      )
    end

    local assertion_arg_start_index = assertion_arg_delimiter_end_index + 1

    return assertion_arg_processor(assertion_arg_delimiter_end_index, assertion_arg_start_index)
  end

  -- @desc convert `assertion_arg` starting at `assertion_arg_start_index` into signless integer
  -- @return `assertion_arg` signless integer
  local function convert_assertion_arg_to_signless_int(assertion_arg, assertion_arg_start_index)
    assert(type(assertion_arg) == "string")
    assert(type(assertion_arg_start_index) == "number")

    local _ = assertion_arg:match("^%d+$")
      or syntax_error(
        "Invalid "
          .. assertion_id
          .. " "
          .. outer_scope_assertion_arg_descriptor
          .. " argument '"
          .. assertion_arg
          .. "' (expected signless integer)",
        assertion_arg_start_index
      )

    return assert(tonumber(assertion_arg))
  end

  --------------------------------------------------
  --               GETTERS/SETTERS                --
  --------------------------------------------------

  -- @desc set expected_e2e_testfile_exit_code to `expected_exit_code`
  local set_expected_e2e_testfile_exit_code = (function()
    local setting_assertion_id = nil

    return function(expected_exit_code)
      assert(type(expected_exit_code) == "number")

      if setting_assertion_id ~= nil then
        syntax_error(
          "Assertion "
            .. assertion_id
            .. " appears after "
            .. setting_assertion_id
            .. " (only one assertion setting expected_e2e_testfile_exit_code is allowed)",
          assertion_id_start_index
        )
      end
      setting_assertion_id = assertion_id
      expected_e2e_testfile_exit_code = expected_exit_code
    end
  end)()

  --------------------------------------------------
  --           ASSERTION EXECUTION LOOP           --
  --------------------------------------------------
  ---@diagnostic disable-next-line: need-check-nil
  for e2e_testfile_line in e2e_testfile_handle:lines() do
    outer_scope_e2e_testfile_line = e2e_testfile_line
    e2e_testfile_line_number = e2e_testfile_line_number + 1

    local assertion_id_end_index
    assertion_id_start_index, assertion_id_end_index, assertion_id =
      string.find(e2e_testfile_line, "(" .. ASSERTION_ID_PREFIX .. "[_%w]*)")
    if not assertion_id then
      goto continue
    end

    if assertion_id == EXIT_CODE_ASSERTION_ID then
      --------------------------------------------------
      --             EXIT_CODE_ASSERTION              --
      --------------------------------------------------
      local expected_exit_code_arg = process_assertion_arg(
        "expected_exit_code",
        assertion_id_end_index,
        function(delimiter_end_index, arg_start_index)
          local arg = e2e_testfile_line:sub(arg_start_index)
            or syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_exit_code argument",
              delimiter_end_index
            )

          arg = convert_assertion_arg_to_signless_int(arg, arg_start_index)
          if arg < 0 or arg > 255 then
            syntax_error(
              "Out-of-bounds " .. assertion_id .. " expected_exit_code argument '" .. arg .. "' (valid range: 0-255)",
              arg_start_index
            )
          end

          return arg
        end
      )

      set_expected_e2e_testfile_exit_code(expected_exit_code_arg)
    elseif assertion_id == STDOUT_ASSERTION_ID or assertion_id == STDOUT_LINE_ASSERTION_ID then
      --------------------------------------------------
      --           STDOUT_{,LINE}_ASSERTION           --
      --------------------------------------------------
      local decoded_expected_stdout_arg = process_assertion_arg(
        "expected_stdout",
        assertion_id_end_index,
        function(delimiter_end_index, arg_start_index)
          local arg = e2e_testfile_line:sub(arg_start_index)
            or syntax_error("Assertion " .. assertion_id .. " is missing expected_stdout argument", delimiter_end_index)

          local decoded_arg = decode_assertion_message_arg_escape_sequences(arg)
          if assertion_id == STDOUT_LINE_ASSERTION_ID then
            decoded_arg = decoded_arg .. "\n"
          end

          return decoded_arg
        end
      )

      local e2e_testfile_stdout_slice_end_index = e2e_testfile_stdout_slice_start_index
        + decoded_expected_stdout_arg:len()
        - 1
      local e2e_testfile_stdout_slice =
        e2e_testfile_stdout:sub(e2e_testfile_stdout_slice_start_index, e2e_testfile_stdout_slice_end_index)
      e2e_testfile_stdout_slice_start_index = e2e_testfile_stdout_slice_end_index + 1

      assert_string_equality("stdout", e2e_testfile_stdout_slice, decoded_expected_stdout_arg)
    elseif assertion_id == STDERR_ASSERTION_ID or assertion_id == STDERR_LINE_ASSERTION_ID then
      --------------------------------------------------
      --           STDERR_{,LINE}_ASSERTION           --
      --------------------------------------------------
      local decoded_expected_stderr_arg = process_assertion_arg(
        "expected_stderr",
        assertion_id_end_index,
        function(delimiter_end_index, arg_start_index)
          local arg = e2e_testfile_line:sub(arg_start_index)
            or syntax_error("Assertion " .. assertion_id .. " is missing expected_stderr argument", delimiter_end_index)

          local decoded_arg = decode_assertion_message_arg_escape_sequences(arg)
          if assertion_id == STDERR_LINE_ASSERTION_ID then
            decoded_arg = decoded_arg .. "\n"
          end

          return decoded_arg
        end
      )

      local e2e_testfile_stderr_slice_end_index = e2e_testfile_stderr_slice_start_index
        + decoded_expected_stderr_arg:len()
        - 1
      local e2e_testfile_stderr_slice =
        e2e_testfile_stderr:sub(e2e_testfile_stderr_slice_start_index, e2e_testfile_stderr_slice_end_index)
      e2e_testfile_stderr_slice_start_index = e2e_testfile_stderr_slice_end_index + 1

      assert_string_equality("stderr", e2e_testfile_stderr_slice, decoded_expected_stderr_arg)
    elseif
      assertion_id == LEXICAL_ERROR_ASSERTION_ID
      or assertion_id == SYNTAX_ERROR_ASSERTION_ID
      or assertion_id == SEMANTIC_ERROR_ASSERTION_ID
    then
      --------------------------------------------------
      --  {LEXICAL,SYNTAX,SEMANTIC}_ERROR_ASSERTION   --
      --------------------------------------------------
      local expected_error_line_arg_end_index, expected_error_line_arg = process_assertion_arg(
        "expected_error_line",
        assertion_id_end_index,
        function(delimiter_end_index, arg_start_index)
          local _, arg_end_index, arg =
            e2e_testfile_line:find("^([^" .. ASSERTION_ARG_DELIMITER_CHAR .. "]+)", arg_start_index)
          if not arg then
            syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_error_line argument",
              delimiter_end_index
            )
          end

          return arg_end_index, convert_assertion_arg_to_signless_int(arg, arg_start_index)
        end
      )

      local expected_error_column_arg_end_index, expected_error_column_arg = process_assertion_arg(
        "expected_error_column",
        expected_error_line_arg_end_index,
        function(delimiter_end_index, arg_start_index)
          local _, arg_end_index, arg =
            e2e_testfile_line:find("^([^" .. ASSERTION_ARG_DELIMITER_CHAR .. "]+)", arg_start_index)
          if not arg then
            syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_error_column argument",
              delimiter_end_index
            )
          end

          return arg_end_index, convert_assertion_arg_to_signless_int(arg, arg_start_index)
        end
      )

      local decoded_expected_error_message_arg = process_assertion_arg(
        "expected_error_message",
        expected_error_column_arg_end_index,
        function(delimiter_end_index, arg_start_index)
          local arg = e2e_testfile_line:sub(arg_start_index)
            or syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_error_message argument",
              delimiter_end_index
            )

          return decode_assertion_message_arg_escape_sequences(arg) .. "\n"
        end
      )

      set_expected_e2e_testfile_exit_code(CLA_COMPILATION_ERROR_CODE)

      local expected_error = CLA_ERROR_TYPE_MAP[assertion_id]
        .. CLA_M_S
        .. e2e_testfile_path
        .. CLA_P_S
        .. expected_error_line_arg
        .. CLA_P_S
        .. expected_error_column_arg
        .. CLA_M_S
        .. decoded_expected_error_message_arg

      local _, e2e_testfile_stderr_slice_end_index, e2e_testfile_stderr_slice =
        e2e_testfile_stderr:find("(.+\n)", e2e_testfile_stderr_slice_start_index)
      if not e2e_testfile_stderr_slice then
        e2e_testfile_stderr_slice_end_index = e2e_testfile_stderr_slice_start_index + expected_error:len() - 1
        e2e_testfile_stderr_slice =
          e2e_testfile_stderr:sub(e2e_testfile_stderr_slice_start_index, e2e_testfile_stderr_slice_end_index)
      end
      e2e_testfile_stderr_slice_start_index = e2e_testfile_stderr_slice_end_index + 1

      assert_string_equality("stderr", e2e_testfile_stderr_slice, expected_error)
    elseif assertion_id == EXECUTION_ERROR_ASSERTION_ID then
      --------------------------------------------------
      --          EXECUTION_ERROR_ASSERTION           --
      --------------------------------------------------
      local expected_error_line_arg_end_index, expected_error_line_arg = process_assertion_arg(
        "expected_error_line",
        assertion_id_end_index,
        function(delimiter_end_index, arg_start_index)
          local _, arg_end_index, arg =
            e2e_testfile_line:find("^([^" .. ASSERTION_ARG_DELIMITER_CHAR .. "]+)", arg_start_index)
          if not arg then
            syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_error_line argument",
              delimiter_end_index
            )
          end

          return arg_end_index, convert_assertion_arg_to_signless_int(arg, arg_start_index)
        end
      )

      local decoded_expected_error_message_arg = process_assertion_arg(
        "expected_error_message",
        expected_error_line_arg_end_index,
        function(delimiter_end_index, arg_start_index)
          local arg = e2e_testfile_line:sub(arg_start_index)
            or syntax_error(
              "Assertion " .. assertion_id .. " is missing expected_error_message argument",
              delimiter_end_index
            )

          return decode_assertion_message_arg_escape_sequences(arg) .. "\n"
        end
      )

      set_expected_e2e_testfile_exit_code(CLA_EXECUTION_ERROR_CODE)

      local expected_error = CLA_ERROR_TYPE_MAP[assertion_id]
        .. CLA_M_S
        .. e2e_testfile_path
        .. CLA_P_S
        .. expected_error_line_arg
        .. CLA_M_S
        .. decoded_expected_error_message_arg

      local _, e2e_testfile_stderr_slice_end_index, e2e_testfile_stderr_slice =
        e2e_testfile_stderr:find("(.+\n)", e2e_testfile_stderr_slice_start_index)
      if not e2e_testfile_stderr_slice then
        e2e_testfile_stderr_slice_end_index = e2e_testfile_stderr_slice_start_index + expected_error:len() - 1
        e2e_testfile_stderr_slice =
          e2e_testfile_stderr:sub(e2e_testfile_stderr_slice_start_index, e2e_testfile_stderr_slice_end_index)
      end
      e2e_testfile_stderr_slice_start_index = e2e_testfile_stderr_slice_end_index + 1

      assert_string_equality("stderr", e2e_testfile_stderr_slice, expected_error)
    else
      --------------------------------------------------
      --         INVALID ASSERTION IDENTIFIER         --
      --------------------------------------------------
      syntax_error("Invalid assertion identifier '" .. assertion_id .. "'", assertion_id_start_index)
    end

    ::continue::
  end

  local e2e_testfile_stdout_slice = e2e_testfile_stdout:sub(e2e_testfile_stdout_slice_start_index)
  if e2e_testfile_stdout_slice:len() > 0 then
    assertion_failure("Unasserted stdout '" .. e2e_testfile_stdout_slice .. "'")
  end

  local e2e_testfile_stderr_slice = e2e_testfile_stderr:sub(e2e_testfile_stderr_slice_start_index)
  if e2e_testfile_stderr_slice:len() > 0 then
    assertion_failure("Unasserted stderr '" .. e2e_testfile_stderr_slice .. "'")
  end

  if e2e_testfile_exit_code ~= expected_e2e_testfile_exit_code then
    assertion_failure(
      "Expected exit code '" .. e2e_testfile_exit_code .. "' to equal '" .. expected_e2e_testfile_exit_code .. "'"
    )
  end
end

--------------------------------------------------
--                   CLEAN UP                   --
--------------------------------------------------

local did_io_close_succeed, io_close_error = io.close(e2e_testfile_handle)
if not did_io_close_succeed then
  io_error("Failed to close '" .. e2e_testfile_path .. "'" .. M_S .. io_close_error)
end
