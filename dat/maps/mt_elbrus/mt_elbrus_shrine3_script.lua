-- Set the namespace according to the map name.
local ns = {};
setmetatable(ns, {__index = _G});
mt_elbrus_shrine3_script = ns;
setfenv(1, ns);

-- The map name, subname and location image
map_name = "Mt. Elbrus Shrine"
map_image_filename = "img/menus/locations/mountain_shrine.png"
map_subname = ""

-- The music file used as default background music on this map.
-- Other musics will have to handled through scripting.
music_filename = "mus/mountain_shrine.ogg"

-- c++ objects instances
local Map = {};
local ObjectManager = {};
local DialogueManager = {};
local EventManager = {};
local Script = {};

-- the main character handler
local hero = {};

-- Name of the main sprite. Used to reload the good one at the end of dialogue events.
local main_sprite_name = "";

-- the main map loading code
function Load(m)

    Map = m;
    ObjectManager = Map.object_supervisor;
    DialogueManager = Map.dialogue_supervisor;
    EventManager = Map.event_supervisor;
    Script = Map:GetScriptSupervisor();

    Map.unlimited_stamina = false;

    _CreateCharacters();
    _CreateObjects();

    -- Set the camera focus on hero
    Map:SetCamera(hero);
    -- This is a dungeon map, we'll use the front battle member sprite as default sprite.
    Map.object_supervisor:SetPartyMemberVisibleSprite(hero);

    _CreateEvents();
    _CreateZones();

    -- Add a mediumly dark overlay
    Map:GetEffectSupervisor():EnableAmbientOverlay("img/ambient/dark.png", 0.0, 0.0, false);

    -- Preload sounds
    AudioManager:LoadSound("snd/stone_roll.wav", Map);
    AudioManager:LoadSound("snd/opening_sword_unsheathe.wav", Map);
    AudioManager:LoadSound("snd/battle_encounter_03.ogg", Map);
end

-- the map update function handles checks done on each game tick.
function Update()
    -- Check whether the character is in one of the zones
    _CheckZones();
end

-- Character creation
function _CreateCharacters()
    -- Default hero and position (from shrine main room)
    hero = CreateSprite(Map, "Bronann", 3.5, 35.5);
    hero:SetDirection(vt_map.MapMode.EAST);
    hero:SetMovementSpeed(vt_map.MapMode.NORMAL_SPEED);

    -- Load previous save point data
    if (GlobalManager:GetPreviousLocation() == "from_shrine_treasure_room") then
        hero:SetDirection(vt_map.MapMode.SOUTH);
        hero:SetPosition(10.0, 10.0);
    end

    Map:AddGroundObject(hero);
end

local fence1 = {};
local fence2 = {};
local spikes = {};

local spike_wall = nil;

function _CreateObjects()
    local object = {}
    local npc = {}
    local dialogue = {}
    local text = {}
    local event = {}

    _add_flame(11.5, 6);
    _add_flame(27.5, 6);
    _add_flame(1.5, 31);

    fence1 = CreateObject(Map, "Stone Fence1", 5, 34);
    Map:AddGroundObject(fence1);
    fence2 = CreateObject(Map, "Stone Fence1", 5, 40);
    Map:AddGroundObject(fence2);

    spikes[1] = CreateObject(Map, "Spikes1", 17, 12);
    Map:AddGroundObject(spikes[1]);
    spikes[2] = CreateObject(Map, "Spikes1", 19, 12);
    Map:AddGroundObject(spikes[2]);
    spikes[3] = CreateObject(Map, "Spikes1", 21, 12);
    Map:AddGroundObject(spikes[3]);
    if (GlobalManager:GetEventValue("story", "mt_shrine_treasure_trap_done") == 1) then
        for _, spike in pairs(spikes) do
            spike:SetVisible(false);
            spike:SetCollisionMask(vt_map.MapMode.NO_COLLISION);
        end
    end

    spike_wall = vt_map.PhysicalObject();
    spike_wall:SetPosition(19.0, 48.0);
    spike_wall:SetObjectID(Map.object_supervisor:GenerateObjectID());
    spike_wall:SetCollHalfWidth(15.0);
    spike_wall:SetCollHeight(5.0);
    spike_wall:SetImgHalfWidth(15.0);
    spike_wall:SetImgHeight(5.0);
    spike_wall:AddStillFrame("dat/maps/mt_elbrus/spike_wall.png");
    spike_wall:SetVisible(false);
    spike_wall:SetCollisionMask(vt_map.MapMode.NO_COLLISION);
    Map:AddGroundObject(spike_wall);
end

function _add_flame(x, y)
    local object = vt_map.SoundObject("snd/campfire.ogg", x, y, 5.0);
    if (object ~= nil) then Map:AddAmbientSoundObject(object) end;
    object = vt_map.SoundObject("snd/campfire.ogg", x + 18.0, y, 5.0);
    if (object ~= nil) then Map:AddAmbientSoundObject(object) end;

    object = CreateObject(Map, "Flame1", x, y);
    Map:AddGroundObject(object);

    Map:AddHalo("img/misc/lights/torch_light_mask2.lua", x, y + 3.0,
        vt_video.Color(0.85, 0.32, 0.0, 0.6));
    Map:AddHalo("img/misc/lights/sun_flare_light_main.lua", x, y + 2.0,
        vt_video.Color(0.99, 1.0, 0.27, 0.1));
end

-- Creates all events and sets up the entire event sequence chain
function _CreateEvents()
    local event = {};
    local dialogue = {};
    local text = {};

    event = vt_map.MapTransitionEvent("to mountain shrine main room", "dat/maps/mt_elbrus/mt_elbrus_shrine2_map.lua",
                                       "dat/maps/mt_elbrus/mt_elbrus_shrine2_script.lua", "from_shrine_trap_room");
    EventManager:RegisterEvent(event);

    event = vt_map.MapTransitionEvent("to mountain shrine treasure room", "dat/maps/mt_elbrus/mt_elbrus_shrineX_map.lua",
                                       "dat/maps/mt_elbrus/mt_elbrus_shrineX_script.lua", "from_shrine_trap_room");
    EventManager:RegisterEvent(event);

    event = vt_map.ScriptedEvent("Start trap", "trap_start", "trap_update");
    EventManager:RegisterEvent(event);

    event = vt_map.ScriptedEvent("Start trap end", "end_trap_start", "end_trap_update");
    EventManager:RegisterEvent(event);

    event = vt_map.ScriptedEvent("Make spike wall go up", "spike_wall_start", "spike_wall_update");
    EventManager:RegisterEvent(event);
    event = vt_map.ScriptedEvent("Make spike wall go down", "spike_wall_down_start", "spike_wall_down_update");
    event:AddEventLinkAtEnd("Start trap end");
    EventManager:RegisterEvent(event);
end

-- Tells which is the latest monster group the party has defeated.
local current_monster_id = -1;
-- Array of enemy zones, enabled one after another...
local roam_zones = {}

-- Sets common battle environment settings for enemy sprites
function _SetBattleEnvironment(enemy)
    enemy:SetBattleMusicTheme("mus/heroism-OGA-Edward-J-Blakeley.ogg");
    enemy:SetBattleBackground("img/backdrops/battle/mountain_shrine.png");
    enemy:AddBattleScript("dat/battles/mountain_shrine_battle_anim.lua");
    enemy:AddBattleScript("dat/maps/mt_elbrus/battles_in_trap_map_script.lua");
end

function _CreateEnemiesZones()
    local enemy = nil;

    roam_zones[1] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "slime");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(1);
    enemy:AddEnemy(1);
    enemy:AddEnemy(1);
    enemy:AddEnemy(1);
    enemy:AddEnemy(1);
    enemy:SetBoss(true);
    roam_zones[1]:AddEnemy(enemy, Map, 1);
    roam_zones[1]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[1]);
    
    --roam_zones[1]:SetEnabled(false); -- Not disabled since it's the first one.

    roam_zones[2] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "spider");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(6);
    enemy:AddEnemy(2);
    enemy:AddEnemy(2);
    enemy:AddEnemy(2);
    enemy:AddEnemy(1);
    enemy:SetBoss(true);
    roam_zones[2]:AddEnemy(enemy, Map, 1);
    roam_zones[2]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[2]);
    roam_zones[2]:SetEnabled(false); -- Disabled per default

    roam_zones[3] = vt_map.EnemyZone(10, 32, 15, 19);
    -- Some bats
    enemy = CreateEnemySprite(Map, "bat");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(6);
    enemy:AddEnemy(6);
    enemy:AddEnemy(6);
    enemy:AddEnemy(4);
    enemy:SetBoss(true);
    roam_zones[3]:AddEnemy(enemy, Map, 1);
    roam_zones[3]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[3]);
    roam_zones[3]:SetEnabled(false); -- Disabled per default

    roam_zones[4] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "snake");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(1);
    enemy:AddEnemy(4);
    enemy:AddEnemy(4);
    enemy:AddEnemy(4);
    enemy:AddEnemy(4);
    enemy:AddEnemy(4);
    enemy:AddEnemy(6);
    enemy:SetBoss(true);
    roam_zones[4]:AddEnemy(enemy, Map, 1);
    roam_zones[4]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[4]);
    roam_zones[4]:SetEnabled(false); -- Disabled per default    
    
    roam_zones[5] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "big slime");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(5, 812.0, 350.0);
    enemy:AddEnemy(5, 612.0, 450.0);
    enemy:SetBoss(true);
    roam_zones[5]:AddEnemy(enemy, Map, 1);
    roam_zones[5]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[5]);
    roam_zones[5]:SetEnabled(false); -- Disabled per default

    roam_zones[6] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "Eyeball");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(12);
    enemy:AddEnemy(12);
    enemy:AddEnemy(12);
    enemy:AddEnemy(12);
    enemy:AddEnemy(12);
    enemy:SetBoss(true);
    roam_zones[6]:AddEnemy(enemy, Map, 1);
    roam_zones[6]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[6]);
    roam_zones[6]:SetEnabled(false); -- Disabled per default

    roam_zones[7] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "Beetle");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(16);
    enemy:AddEnemy(15);
    enemy:AddEnemy(15);
    enemy:AddEnemy(16);
    enemy:AddEnemy(15);
    enemy:AddEnemy(15);
    enemy:AddEnemy(17);
    enemy:SetBoss(true);
    roam_zones[7]:AddEnemy(enemy, Map, 1);
    roam_zones[7]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[7]);
    roam_zones[7]:SetEnabled(false); -- Disabled per default

    roam_zones[8] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "Skeleton");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(16);
    enemy:AddEnemy(19);
    enemy:AddEnemy(15);
    enemy:AddEnemy(19);
    enemy:AddEnemy(19);
    enemy:AddEnemy(19);
    enemy:AddEnemy(19);
    enemy:AddEnemy(17);
    enemy:SetBoss(true);
    roam_zones[8]:AddEnemy(enemy, Map, 1);
    roam_zones[8]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[8]);
    roam_zones[8]:SetEnabled(false); -- Disabled per default

    roam_zones[9] = vt_map.EnemyZone(10, 32, 15, 19);
    enemy = CreateEnemySprite(Map, "Skeleton");
    _SetBattleEnvironment(enemy);
    enemy:NewEnemyParty();
    enemy:AddEnemy(16);
    enemy:AddEnemy(19);
    enemy:AddEnemy(15);
    enemy:AddEnemy(19);
    enemy:AddEnemy(20);
    enemy:AddEnemy(19);
    enemy:AddEnemy(19);
    enemy:AddEnemy(17);
    enemy:SetBoss(true);
    roam_zones[9]:AddEnemy(enemy, Map, 1);
    roam_zones[9]:SetSpawnsLeft(1); -- This monster shall spawn only one time.
    Map:AddZone(roam_zones[9]);
    roam_zones[9]:SetEnabled(false); -- Disabled per default
end

local monsters_started = false;

function _CheckEnemiesState()

    if (current_monster_id >= 10) then
        return;
    end

    if (roam_zones[current_monster_id] == nil) then
        return;
    end

    if (roam_zones[current_monster_id]:GetSpawnsLeft() == 0) then
        -- Change the active zone.
        roam_zones[current_monster_id]:SetEnabled(false);
        current_monster_id = current_monster_id + 1;
    
        if (roam_zones[current_monster_id] ~= nil) then
            roam_zones[current_monster_id]:SetEnabled(true);
        end
    end

    if (current_monster_id >= 10) then
        -- Won!!
        -- Deactivate trap check
        monsters_started = false;

        -- Stop the death wall
        EventManager:TerminateEvents("Make spike wall go up", false);
        EventManager:StartEvent("Make spike wall go down");
        return;
    end
end

-- zones
local to_shrine_main_room_zone = {};
local to_shrine_treasure_room_zone = {};
local trap_zone = {};

-- Create the different map zones triggering events
function _CreateZones()

    -- N.B.: left, right, top, bottom
    to_shrine_main_room_zone = vt_map.CameraZone(0, 2, 34, 38);
    Map:AddZone(to_shrine_main_room_zone);

    to_shrine_treasure_room_zone = vt_map.CameraZone(18, 20, 9, 10);
    Map:AddZone(to_shrine_treasure_room_zone);

    trap_zone = vt_map.CameraZone(10, 34, 10, 44);
    Map:AddZone(trap_zone);
end

local trap_started = false;
local caught_by_trap = false;

-- Check whether the active camera has entered a zone. To be called within Update()
function _CheckZones()
    if (monsters_started == true) then
        if (roam_zones[current_monster_id] ~= nil) then
            if (roam_zones[current_monster_id]:GetSpawnsLeft() == 0) then
                _CheckEnemiesState();
            end
        end
    elseif (to_shrine_main_room_zone:IsCameraEntering() == true) then
        hero:SetDirection(vt_map.MapMode.WEST);
        EventManager:StartEvent("to mountain shrine main room");
    elseif (to_shrine_treasure_room_zone:IsCameraEntering() == true) then
        hero:SetDirection(vt_map.MapMode.NORTH);
        --EventManager:StartEvent("to mountain shrine treasure room");
    elseif (trap_started == false and trap_zone:IsCameraEntering() == true) then
        if (GlobalManager:GetEventValue("story", "mt_shrine_treasure_trap_done") == 0) then
            EventManager:StartEvent("Start trap");
            EventManager:StartEvent("Make spike wall go up", 2000);
        end
    end

    -- Check whether the hero is dead because of trap.
    if (caught_by_trap == true and Map:CurrentState() == vt_map.MapMode.STATE_EXPLORE) then
        Map:PushState(vt_map.MapMode.STATE_SCENE);
        hero:SetCustomAnimation("hurt", 0);
        hero:SetMoving(false);
        -- Trigger party damage.
        local hp_change = math.random(40, 60);
        _TriggerPartyDamage(hp_change);

        EventManager:StartEvent("to mountain shrine main room");
        AudioManager:PlaySound("snd/battle_encounter_03.ogg");
    end
end

-- Trigger damages on the characters present on the battle front.
function _TriggerPartyDamage(damage)
    -- Adds an effect on map
    local x_pos = Map:GetScreenXCoordinate(hero:GetXPosition());
    local y_pos = Map:GetScreenYCoordinate(hero:GetYPosition());
    local map_indicator = Map:GetIndicatorSupervisor();
    map_indicator:AddDamageIndicator(x_pos, y_pos, damage, vt_video.TextStyle("text22", vt_video.Color(1.0, 0.0, 0.0, 0.9)), true);

    local index = 0;
    for index = 0, 3 do
        local char = GlobalManager:GetCharacter(index);
        if (char ~= nil) then
            -- Do not kill characters. though
            local hp_damage = damage;
            if (hp_damage >= char:GetHitPoints()) then
                hp_damage = char:GetHitPoints() - 1;
            end
            if (hp_damage > 0) then
                char:SubtractHitPoints(hp_damage);
            end
        end
    end
end

local fence1_x = 0;
local fence1_y = 0;
local fence2_x = 0;
local fence2_y = 0;
local moving_step = 1;

-- Map Custom functions
-- Used through scripted events
map_functions = {
    trap_start = function()
        trap_started = true;
        fence1_x = 5;
        fence1_y = 34;
        fence2_x = 5;
        fence2_y = 40;
        moving_step = 1;
        current_monster_id = -1;
        monsters_started = false;
        AudioManager:PlaySound("snd/stone_roll.wav");
    end,

    trap_update = function()
        local update_time = SystemManager:GetUpdateTime();
        
        -- Moving vertically first
        if (moving_step == 1) then
            if (fence1_y < 36) then
                fence1_y = fence1_y + 0.010 * update_time;
            end
            if (fence2_y > 38) then
                fence2_y = fence2_y - 0.010 * update_time;
            end
            if (fence1_y >= 36 and fence2_y <= 38) then
                AudioManager:PlaySound("snd/stone_roll.wav");
                moving_step = 2;
            end
        -- Moving horizontally
        elseif(moving_step == 2) then
            if (fence1_x > 3) then
                fence1_x = fence1_x - 0.010 * update_time;
            end
            if (fence2_x > 3) then
                fence2_x = fence2_x - 0.010 * update_time;
            end
            if (fence1_x <= 3 and fence2_x <= 3) then
                moving_step = 3;
            end
        end

        fence1:SetPosition(fence1_x, fence1_y);
        fence2:SetPosition(fence2_x, fence2_y);
        if (moving_step < 3) then
            return false;
        end

        -- Spawn the first monster
        current_monster_id = 1;
        monsters_started = true;
        _CreateEnemiesZones();

        return true;
    end,

    end_trap_start = function()
        fence1_x = 3;
        fence1_y = 36;
        fence2_x = 3;
        fence2_y = 38;
        moving_step = 1;
        AudioManager:PlaySound("snd/stone_roll.wav");
    end,
    
    end_trap_update = function()
        local update_time = SystemManager:GetUpdateTime();
        
        -- Moving horizontally first
        if(moving_step == 1) then
            if (fence1_x < 5) then
                fence1_x = fence1_x + 0.010 * update_time;
            end
            if (fence2_x < 5) then
                fence2_x = fence2_x + 0.010 * update_time;
            end
            if (fence1_x >= 5 and fence2_x >= 5) then
                moving_step = 2;
                AudioManager:PlaySound("snd/stone_roll.wav");
            end
        -- Moving vertically
        elseif (moving_step == 2) then
            if (fence1_y > 34) then
                fence1_y = fence1_y - 0.010 * update_time;
            end
            if (fence2_y < 40) then
                fence2_y = fence2_y + 0.010 * update_time;
            end
            if (fence1_y <= 34 and fence2_y >= 40) then
                moving_step = 3;
            end
        end

        fence1:SetPosition(fence1_x, fence1_y);
        fence2:SetPosition(fence2_x, fence2_y);
        if (moving_step < 3) then
            return false;
        end

        -- Remove spikes
        AudioManager:PlaySound("snd/opening_sword_unsheathe.wav");
        for my_index, spike in pairs(spikes) do
            spike:SetVisible(false);
            spike:SetCollisionMask(vt_map.MapMode.NO_COLLISION);
        end

        return true;
    end,

    spike_wall_start = function()
        spike_wall:SetPosition(19, 50);
        spike_wall:SetVisible(true);
        spike_wall:SetCollisionMask(vt_map.MapMode.ALL_COLLISION);
        caught_by_trap = false;

        -- Start the tremor
        Map:GetEffectSupervisor():ShakeScreen(0.6, 0, vt_mode_manager.EffectSupervisor.SHAKE_FALLOFF_NONE); -- 0 means infinite time.
        -- TODO: Play an environmental sound of continuous tremor, following the wall...
    end,
    
    spike_wall_update = function()
        local update_time = SystemManager:GetUpdateTime();
        local spike_y = spike_wall:GetYPosition();
        spike_y = spike_y - 0.0007 * update_time;
        spike_wall:SetYPosition(spike_y);

        -- TODO: Check collision with props and push them...

        -- Check collision with camera
        if (spike_wall:IsCollidingWith(hero) == true) then
            caught_by_trap = true;
            return true;
        end

        -- Check collision with camera
        if (spike_wall:GetYPosition() > 14) then
            return false;
        end

        caught_by_trap = true;

        return true;
    end,

    spike_wall_down_start = function()
        -- Stops the tremor
        Map:GetEffectSupervisor():StopShaking();

        -- Set event as done
        GlobalManager:SetEventValue("story", "mt_shrine_treasure_trap_done", 1);
    end,
    
    spike_wall_down_update = function()
        local update_time = SystemManager:GetUpdateTime();
        local spike_y = spike_wall:GetYPosition();
        spike_y = spike_y + 0.002 * update_time;
        spike_wall:SetYPosition(spike_y);

        -- Check collision with camera
        if (spike_wall:GetYPosition() < 50) then
            return false;
        end

        spike_wall:SetVisible(false);
        spike_wall:SetCollisionMask(vt_map.MapMode.NO_COLLISION);
        
        return true;
    end,
}
