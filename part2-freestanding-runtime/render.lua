-- This is the replacement for printf, written in Lua.
-- It receives the format string and the decoded arguments, formats them,
-- and prints. Edit this file to change the target's behavior -- no recompile.
return function(fmt, ...)
  emit("[lua] " .. string.format(fmt, ...))
end
