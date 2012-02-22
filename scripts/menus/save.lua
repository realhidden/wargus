function RunSaveGame(name, menu)
  if (SaveGame(name) == -1) then
    local confirm = WarGameMenu(panel(3))
    confirm:resize(300,120)
    confirm:addLabel("Cannot save game to file:", 300 / 2, 11)
    confirm:addLabel(name, 300 / 2, 31)
    confirm:addHalfButton("~!OK", "o", 1 * (300 / 3), 120 - 16 - 27, function() confirm:stop() end)
    confirm:run(false)
  else
    UI.StatusLine:Set("Saved game to: " .. name)
    menu:stop()
  end
end

function RunConfirmErase(name, menu)
  local confirm = WarGameMenu(panel(3))

  confirm:resize(300,120)

  confirm:addLabel(name, 300 / 2, 11)
  confirm:addLabel("File exists, are you sure ?", 300 / 2, 31)

  confirm:addHalfButton("~!Yes", "y", 1 * (300 / 3) - 90, 120 - 16 - 27,
    function()
        confirm:stop()
        RunSaveGame(name, menu)
    end)

  confirm:addHalfButton("~!No", "n", 3 * (300 / 3) - 116, 120 - 16 - 27,
    function() confirm:stop() end)

  confirm:run(false)
end

function RunSaveMenu()
  local menu = WarGameMenu(panel(3))
  menu:resize(384, 256)

  menu:addLabel("Save Game", 384 / 2, 11)

  local t = menu:addTextInputField("game.sav",
    (384 - 300 - 18) / 2, 11 + 36, 318)

  local browser = menu:addBrowser("~save", ".sav.gz$",
    (384 - 300 - 18) / 2, 11 + 36 + 22, 318, 126)
  local function cb(s)
    t:setText(browser:getSelectedItem())
  end
  browser:setActionCallback(cb)

  menu:addHalfButton("~!Save", "s", (384 - 300 - 18) / 2, 256 - 16 - 27,
    function()
      local name = t:getText()
      -- check for an empty string
      if (string.len(name) == 0) then
        return
      end
      -- strip .gz
      if (string.find(name, ".gz$") ~= nil) then
        name = string.sub(name, 1, string.len(name) - 3)
      end
      -- append .sav
      if (string.find(name, ".sav$") == nil) then
        name = name .. ".sav"
      end
      -- replace invalid chars with underscore
      local t = {"\\", "/", ":", "*", "?", "\"", "<", ">", "|"}
      table.foreachi(t, function(k,v) name = string.gsub(name, v, "_") end)

      if (browser:exists(name .. ".gz")) then
        RunConfirmErase(name, menu)
      else
        RunSaveGame(name, menu)
      end
    end)

  menu:addHalfButton("~!Cancel", "c", 384 - ((384 - 300 - 18) / 2) - 106, 256 - 16 - 27,
    function() menu:stop() end)

  menu:run(false)
end

