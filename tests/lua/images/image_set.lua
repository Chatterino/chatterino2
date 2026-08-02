local imgA = c2.Image.from_url("https://chatterino.com/test-image-set/A")
local imgB = c2.Image.from_url("https://chatterino.com/test-image-set/B")
local imgC = c2.Image.from_url("https://chatterino.com/test-image-set/C")
local imgD = c2.Image.from_url("https://chatterino.com/test-image-set/D")

local tests = {
    no_image = function()
        local set = c2.ImageSet.new()
        assert(set.image1.is_empty)
        assert(set.image2.is_empty)
        assert(set.image3.is_empty)
    end,
    one_image = function()
        local set = c2.ImageSet.new(imgA)
        assert(set.image1 == imgA)
        assert(set.image2.is_empty)
        assert(set.image3.is_empty)
    end,
    two_images = function()
        local set = c2.ImageSet.new(imgA, imgB)
        assert(set.image1 == imgA)
        assert(set.image2 == imgB)
        assert(set.image3.is_empty)
    end,
    three_images = function()
        local set = c2.ImageSet.new(imgA, imgB, imgC)
        assert(set.image1 == imgA)
        assert(set.image2 == imgB)
        assert(set.image3 == imgC)
    end,
    some_strings = function()
        local set = c2.ImageSet.new("https://str1", imgB, "http://str2")
        assert(set.image1.url == "https://str1")
        assert(set.image2 == imgB)
        assert(set.image3.url == "http://str2")
    end,
    one_missing = function()
        local ok = pcall(c2.ImageSet.new, nil, imgB, imgC)
        assert(not ok)
    end,
    setters = function()
        local set = c2.ImageSet.new(imgA, imgB)
        assert(set.image1 == imgA)
        assert(set.image2 == imgB)
        assert(set.image3 == c2.Image.empty())
        set.image3 = imgC
        assert(set.image3 == imgC)
        set.image2 = imgD
        assert(set.image2 == imgD)
    end,
    bad_setters = function()
        local set = c2.ImageSet.new(imgA, imgB)
        local ok = pcall(function()
            set.image2 = nil
        end)
        assert(not ok)
        ok = pcall(function()
            set.image2 = "https://foo.bar"
        end)
        assert(not ok)
    end,
}

for name, fn in pairs(tests) do
    local ok, res = pcall(fn)
    if not ok then
        error(name .. " failed: " .. res)
    end
end
