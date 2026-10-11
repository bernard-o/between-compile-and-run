-- Two distinct Lua functions, one per intercepted symbol.
-- printf: formats varargs (no \n; the fmt carries it). puts: takes the string
-- and ADDS a \n. Distinct prefixes prove each call reached its own function.
local function render_printf(fmt, ...)
  emit("[lua:printf] " .. string.format(fmt, ...))
end
local function render_puts(s)
  emit("[lua:puts] " .. s .. "\n")      -- puts semantics: write s and a newline
end
return render_printf, render_puts
