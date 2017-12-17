local log = require 'dimonnetalk.log'
local fw = require 'dimonnetalk.framework'
local json = require 'json'
local utf8 = require 'utf8'

local ovandriyanov_chat_id = 165888502

function dump(x, level)
    level = level or 0
    local indent = level * 2
    if type(x) == 'table' then
        local ret = '{\n'
        indent = indent + 2
        for a, b in pairs(x) do
            local item = tostring(a) .. ': ' .. dump(b, level + 1) .. '\n'
            ret = ret .. string.rep(' ', indent) ..  item
        end
        indent = indent - 2
        return ret .. string.rep(' ', indent) .. '}'
    else
        return tostring(x)
    end
end

function check_update(up)
    log.debug(dump(up))
    if not up.message then return false end
    -- return up.message.chat.id == ovandriyanov_chat_id
    return true
end

local cyrillic_to_latin = {
    ['а'] = 'a',
    ['б'] = 'b',
    ['в'] = 'v',
    ['г'] = 'g',
    ['д'] = 'd',
    ['е'] = 'e',
    ['ё'] = 'e',
    ['ж'] = 'zh',
    ['з'] = 'z',
    ['и'] = 'i',
    ['й'] = 'j',
    ['к'] = 'k',
    ['л'] = 'l',
    ['м'] = 'm',
    ['н'] = 'n',
    ['о'] = 'o',
    ['п'] = 'p',
    ['р'] = 'r',
    ['с'] = 's',
    ['т'] = 't',
    ['у'] = 'u',
    ['ф'] = 'f',
    ['х'] = 'h',
    ['ц'] = 'ts',
    ['ч'] = 'ch',
    ['ш'] = 'sh',
    ['щ'] = 'sch',
    ['ъ'] = '',
    ['ы'] = 'y',
    ['ь'] = '',
    ['э'] = 'e',
    ['ю'] = 'yu',
    ['я'] = 'ya'
}

function latinize(str)
    local ret = ''
    for pos, codepoint in utf8.codes(str) do
        local c = utf8.char(codepoint)
        ret = ret .. (cyrillic_to_latin[c] or c)
    end
    return ret
end

while true do ::again::
    local up = json.decode(fw.get_update())
    if not check_update(up) then goto again end
    fw.call_api('sendMessage', json.encode({chat_id = up.message.chat.id, text = latinize(up.message.text)}))
end
