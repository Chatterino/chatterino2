local tests = {
    from_only_url = function()
        local img = c2.Image.from_url("https://chatterino.com/foo/1x")
        assert(img.url == "https://chatterino.com/foo/1x")
        assert(not img.animated)
        assert(not img.is_loaded)
        assert(not img.is_empty)
        assert(img.width == 16) -- default width
        assert(img.height == 16) -- default height
        assert(img.scale == 1) -- default scale
        assert(img.size.width == img.width)
        assert(img.size.height == img.height)
    end,
    from_url_and_scale = function()
        local img = c2.Image.from_url("https://chatterino.com/foo/2x", 0.5)
        assert(img.url == "https://chatterino.com/foo/2x")
        assert(not img.animated)
        assert(not img.is_loaded)
        assert(not img.is_empty)
        assert(img.width == 4) -- default width * scale * scale (once in the constructor and once in the width accessor)
        assert(img.height == 4) -- default height * scale * scale
        assert(img.scale == 0.5)
        assert(img.size.width == img.width)
        assert(img.size.height == img.height)
    end,
    from_url_scale_exp_size = function()
        local img = c2.Image.from_url("https://chatterino.com/bar/2x", 0.5, { 64, 32 })
        assert(img.url == "https://chatterino.com/bar/2x")
        assert(not img.animated)
        assert(not img.is_loaded)
        assert(not img.is_empty)
        assert(img.width == 32) -- width * scale
        assert(img.height == 16) -- height * scale
        assert(img.scale == 0.5)
        assert(img.size.width == img.width)
        assert(img.size.height == img.height)
    end,
    from_invalid_url = function()
        local ok, res = pcall(c2.Image.from_url, "ws://foo")
        assert(not ok and res == "Invalid URL")
    end,
    empty = function()
        local img = c2.Image.empty()
        assert(img.url == "")
        assert(not img.animated)
        assert(not img.is_loaded)
        assert(img.is_empty)
        assert(img.width == 0)
        assert(img.height == 0)
        assert(img.scale == 1)
        assert(img.size.width == img.width)
        assert(img.size.height == img.height)
    end,
    equality = function()
        local imageA = c2.Image.from_url("https://chatterino.com/image-equality/A")
        local imageB = c2.Image.from_url("https://chatterino.com/image-equality/B")
        local imageC = c2.Image.from_url("https://chatterino.com/image-equality/C")
        local imageC2 = c2.Image.from_url("https://chatterino.com/image-equality/C") -- should be the same as C
        local imageA2 = c2.Image.from_url("https://chatterino.com/image-equality/A", 0.5) -- should be the same as A

        assert(imageA == imageA)
        assert(imageA ~= imageB)
        assert(imageA ~= imageC)
        assert(imageA ~= imageC2)
        assert(imageA == imageA2)

        assert(imageB ~= imageA)
        assert(imageB == imageB)
        assert(imageB ~= imageC)
        assert(imageB ~= imageC2)
        assert(imageB ~= imageA2)

        assert(imageC ~= imageA)
        assert(imageC ~= imageB)
        assert(imageC == imageC)
        assert(imageC == imageC2)
        assert(imageC ~= imageA2)
    end,
}

for name, fn in pairs(tests) do
    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
