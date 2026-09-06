-- Tesla Menu Overlay Preview for Sharpscale-NX
-- Replicates libtesla / ovl-sharpscale.ovl in full 1280x720 resolution

local overlay = {
    title = "Sharpscale-NX",
    version = "v1.0.0",
    subtitle = "Pixel-Perfect Display Scaler & Filters",
    width = 420,
    selected_idx = 1,
    categories = {
        {
            name = "DISPLAY SCALING",
            items = {
                {
                    label = "Scaling Mode",
                    options = {
                        "Integer (Pixel Perfect)",
                        "Real (1:1 Native Centered)",
                        "Fit (Aspect Correct Fit)",
                        "Original (System Bilinear)"
                    },
                    current = 1,
                    type = "cycle"
                }
            }
        },
        {
            name = "FILTERS & SHARPENING",
            items = {
                {
                    label = "Filter Type",
                    options = {
                        "AMD CAS (Contrast Adaptive)",
                        "Sharp Bilinear",
                        "Point (Nearest Neighbor)",
                        "Bicubic Spline",
                        "Bilinear"
                    },
                    current = 1,
                    type = "cycle"
                }
            }
        },
        {
            name = "GEOMETRY & ASPECT RATIO",
            items = {
                {
                    label = "Aspect Ratio",
                    options = {
                        "Auto (Game Default)",
                        "16:9 Widescreen",
                        "4:3 Retro Standard",
                        "3:2 GBA Native",
                        "1:1 Square Pixel",
                        "10:9 Game Boy Native"
                    },
                    current = 1,
                    type = "cycle"
                }
            }
        },
        {
            name = "VIDEO CAPTURE",
            items = {
                {
                    label = "Unlock 1080p Stream",
                    sub = "SysDVR USB Streaming",
                    state = true,
                    type = "toggle"
                }
            }
        }
    }
}

-- Flatten items for easy navigation
local flat_items = {}
for c_idx, cat in ipairs(overlay.categories) do
    for i_idx, item in ipairs(cat.items) do
        table.insert(flat_items, {cat = cat, item = item})
    end
end

function love.load()
    local f = io.open("sdmc:/love_test.txt", "w")
    if f then
        f:write("LovePotion loaded!\n")
        f:close()
    end
    love.graphics.setDefaultFilter("linear", "linear")
    font_large = love.graphics.newFont(22)
    font_med = love.graphics.newFont(15)
    font_small = love.graphics.newFont(12)
    font_sub = love.graphics.newFont(11)
end

local frame_count = 0
function love.draw()
    local screen_w, screen_h = love.graphics.getDimensions()

    frame_count = frame_count + 1
    if frame_count == 30 then
        local log = io.open("sdmc:/screenshot_log.txt", "w")
        if log then log:write("Attempting screenshot at frame 30\n") end
        
        pcall(function()
            if love.graphics.captureScreenshot then
                love.graphics.captureScreenshot("preview.png")
            end
        end)

        pcall(function()
            if love.graphics.captureScreenshot then
                love.graphics.captureScreenshot(function(imgData)
                    local f = io.open("sdmc:/preview_cb.png", "wb")
                    if f then
                        local data = imgData:encode("png")
                        f:write(data:getString())
                        f:close()
                    end
                end)
            end
        end)

        pcall(function()
            if love.graphics.newScreenshot then
                local img = love.graphics.newScreenshot()
                local data = img:encode("png")
                local f = io.open("sdmc:/preview_new.png", "wb")
                if f then f:write(data:getString()); f:close() end
            end
        end)

        if log then
            log:write("Screenshot attempt completed\n")
            log:close()
        end
    end

    -- 1. BACKGROUND: Simulated game running behind overlay
    love.graphics.setColor(0.08, 0.08, 0.12, 1)
    love.graphics.rectangle("fill", 0, 0, screen_w, screen_h)
    
    -- Draw retro starfield / grid
    love.graphics.setColor(0.18, 0.2, 0.35, 0.4)
    for x = 0, screen_w, 40 do
        love.graphics.line(x, 0, x, screen_h)
    end
    for y = 0, screen_h, 40 do
        love.graphics.line(0, y, screen_w, y)
    end

    -- Simulated game viewport center
    local gw, gh = 640, 480
    local gx = overlay.width + (screen_w - overlay.width - gw) / 2
    local gy = (screen_h - gh) / 2
    love.graphics.setColor(0.2, 0.15, 0.3, 0.8)
    love.graphics.rectangle("fill", gx, gy, gw, gh, 8, 8)
    love.graphics.setColor(0.5, 0.4, 0.8, 0.9)
    love.graphics.setLineWidth(2)
    love.graphics.rectangle("line", gx, gy, gw, gh, 8, 8)

    love.graphics.setFont(font_large)
    love.graphics.setColor(0.9, 0.85, 1, 0.6)
    love.graphics.printf("RUNNING GAME (ACTIVE NVN SURFACE)", gx, gy + 200, gw, "center")
    love.graphics.setFont(font_small)
    love.graphics.setColor(0.6, 0.55, 0.7, 0.6)
    love.graphics.printf("Resolution: 1280x720 Docked / Native Scaling Active", gx, gy + 240, gw, "center")

    -- 2. DIMMING BACKDROP (Tesla Menu Dim Layer)
    love.graphics.setColor(0, 0, 0, 0.62)
    love.graphics.rectangle("fill", 0, 0, screen_w, screen_h)

    -- 3. TESLA OVERLAY DRAWER
    local ow = overlay.width
    love.graphics.setColor(0.11, 0.11, 0.12, 0.96)
    love.graphics.rectangle("fill", 0, 0, ow, screen_h)

    -- Right separator line
    love.graphics.setColor(1, 1, 1, 0.12)
    love.graphics.setLineWidth(1)
    love.graphics.line(ow, 0, ow, screen_h)

    -- HEADER
    local pad_x = 24
    local y_cursor = 28
    
    love.graphics.setFont(font_large)
    love.graphics.setColor(1, 1, 1, 1)
    love.graphics.print(overlay.title, pad_x, y_cursor)

    -- Version pill badge
    local title_w = font_large:getWidth(overlay.title)
    local badge_x = pad_x + title_w + 10
    love.graphics.setColor(0, 0.9, 1, 0.2)
    love.graphics.rectangle("fill", badge_x, y_cursor + 2, 54, 20, 4, 4)
    love.graphics.setColor(0, 0.9, 1, 0.8)
    love.graphics.rectangle("line", badge_x, y_cursor + 2, 54, 20, 4, 4)
    love.graphics.setFont(font_sub)
    love.graphics.setColor(0, 0.9, 1, 1)
    love.graphics.print(overlay.version, badge_x + 8, y_cursor + 5)

    y_cursor = y_cursor + 32
    love.graphics.setFont(font_sub)
    love.graphics.setColor(0.6, 0.6, 0.64, 1)
    love.graphics.print(overlay.subtitle, pad_x, y_cursor)

    y_cursor = y_cursor + 22
    love.graphics.setColor(1, 1, 1, 0.1)
    love.graphics.line(pad_x, y_cursor, ow - pad_x, y_cursor)
    y_cursor = y_cursor + 18

    -- LIST ITEMS (libtesla layout)
    local item_flat_idx = 1
    for c_idx, cat in ipairs(overlay.categories) do
        -- Category Header
        love.graphics.setFont(font_sub)
        love.graphics.setColor(0, 0.9, 1, 1) -- Cyan category text
        love.graphics.print(cat.name, pad_x + 4, y_cursor)
        y_cursor = y_cursor + 20

        for i_idx, item in ipairs(cat.items) do
            local is_selected = (item_flat_idx == overlay.selected_idx)
            local item_h = 52
            local item_w = ow - (pad_x * 2)

            -- Item background box
            if is_selected then
                love.graphics.setColor(1, 1, 1, 0.08)
            else
                love.graphics.setColor(1, 1, 1, 0.04)
            end
            love.graphics.rectangle("fill", pad_x, y_cursor, item_w, item_h, 6, 6)

            -- Selected focus glow & border
            if is_selected then
                love.graphics.setColor(0, 0.9, 1, 0.35)
                love.graphics.setLineWidth(3)
                love.graphics.rectangle("line", pad_x - 1, y_cursor - 1, item_w + 2, item_h + 2, 7, 7)
                love.graphics.setColor(0, 0.9, 1, 1)
                love.graphics.setLineWidth(1.5)
                love.graphics.rectangle("line", pad_x, y_cursor, item_w, item_h, 6, 6)
            end

            -- Label text
            love.graphics.setFont(font_med)
            love.graphics.setColor(0.92, 0.92, 0.95, 1)
            love.graphics.print(item.label, pad_x + 14, y_cursor + 10)

            -- Subtext or Value
            if item.type == "cycle" then
                local val_text = item.options[item.current]
                love.graphics.setFont(font_small)
                love.graphics.setColor(0, 0.9, 1, 0.95)
                local val_w = font_small:getWidth(val_text)
                love.graphics.print(val_text, pad_x + item_w - val_w - 14, y_cursor + 18)
            elseif item.type == "toggle" then
                if item.sub then
                    love.graphics.setFont(font_sub)
                    love.graphics.setColor(0.55, 0.55, 0.6, 1)
                    love.graphics.print(item.sub, pad_x + 14, y_cursor + 30)
                end

                -- Switch toggle switch
                local sw_w, sw_h = 42, 22
                local sw_x = pad_x + item_w - sw_w - 14
                local sw_y = y_cursor + (item_h - sw_h) / 2
                if item.state then
                    love.graphics.setColor(0, 0.9, 1, 1)
                    love.graphics.rectangle("fill", sw_x, sw_y, sw_w, sw_h, sw_h / 2, sw_h / 2)
                    love.graphics.setColor(1, 1, 1, 1)
                    love.graphics.circle("fill", sw_x + sw_w - sw_h / 2, sw_y + sw_h / 2, (sw_h - 4) / 2)
                else
                    love.graphics.setColor(0.24, 0.24, 0.26, 1)
                    love.graphics.rectangle("fill", sw_x, sw_y, sw_w, sw_h, sw_h / 2, sw_h / 2)
                    love.graphics.setColor(0.8, 0.8, 0.8, 1)
                    love.graphics.circle("fill", sw_x + sw_h / 2, sw_y + sw_h / 2, (sw_h - 4) / 2)
                end
            end

            y_cursor = y_cursor + item_h + 8
            item_flat_idx = item_flat_idx + 1
        end

        y_cursor = y_cursor + 10
    end

    -- FOOTER / CONTROLLER GLYPHS
    local footer_h = 48
    local fy = screen_h - footer_h
    love.graphics.setColor(0.06, 0.06, 0.07, 0.9)
    love.graphics.rectangle("fill", 0, fy, ow, footer_h)
    love.graphics.setColor(1, 1, 1, 0.1)
    love.graphics.line(0, fy, ow, fy)

    local function draw_glyph(btn, text, x)
        love.graphics.setColor(1, 1, 1, 0.15)
        love.graphics.circle("fill", x + 10, fy + 24, 10)
        love.graphics.setColor(1, 1, 1, 0.35)
        love.graphics.circle("line", x + 10, fy + 24, 10)
        love.graphics.setFont(font_sub)
        love.graphics.setColor(1, 1, 1, 1)
        love.graphics.print(btn, x + 7, fy + 17)
        love.graphics.setColor(0.7, 0.7, 0.75, 1)
        love.graphics.print(text, x + 25, fy + 17)
        return x + 25 + font_sub:getWidth(text) + 16
    end

    local gx = pad_x
    gx = draw_glyph("A", "Change", gx)
    gx = draw_glyph("B", "Back", gx)
    gx = draw_glyph("Y", "Default", gx)

    love.graphics.setColor(0.4, 0.4, 0.45, 1)
    love.graphics.setFont(font_sub)
    love.graphics.print("Tesla 1.3.3", ow - 78, fy + 17)
end

function love.keypressed(key)
    if key == "up" then
        overlay.selected_idx = math.max(1, overlay.selected_idx - 1)
    elseif key == "down" then
        overlay.selected_idx = math.min(#flat_items, overlay.selected_idx + 1)
    elseif key == "return" or key == "space" or key == "a" then
        local entry = flat_items[overlay.selected_idx]
        if entry.item.type == "cycle" then
            entry.item.current = (entry.item.current % #entry.item.options) + 1
        elseif entry.item.type == "toggle" then
            entry.item.state = not entry.item.state
        end
    elseif key == "escape" or key == "b" then
        love.event.quit()
    end
end

function love.gamepadpressed(joystick, button)
    if button == "dpup" then
        overlay.selected_idx = math.max(1, overlay.selected_idx - 1)
    elseif button == "dpdown" then
        overlay.selected_idx = math.min(#flat_items, overlay.selected_idx + 1)
    elseif button == "a" then
        local entry = flat_items[overlay.selected_idx]
        if entry.item.type == "cycle" then
            entry.item.current = (entry.item.current % #entry.item.options) + 1
        elseif entry.item.type == "toggle" then
            entry.item.state = not entry.item.state
        end
    elseif button == "b" then
        love.event.quit()
    end
end
