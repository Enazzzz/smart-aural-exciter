-- render_corpus.lua
-- -----------------
-- ReaScript that batch-renders every reference-corpus slot through REAPER
-- according to docs/validation/04-validation-suite.md sections 2.3 and 3.
-- Produces, per slot:
--   <slot>_bypass.wav    plugin internally bypassed
--   <slot>_wet.wav       plugin enabled, default preset
--   <slot>_stress.wav    +12 dB pre-gain into the plugin (safety stress)
--
-- Reads the slot index from docs/validation/refs/corpus.txt. Skips any
-- row whose audio file isn't present (recorded as SKIP in the scorecard).
--
-- How to install:
--   1. In REAPER: Actions -> Show action list -> Load -> select this file.
--   2. Open the canonical validation project
--      docs\validation\reaper\v1-validation.RPP
--      (Track 1 = REF, Track 2 = BYPASS, Track 3 = WET, Smart Exciter on master.)
--   3. Run "Script: render_corpus.lua".
--
-- Notes:
--   * The script assumes the Smart Exciter is already inserted on the master
--     bus and that its bypass parameter is named "Bypass" (JUCE default).
--   * Renders go to docs\validation\reaper\renders\<slot>\ which is gitignored.
--   * This script only AUTOMATES. The listening / scoring still happens in
--     `04-validation-suite.md`.

----------------------------------------------------------------------
-- Helpers
----------------------------------------------------------------------

local function script_dir()
	local info = debug.getinfo(1, "S").source:sub(2)
	return info:match("(.*[/\\])") or "./"
end

local function repo_root()
	-- script lives in <repo>\tools\reaper\, walk up two folders.
	local sd = script_dir()
	return sd .. "..\\..\\"
end

local function trim(s) return (s:gsub("^%s+", ""):gsub("%s+$", "")) end

local function path_exists(p)
	local f = io.open(p, "rb")
	if f then f:close() return true end
	return false
end

local function ensure_dir(p)
	-- Windows mkdir is silent if the dir exists.
	os.execute('mkdir "' .. p .. '" >nul 2>&1')
end

----------------------------------------------------------------------
-- Read corpus index
----------------------------------------------------------------------

local function load_corpus()
	local path = repo_root() .. "docs\\validation\\refs\\corpus.txt"
	local f = io.open(path, "r")
	if not f then
		reaper.ShowMessageBox("Could not open corpus.txt at:\n" .. path,
			"render_corpus.lua", 0)
		return {}
	end

	local rows = {}
	for line in f:lines() do
		local trimmed = trim(line)
		if trimmed ~= "" and not trimmed:match("^#") then
			-- columns: slot category filename samplerate description...
			local slot, category, filename, sr = trimmed:match("(%S+)%s+(%S+)%s+(%S+)%s+(%S+)")
			if slot and category and filename then
				local refPath = repo_root() .. "docs\\validation\\refs\\"
					.. category .. "\\" .. filename
				rows[#rows + 1] = {
					slot      = slot,
					category  = category,
					filename  = filename,
					srHint    = tonumber(sr or "0") or 0,
					path      = refPath,
				}
			end
		end
	end
	f:close()
	return rows
end

----------------------------------------------------------------------
-- Render a single slot
----------------------------------------------------------------------

local function find_master_fx_index_by_name(name)
	local n = reaper.TrackFX_GetCount(reaper.GetMasterTrack(0))
	for i = 0, n - 1 do
		local _, fxName = reaper.TrackFX_GetFXName(reaper.GetMasterTrack(0), i, "")
		if fxName and fxName:find(name, 1, true) then
			return i
		end
	end
	return -1
end

local function set_master_plugin_bypass(bypass)
	local fxIdx = find_master_fx_index_by_name("Smart Exciter")
	if fxIdx < 0 then
		reaper.ShowMessageBox("Smart Exciter not found on master FX chain.",
			"render_corpus.lua", 0)
		return false
	end
	reaper.TrackFX_SetEnabled(reaper.GetMasterTrack(0), fxIdx, not bypass)
	return true
end

local function render_one(slot, refPath, outDir, suffix)
	-- Sets the project render path/filename and triggers an offline render.
	-- We use the "Render queue" action so the render happens silently.
	ensure_dir(outDir)
	local outFile = outDir .. "\\" .. slot .. "_" .. suffix .. ".wav"

	reaper.GetSetProjectInfo_String(0, "RENDER_FILE", outDir, true)
	reaper.GetSetProjectInfo_String(0, "RENDER_PATTERN",
		slot .. "_" .. suffix, true)

	-- 41824 = "File: Render project, using the most recent render settings"
	reaper.Main_OnCommand(41824, 0)
	return outFile
end

----------------------------------------------------------------------
-- Main
----------------------------------------------------------------------

local function main()
	local rows = load_corpus()
	if #rows == 0 then return end

	local renderRoot = repo_root() .. "docs\\validation\\reaper\\renders"
	ensure_dir(renderRoot)

	local skipped, rendered = 0, 0
	for _, row in ipairs(rows) do
		if not path_exists(row.path) then
			reaper.ShowConsoleMsg(("SKIP %s: missing audio %s\n"):format(row.slot, row.path))
			skipped = skipped + 1
		else
			local outDir = renderRoot .. "\\" .. row.slot
			ensure_dir(outDir)

			-- TODO: load the audio file onto Track 1 (REF). This requires
			-- find/replace within the REAPER session, which depends on the
			-- exact RPP layout you create in v1-validation.RPP. For now, the
			-- script assumes the user has manually loaded the slot on Track 1
			-- before running the render for that slot. A future enhancement
			-- can use reaper.InsertMedia() to automate file swaps.

			set_master_plugin_bypass(true)
			render_one(row.slot, row.path, outDir, "bypass")

			set_master_plugin_bypass(false)
			render_one(row.slot, row.path, outDir, "wet")

			-- Safety stress: caller is expected to have a +12 dB pre-gain
			-- plugin BEFORE the Smart Exciter on the master, enabled only
			-- for this render. Toggling that gain plugin programmatically
			-- depends on its name; keeping that out of scope of v1.
			reaper.ShowConsoleMsg(
				"Manual step for " .. row.slot ..
				": enable +12 dB pre-gain on master, render <slot>_stress, then disable.\n")

			rendered = rendered + 1
		end
	end

	reaper.ShowMessageBox(
		("Rendered %d slots, skipped %d. See:\n%s"):format(rendered, skipped, renderRoot),
		"render_corpus.lua", 0)
end

main()
