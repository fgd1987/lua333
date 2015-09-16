module('FrameServer', package.seeall)

--[[

update
fixed_update
destory

生命线
-------->update-------->fixed_update--------->stopping------------>destory

--]]

--每秒多少帧
FRAME_PER_SEC = 60
--每帧多少毫秒
MSEC_PER_FRAME = math.floor(1000/60)
--固定帧(单位：毫秒）
FIXED_UPDATE = 3000

running = true

mod_table = {}

function main()

end

--设置模块
function set_mod_table(table)
    mod_table = table
end

--停止
function stop()
    running = false
end

function update()
    --记录误差
    local diff = 0
    local loop_time = 0
    while true then
        _G['TIME'] = Date.time()
        _G['MSEC_NOW'] = Date.mtime()
        --固定帧数
        for _, mod in pairs(mod_table) do
            local func = mod['update']
            if func then
                func(MSEC_NOW)
            end
        end
        --固定时间
        if MSEC_NOW - last_loop_time > FIXED_UPDATE then
            for _, mod in pairs(mod_table) do
                local func = mod['fixed_update']
                if func then
                    func(MSEC_NOW)
                end
            end
            last_loop_time = MSEC_NOW
        end
        --是否要休息一下
        local cost = Date.mtime() - MSEC_NOW
        diff = diff + (MSEC_PER_FRAME - cost)
        if diff > 0 then
            Date.msleep(diff)
            diff = 0
        end
        --是否要退出
        if not running then
            local allstop = true
            --通知模块暂停，做一些处理
            for _, mod in pairs(mod_table) do
                local func = mod['stopping']
                if func then
                    if not func() then
                        allstop = false
                        Log.log(TAG, 'waiting mod(%s) to stop', mod.MOD_NAME)
                    end
                end
            end
            --有部分模块末清理完成
            if allstop then
                break
            end
        end
    end
    --最后的清理工作
    for _, mod in pairs(mod_table) do
        local func = mod['destory']
        if func then
            local result = func()
        end
    end
end

