<script setup>
import { ref, onUnmounted, computed } from 'vue'
import SettingsTab from './components/SettingsTab.vue'
import CalibrationTab from './components/CalibrationTab.vue'
import { 
  darkTheme, 
  NConfigProvider, 
  NGlobalStyle, 
  NLayout, 
  NLayoutHeader, 
  NLayoutContent, 
  NLayoutFooter, 
  NCard, 
  NButton, 
  NSpace, 
  NH1, 
  NText, 
  NGradientText,
  NSlider,
  NTable,
  NSelect,
  NDivider,
  NTabs,
  NTabPane,
  NPopconfirm,
  NInputNumber,
  NGrid,
  NGridItem,
  NSwitch,
  NButtonGroup,
  NRadioGroup,
  NRadioButton
} from 'naive-ui'

const isConnected = ref(false)
const isSyncing = ref(false)
const syncStatus = ref('Disconnected')
const syncSuccess = ref(false)

const isBigBoard = ref(true)
const enableBuzzer = ref(true)
const displayMode = ref(0)
const slots = ref(0)
const swapTeams = ref(false)
const serveBypass = ref(-1)

const brightness = ref(5)
const volume = ref(4)
const bleName = ref('Netscore')
const battery = ref({ main: 0, device_1: 0, device_2: 0, min: null, max: null })
const lastResponse = ref('')
const matchHistory = ref([])
const maxScores = ref([])
const goldenPoint = ref(false)
const endlessMode = ref(false)

const groupCal = ref({ r: 100, g: 100, b: 100 })
const segmentA = ref(Array(10).fill(100))
const segmentB = ref(Array(10).fill(100))
const miscA = ref(Array(32).fill(100))
const miscB = ref(Array(32).fill(100))
const hasUnsavedSettings = ref(false)

const savedMatchCount = ref(0)
const savedMatchIndex = ref(0)

const shortcuts = ref({
  up: { en: false, sp: 0, m: 0, max: 0, pt: 0, pd: 0 },
  down: { en: false, sp: 0, m: 0, max: 0, pt: 0, pd: 0 },
  center: { en: false, sp: 0, m: 0, max: 0, pt: 0, pd: 0 }
})

const activeTab = ref('live')

const startGameSport = ref(0)
const startGameModeVolley = ref(1)
const startGameMaxScoreIndex = ref(3)
const startGameModePadel = ref(1)
const startPadelGameType = ref(0)
const startPadelDeuceType = ref(0)

const sportOptions = [
  { label: 'Volleyball', value: 0 },
  { label: 'Padel', value: 1 }
]

const modeVolleyOptions = [
  { label: 'Practice', value: 0 },
  { label: 'Normal', value: 1 }
]

const maxScoreOptionsVolley = computed(() => {
  if (startGameModeVolley.value === 0) {
    if (startGameMaxScoreIndex.value > 1) startGameMaxScoreIndex.value = 1;
    return [
      { label: '12 Points', value: 0 },
      { label: '15 Points', value: 1 }
    ]
  } else {
    return [
      { label: '12 Points', value: 0 },
      { label: '15 Points', value: 1 },
      { label: '21 Points', value: 2 },
      { label: '25 Points', value: 3 }
    ]
  }
})

const modePadelOptions = [
  { label: 'Normal', value: 1 },
  { label: 'Tournament', value: 2 }
]

const padelGameTypeOptions = [
  { label: 'Sets Diff 2', value: 0 },
  { label: 'Tiebreak', value: 1 }
]

const padelDeuceTypeOptions = [
  { label: 'Golden Point', value: 0 },
  { label: 'Advantages', value: 1 }
]

const Screen = {
  BOOT_SCR: 0,
  BOOT_2_SCR: 1,
  BOOT_3_SCR: 2,
  BOOT_4_SCR: 3,
  BOOT_5_SCR: 4,
  MENU_SCR: 5,
  MENU_TRANSITION_SCR: 6,
  SPORT_SCR: 7,
  SET_SPORT_MODE_SCR: 8,
  SET_MAX_SCORE_SCR: 9,
  SET_PADEL_GAME_TYPE_SCR: 10,
  SET_PADEL_DEUCE_TYPE_SCR: 11,
  PLAY_SERVE_SELECT_SCR: 12,
  PLAY_SCR: 13,
  PLAY_MENU_SCR: 14,
  PLAY_MENU_PAINEL_SCR: 15,
  PLAY_WIN_SCR: 16,
  PRACTICE_TRANSITION_SCR: 17,
  CONNECTING_SCR: 18,
  BRILHO_SCR: 19,
  BRILHO_OVERLAY_SCR: 20,
  VOLUME_OVERLAY_SCR: 21,
  BATT_SCR: 22,
  CLOCK_SCR: 23,
  TEST_MENU_SCR: 24,
  TEST_COUNTER_SCR: 25,
  TEST_ALL_SCR: 26,
  TEST_BOMB_SCR: 27,
  OFF_SCR: 28,
  OFF_2_SCR: 29,
  SLEEP_SCR: 30,
  SLEEP_2_SCR: 31,
  OOPS_SCR: 32
}

const screenCategories = [
  {
    name: 'System',
    screens: [
      { label: 'Boot', value: Screen.BOOT_SCR },
      { label: 'Connecting', value: Screen.CONNECTING_SCR },
      { label: 'Battery', value: Screen.BATT_SCR },
      { label: 'Clock', value: Screen.CLOCK_SCR },
      { label: 'Brightness', value: Screen.BRILHO_SCR },
      { label: 'Power Off', value: Screen.OFF_SCR },
      { label: 'Sleep', value: Screen.SLEEP_SCR },
      { label: 'Oops', value: Screen.OOPS_SCR },
    ]
  },
  {
    name: 'Setup',
    screens: [
      { label: 'Menu', value: Screen.MENU_SCR },
      { label: 'Sport Select', value: Screen.SPORT_SCR },
      { label: 'Sport Mode', value: Screen.SET_SPORT_MODE_SCR },
      { label: 'Max Score', value: Screen.SET_MAX_SCORE_SCR },
      { label: 'Padel Game', value: Screen.SET_PADEL_GAME_TYPE_SCR },
      { label: 'Padel Deuce', value: Screen.SET_PADEL_DEUCE_TYPE_SCR },
    ]
  },
  {
    name: 'Match',
    screens: [
      { label: 'Serve Select', value: Screen.PLAY_SERVE_SELECT_SCR },
      { label: 'Play Match', value: Screen.PLAY_SCR },
      { label: 'Play Menu', value: Screen.PLAY_MENU_SCR },
      { label: 'Play Menu Side', value: Screen.PLAY_MENU_PAINEL_SCR },
      { label: 'Set Win', value: Screen.PLAY_WIN_SCR },
      { label: 'Practice Trans', value: Screen.PRACTICE_TRANSITION_SCR },
    ]
  },
  {
    name: 'Test Modes',
    screens: [
      { label: 'Test Menu', value: Screen.TEST_MENU_SCR },
      { label: 'Test Counter', value: Screen.TEST_COUNTER_SCR },
      { label: 'Test All', value: Screen.TEST_ALL_SCR },
      { label: 'Test Bomb', value: Screen.TEST_BOMB_SCR },
    ]
  }
]

const Device = {
  NONE: 0,
  DEVICE_1: 1,
  DEVICE_2: 2,
  ALL: 3
}

const ButtonEvent = {
  BLE_BTN_PRESS: 0,
  BLE_BTN_HOLD: 1,
  BLE_BTN_A_PRESS: 2,
  BLE_BTN_B_PRESS: 3,
  BLE_BTN_A_HOLD: 4,
  BLE_BTN_B_HOLD: 5,
  BLE_BTN_A_PRESS_BOTH: 6,
  BLE_BTN_B_PRESS_BOTH: 7,
  ITAG_PRESS: 8,
  ITAG_DOUBLE_PRESS: 9,
  BUTTON_CENTER_PRESS: 10,
  BUTTON_CENTER_DOUBLE_PRESS: 11,
  BUTTON_CENTER_HOLD: 12,
  BUTTON_CENTER_REPEAT: 13,
  BUTTON_CENTER_RELEASE: 14,
  BUTTON_UP_PRESS: 15,
  BUTTON_UP_DOUBLE_PRESS: 16,
  BUTTON_UP_HOLD: 17,
  BUTTON_UP_REPEAT: 18,
  BUTTON_UP_RELEASE: 19,
  BUTTON_DOWN_PRESS: 20,
  BUTTON_DOWN_DOUBLE_PRESS: 21,
  BUTTON_DOWN_HOLD: 22,
  BUTTON_DOWN_REPEAT: 23,
  BUTTON_DOWN_RELEASE: 24,
  BUTTON_POWER_PRESS: 25,
  BUTTON_POWER_DOUBLE_PRESS: 26,
  BUTTON_POWER_HOLD: 27
}

const selectedDevice = ref(1);
let pressTimer = null;
let isLongPress = false;

let doubleClickTimer = null;
let pendingPressEvt = null;
let pendingDev = null;

const onPointerDown = (dev, holdEvt) => {
  isLongPress = false;
  if (pressTimer) clearTimeout(pressTimer);
  pressTimer = setTimeout(() => {
    isLongPress = true;
    sendCommand({ cmd: 'button', dev, evt: holdEvt })
  }, 500)
}

const onPointerUp = (dev, pressEvt, doublePressEvt = null) => {
  if (pressTimer) {
    clearTimeout(pressTimer)
    pressTimer = null
  }
  if (!isLongPress && pressEvt !== null) {
    if (doublePressEvt !== null) {
      if (doubleClickTimer && pendingPressEvt === pressEvt) {
        // Double click detected
        clearTimeout(doubleClickTimer);
        doubleClickTimer = null;
        pendingPressEvt = null;
        sendCommand({ cmd: 'button', dev, evt: doublePressEvt });
      } else {
        // First click
        pendingPressEvt = pressEvt;
        pendingDev = dev;
        doubleClickTimer = setTimeout(() => {
          sendCommand({ cmd: 'button', dev: pendingDev, evt: pendingPressEvt });
          doubleClickTimer = null;
          pendingPressEvt = null;
        }, 250); // 250ms window for double click
      }
    } else {
      sendCommand({ cmd: 'button', dev, evt: pressEvt })
    }
  }
  isLongPress = false;
}

const onPointerLeave = () => {
  if (pressTimer) {
    clearTimeout(pressTimer)
    pressTimer = null
  }
  isLongPress = true; // Prevent click on release outside
}

const isPadel = computed(() => currentSport.value === 1)

const currentSport = computed(() => {
  if (matchHistory.value.length === 0) return null;
  return matchHistory.value[matchHistory.value.length - 1].sport;
})

const currentMode = computed(() => {
  if (matchHistory.value.length === 0) return null;
  return matchHistory.value[matchHistory.value.length - 1].game_mode;
})

const filteredHistory = computed(() => {
  if (currentSport.value === null) return [];
  return matchHistory.value.filter(e => e.sport === currentSport.value);
})

const currentScore = computed(() => {
  let score = {
    home_points: 0,
    away_points: 0,
    home_sets: 0,
    away_sets: 0,
    set_points_home: Array(15).fill(0),
    set_points_away: Array(15).fill(0),
    mode: Array(15).fill(0)
  }
  
  if (isPadel.value) return score;
  
  const validEvents = []
  for (const ev of matchHistory.value) {
    if (ev.evt === 0) validEvents.push(ev)
    else if (ev.evt === 1) {
      if (ev.team === 2) {
        for (let i = validEvents.length - 1; i >= 0; i--) {
          if (validEvents[i].sport === ev.sport && validEvents[i].game_mode === ev.game_mode) {
            validEvents.splice(i, 1)
            break
          }
        }
      } else {
        for (let i = validEvents.length - 1; i >= 0; i--) {
          if (validEvents[i].team === ev.team && validEvents[i].sport === ev.sport && validEvents[i].game_mode === ev.game_mode) {
            validEvents.splice(i, 1)
            break
          }
        }
      }
    }
  }

  for (const ev of validEvents) {
    if (ev.sport !== currentSport.value) continue; // Only process the current sport

    if (ev.team === 0) score.home_points++
    else score.away_points++

    let set_idx = score.home_sets + score.away_sets
    let current_max = (set_idx < 15 && maxScores.value.length > set_idx) ? maxScores.value[set_idx] : 21

    let cur_p = ev.team === 0 ? score.home_points : score.away_points
    let oth_p = ev.team === 0 ? score.away_points : score.home_points

    let set_won = (cur_p >= current_max && (cur_p - oth_p >= 2))

    if (set_idx < 15) {
      score.set_points_home[set_idx] = score.home_points
      score.set_points_away[set_idx] = score.away_points
      score.mode[set_idx] = ev.game_mode
    }

    if (set_won) {
      if (ev.team === 0) score.home_sets++
      else score.away_sets++
      score.home_points = 0
      score.away_points = 0
    }
  }
  return score
})

const currentPadelScore = computed(() => {
  let state = {
    home_points: 0,
    away_points: 0,
    home_games: 0,
    away_games: 0,
    home_sets: 0,
    away_sets: 0,
    set_games_home: Array(15).fill(0),
    set_games_away: Array(15).fill(0),
    tiebreak: false
  }

  if (!isPadel.value) return state;

  const validEvents = []
  for (const ev of filteredHistory.value) {
    if (ev.evt === 0) validEvents.push(ev)
    else if (ev.evt === 1) {
      if (ev.team === 2) {
        for (let i = validEvents.length - 1; i >= 0; i--) {
          if (validEvents[i].sport === ev.sport && validEvents[i].game_mode === ev.game_mode) {
            validEvents.splice(i, 1)
            break
          }
        }
      } else {
        for (let i = validEvents.length - 1; i >= 0; i--) {
          if (validEvents[i].team === ev.team && validEvents[i].sport === ev.sport && validEvents[i].game_mode === ev.game_mode) {
            validEvents.splice(i, 1)
            break
          }
        }
      }
    }
  }

  const game_win = (team) => {
    if (team === 0) state.home_games++
    else state.away_games++
    
    state.home_points = 0
    state.away_points = 0

    let set_idx = state.home_sets + state.away_sets
    if (set_idx < 15) {
      state.set_games_home[set_idx] = state.home_games
      state.set_games_away[set_idx] = state.away_games
    }

    if (!endlessMode.value) {
      let cur_g = team === 0 ? state.home_games : state.away_games
      let oth_g = team === 0 ? state.away_games : state.home_games

      if ((cur_g >= 6 && (cur_g - oth_g > 1)) || state.tiebreak) {
        if (team === 0) state.home_sets++
        else state.away_sets++
        
        state.home_points = 0
        state.away_points = 0
        state.home_games = 0
        state.away_games = 0
        state.tiebreak = false
      } else if (cur_g === 6 && oth_g === 6) {
        state.tiebreak = true
      }
    }
  }

  for (const ev of validEvents) {
    let team = ev.team

    if (state.tiebreak) {
      if (team === 0) state.home_points++
      else state.away_points++

      let cur_p = team === 0 ? state.home_points : state.away_points
      let oth_p = team === 0 ? state.away_points : state.home_points

      if (cur_p >= 7 && (cur_p - oth_p >= 2)) {
        game_win(team)
      }
    } else {
      let current_p = team === 0 ? state.home_points : state.away_points
      let other_p = team === 0 ? state.away_points : state.home_points

      if (current_p === 0) current_p = 15
      else if (current_p === 15) current_p = 30
      else if (current_p === 30) current_p = 40
      else if (current_p === 40) {
        if (goldenPoint.value) {
          game_win(team)
          continue
        } else {
          if (other_p === 40) current_p = 60 // ADV
          else if (other_p === 60) {
            other_p = 40
          } else {
            game_win(team)
            continue
          }
        }
      } else if (current_p === 60) {
        game_win(team)
        continue
      }

      if (team === 0) {
        state.home_points = current_p
        state.away_points = other_p
      } else {
        state.away_points = current_p
        state.home_points = other_p
      }
    }
  }
  return state
})

const SPORTS = {
  0: 'Volleyball',
  1: 'Padel',
  2: 'Ping Pong',
  3: 'Tennis',
  4: 'Soccer',
  5: 'Basketball'
}
const MODES = {
  0: 'Practice',
  1: 'Normal'
}
const getSportName = (mode) => SPORTS[mode] || 'Unknown'
const getModeName = (mode) => MODES[mode] || 'Unknown'

// Nordic UART Service (NUS) UUIDs
const SERVICE_UUID = '6e400001-b5a3-f393-e0a9-e50e24dcca9e'
const RX_CHAR_UUID = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'
const TX_CHAR_UUID = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'

let bleServer = null;
let rxCharacteristic = null;
let txCharacteristic = null;

const connect = async () => {
  if (!navigator.bluetooth) {
    syncStatus.value = 'Web Bluetooth is not supported in this browser.'
    return
  }
  
  try {
    syncStatus.value = 'Requesting Bluetooth Device...'
    const device = await navigator.bluetooth.requestDevice({
      filters: [{ services: [SERVICE_UUID] }]
    })
    
    device.addEventListener('gattserverdisconnected', onDisconnected)

    syncStatus.value = 'Connecting...'
    bleServer = await device.gatt.connect()

    const service = await bleServer.getPrimaryService(SERVICE_UUID)
    rxCharacteristic = await service.getCharacteristic(RX_CHAR_UUID)
    txCharacteristic = await service.getCharacteristic(TX_CHAR_UUID)
    
    await txCharacteristic.startNotifications()
    txCharacteristic.addEventListener('characteristicvaluechanged', handleNotifications)

    isConnected.value = true
    syncStatus.value = 'Connected'
    
    // Request current settings
    await sendCommand({ cmd: 'getSettings' })
    
  } catch (error) {
    console.error(error)
    syncStatus.value = 'Connection failed: ' + error.message
    isConnected.value = false
  }
}

const saveShortcut = (btnIdx) => {
  let sc = shortcuts.value.up;
  if (btnIdx === 1) sc = shortcuts.value.down;
  if (btnIdx === 2) sc = shortcuts.value.center;
  
  sendCommand({
    cmd: 'setShortcut',
    btn: btnIdx,
    en: sc.en ? 1 : 0,
    sp: sc.sp,
    m: sc.m,
    max: sc.max,
    pt: sc.pt,
    pd: sc.pd
  });
}

const saveCurrentToShortcut = (btnIdx) => {
  let sc = shortcuts.value.up;
  if (btnIdx === 1) sc = shortcuts.value.down;
  if (btnIdx === 2) sc = shortcuts.value.center;

  sc.en = true;
  sc.sp = startGameSport.value;
  sc.m = startGameSport.value === 0 ? startGameModeVolley.value : startGameModePadel.value;
  sc.max = startGameMaxScoreIndex.value;
  sc.pt = startPadelGameType.value;
  sc.pd = startPadelDeuceType.value;

  saveShortcut(btnIdx);
}

const getShortcutDescription = (sc) => {
  if (!sc.en) return "Disabled";
  const sp = sportOptions.find(s => s.value === sc.sp)?.label || "Unknown Sport";
  let modeDesc = "";
  
  if (sc.sp === 0) { // Volleyball
    const mode = modeVolleyOptions.find(m => m.value === sc.m)?.label || "Unknown Mode";
    let pts = "";
    if (sc.m === 0) {
      pts = sc.max === 0 ? "12 Pts" : "15 Pts";
    } else {
      const ptsArr = ["12 Pts", "15 Pts", "21 Pts", "25 Pts"];
      pts = ptsArr[sc.max] || "Unknown Pts";
    }
    modeDesc = `${mode} | ${pts}`;
  } else if (sc.sp === 1) { // Padel
    const mode = modePadelOptions.find(m => m.value === sc.m)?.label || "Unknown Mode";
    if (sc.m === 1) { // Normal
      const gameType = padelGameTypeOptions.find(p => p.value === sc.pt)?.label || "Unknown Game";
      const deuceType = padelDeuceTypeOptions.find(p => p.value === sc.pd)?.label || "Unknown Deuce";
      modeDesc = `${mode} | ${gameType} | ${deuceType}`;
    } else { // Tournament
      const deuceType = padelDeuceTypeOptions.find(p => p.value === sc.pd)?.label || "Unknown Deuce";
      modeDesc = `${mode} | ${deuceType}`;
    }
  } else {
    // If other sports are ever added
    modeDesc = `Mode ${sc.m} | Max ${sc.max} | PT ${sc.pt} | PD ${sc.pd}`;
  }
  
  return `${sp} - ${modeDesc}`;
}

const gotoScreen = (screen) => {
  sendCommand({ cmd: 'goto', val: screen })
}

const disconnect = () => {
  if (bleServer && bleServer.connected) {
    bleServer.disconnect()
  }
}

const onDisconnected = () => {
  isConnected.value = false
  syncStatus.value = 'Disconnected'
  bleServer = null
  rxCharacteristic = null
  txCharacteristic = null
}

let rxBuffer = ''

const handleNotifications = (event) => {
  const value = event.target.value
  const decoder = new TextDecoder('utf-8')
  const chunk = decoder.decode(value)
  rxBuffer += chunk
  
  // If the chunk contains a newline, we have a complete JSON string
  if (rxBuffer.includes('\n')) {
    const messages = rxBuffer.split('\n')
    // Keep the last partial fragment in the buffer
    rxBuffer = messages.pop()
    
    for (const jsonStr of messages) {
      if (!jsonStr.trim()) continue;
      lastResponse.value = jsonStr
      try {
        const data = JSON.parse(jsonStr)
        if (data.type === 'settings') {
          if (data.brightness !== undefined) brightness.value = data.brightness
          if (data.volume !== undefined) volume.value = data.volume
          if (data.board_type !== undefined) isBigBoard.value = (data.board_type !== 0)
          if (data.buzzer !== undefined) enableBuzzer.value = data.buzzer
          if (data.display_mode !== undefined) displayMode.value = data.display_mode
          if (data.slots !== undefined) slots.value = data.slots
          if (data.swap_teams !== undefined) swapTeams.value = data.swap_teams
          if (data.srv_bypass !== undefined) serveBypass.value = data.srv_bypass
          if (data.gr !== undefined) groupCal.value.r = data.gr
          if (data.gg !== undefined) groupCal.value.g = data.gg
          if (data.gb !== undefined) groupCal.value.b = data.gb
          if (data.a !== undefined) segmentA.value = data.a
          if (data.b !== undefined) segmentB.value = data.b
          if (data.ma !== undefined) miscA.value = data.ma
          if (data.mb !== undefined) miscB.value = data.mb
          if (data.ble_name !== undefined) bleName.value = data.ble_name
          if (data.battery !== undefined) battery.value = data.battery
          
          if (data.sc_up) shortcuts.value.up = data.sc_up;
          if (data.sc_down) shortcuts.value.down = data.sc_down;
          if (data.sc_center) shortcuts.value.center = data.sc_center;

          if (!isSyncing.value && Object.keys(originalSettings).length > 0) {
            const nextCmd = pendingCommands.shift();
            setTimeout(() => {
              sendCommand(nextCmd);
            }, 30);
          }
        } else if (data.type === 'history') {
          matchHistory.value = data.events || []
          maxScores.value = data.max_scores || []
          goldenPoint.value = !!data.golden_point
          endlessMode.value = !!data.endless
          syncStatus.value = `Loaded ${matchHistory.value.length} events`
        } else if (data.type === 'matchCount') {
          savedMatchCount.value = data.count
          syncStatus.value = `Found ${data.count} saved matches`
        }
      } catch (e) {
        console.error("Failed to parse response:", e)
      }
    }
  }
}

let isSending = false;
let pendingCommands = [];

const sendCommand = async (payload) => {
  if (!isConnected.value || !rxCharacteristic) {
    syncStatus.value = 'Not connected'
    return
  }
  
  if (isSending) {
    pendingCommands.push(payload);
    return;
  }
  
  try {
    isSending = true;
    const jsonStr = JSON.stringify(payload)
    const encoder = new TextEncoder()
    const data = encoder.encode(jsonStr)
    await rxCharacteristic.writeValue(data)
  } catch (e) {
    console.error("Write error:", e)
    syncStatus.value = 'Failed to send command'
  } finally {
    isSending = false;
    if (pendingCommands.length > 0) {
      const nextCmd = pendingCommands.shift();
      setTimeout(() => {
        sendCommand(nextCmd);
      }, 30);
    }
  }
}

const updateBleName = (name) => {
  sendCommand({ cmd: 'setBleName', val: name })
}

const debounceTimers = new Map();
const debouncedSendCommand = (payload, key) => {
  if (debounceTimers.has(key)) {
    clearTimeout(debounceTimers.get(key));
  }
  debounceTimers.set(key, setTimeout(() => {
    sendCommand(payload);
    debounceTimers.delete(key);
  }, 200));
}

const syncTime = async () => {
  isSyncing.value = true
  const now = new Date()
  // Adjust the timestamp by the local timezone offset so the ESP32's RTC receives the correct local time
  const localTimestamp = Math.floor((now.getTime() - (now.getTimezoneOffset() * 60000)) / 1000)
  console.log(localTimestamp)
  await sendCommand({ cmd: 'setTime', val: localTimestamp })
  syncStatus.value = 'Time synced!'
  syncSuccess.value = true
  isSyncing.value = false
}

const startMatch = async () => {
  const payload = {
    cmd: 'startGame',
    sp: startGameSport.value
  };
  
  if (startGameSport.value === 0) {
    // Volley
    payload.m = startGameModeVolley.value;
    payload.max = startGameMaxScoreIndex.value;
    // Fill dummy values for padel so the parser is happy
    payload.pt = 0;
    payload.pd = 0;
  } else {
    // Padel
    payload.m = startGameModePadel.value;
    payload.pt = startPadelGameType.value;
    payload.pd = startPadelDeuceType.value;
    payload.max = 0; // Dummy
  }
  
  await sendCommand(payload);
  syncStatus.value = 'Match started!';
  
  // Clear the history to avoid visual glitches in the Live Control tab
  matchHistory.value = [];
  
  // Switch to live tab
  activeTab.value = 'live';
}

const setBrightness = async (val) => {
  debouncedSendCommand({ cmd: 'setBrightnessVal', val }, 'brightness')
  hasUnsavedSettings.value = true
}

const setVolume = async (val) => {
  debouncedSendCommand({ cmd: 'setVolumeVal', val }, 'volume')
  hasUnsavedSettings.value = true
}

const updateSettings = async (settingsPayload) => {
  isBigBoard.value = settingsPayload.board_type === 1;
  enableBuzzer.value = settingsPayload.buzzer;
  displayMode.value = settingsPayload.display_mode;
  swapTeams.value = settingsPayload.swap_teams;
  await sendCommand({ cmd: 'setSettings', board_type: settingsPayload.board_type, buzzer: settingsPayload.buzzer })
  await sendCommand({ cmd: 'setDisplayMode', val: settingsPayload.display_mode })
  await sendCommand({ cmd: 'setSwapTeams', val: settingsPayload.swap_teams })
  hasUnsavedSettings.value = true
}

const updateServeBypass = async (val) => {
  serveBypass.value = val
  await sendCommand({ cmd: 'setServeBypass', val })
  hasUnsavedSettings.value = true
}

const handleGroupCalUpdate = async (payload) => {
  groupCal.value[payload.color] = payload.value
  debouncedSendCommand({ cmd: 'setGroupCal', r: groupCal.value.r, g: groupCal.value.g, b: groupCal.value.b }, 'groupCal')
  hasUnsavedSettings.value = true
}

const handleDigitCalUpdate = async (payload) => {
  let sideInt = payload.side === 'A' ? 0 : 1;
  if (payload.side === 'A') {
    segmentA.value[payload.index] = payload.value;
  } else {
    segmentB.value[payload.index] = payload.value;
  }
  debouncedSendCommand({ cmd: 'setDigitCal', side: sideInt, idx: payload.index, val: payload.value }, 'digitCal' + payload.side + payload.index)
  hasUnsavedSettings.value = true
}

const handleMiscCalUpdate = async (payload) => {
  let sideInt = payload.side === 'A' ? 0 : 1;
  if (payload.side === 'A') {
    miscA.value[payload.index] = payload.value;
  } else {
    miscB.value[payload.index] = payload.value;
  }
  debouncedSendCommand({ cmd: 'setMiscCal', side: sideInt, idx: payload.index, val: payload.value }, 'miscCal' + payload.side + payload.index)
  hasUnsavedSettings.value = true
}

const handleCalibrationImport = async (data) => {
  if (data.groupCal) groupCal.value = { ...data.groupCal }
  if (data.segmentA) segmentA.value = [...data.segmentA]
  if (data.segmentB) segmentB.value = [...data.segmentB]
  if (data.miscA) {
    if (Array.isArray(data.miscA)) miscA.value = [...data.miscA]
    else Object.keys(data.miscA).forEach(k => miscA.value[k] = data.miscA[k])
  }
  if (data.miscB) {
    if (Array.isArray(data.miscB)) miscB.value = [...data.miscB]
    else Object.keys(data.miscB).forEach(k => miscB.value[k] = data.miscB[k])
  }
  
  const payload = {
    cmd: 'setAllCal',
    gr: groupCal.value.r,
    gg: groupCal.value.g,
    gb: groupCal.value.b,
    a: segmentA.value,
    b: segmentB.value,
    ma: miscA.value,
    mb: miscB.value
  }
  await sendCommand(payload)
  syncStatus.value = 'Template applied and saved!'
}

const commitSettings = async () => {
  await sendCommand({ cmd: 'commitSettings' })
  hasUnsavedSettings.value = false
  syncStatus.value = 'Settings permanently saved to device!'
}

const getHistory = async () => {
  await sendCommand({ cmd: 'getHistory' })
  syncStatus.value = 'Fetching history...'
}

const getMatchCount = async () => {
  await sendCommand({ cmd: 'getMatchCount' })
  syncStatus.value = 'Fetching match count...'
}

const getSavedMatch = async (index) => {
  await sendCommand({ cmd: 'getSavedMatch', val: index })
  syncStatus.value = `Fetching saved match ${index}...`
}

const clearMatches = async () => {
  await sendCommand({ cmd: 'clearMatches' })
  syncStatus.value = 'Clearing matches...'
  savedMatchCount.value = 0
}

onUnmounted(() => {
  disconnect()
})
</script>

<template>
  <n-config-provider :theme="darkTheme">
    <n-global-style />
    <n-layout position="absolute" class="app-layout">
      <n-layout-header bordered class="header">
        <n-gradient-text type="info" :size="32" class="title">
          Netscore Platform
        </n-gradient-text>
      </n-layout-header>
      
      <n-layout-content class="content">
        <n-card class="hero-card" hoverable>
          <template #header>
            <n-h1 style="margin: 0; font-weight: 800; letter-spacing: -1px;">
              Scoreboard Settings
            </n-h1>
          </template>
          <n-space vertical size="large">
            <n-text depth="3" class="description">
              Connect to your Netscore Scoreboard via Bluetooth to synchronize its internal clock and adjust settings.
            </n-text>
            
            <n-space vertical v-if="!isConnected">
              <n-button 
                type="info" 
                size="large" 
                round 
                @click="connect"
              >
                Connect to Scoreboard
              </n-button>
            </n-space>
            
            <div v-else style="width: 100%;">
              <n-card v-if="hasUnsavedSettings" style="background: rgba(242, 201, 125, 0.1); border: 1px solid #f2c97d; margin-bottom: 16px;">
                <n-space justify="space-between" align="center">
                  <n-text type="warning" strong>You have unsaved changes that will be lost on reboot.</n-text>
                  <n-button type="warning" @click="commitSettings">Save to Device</n-button>
                </n-space>
              </n-card>

              <n-tabs type="segment" animated v-model:value="activeTab">
                <n-tab-pane name="start" tab="Start Match">
                  <n-space vertical size="large" style="margin-top: 16px;">
                    <n-grid :cols="1" :y-gap="16">
                      <n-grid-item>
                        <n-radio-group v-model:value="startGameSport" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in sportOptions" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>

                      <n-grid-item v-if="startGameSport === 0">
                        <n-radio-group v-model:value="startGameModeVolley" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in modeVolleyOptions" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>

                      <n-grid-item v-if="startGameSport === 0">
                        <n-radio-group v-model:value="startGameMaxScoreIndex" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in maxScoreOptionsVolley" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>

                      <n-grid-item v-if="startGameSport === 1">
                        <n-radio-group v-model:value="startGameModePadel" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in modePadelOptions" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>

                      <n-grid-item v-if="startGameSport === 1 && startGameModePadel === 1">
                        <n-radio-group v-model:value="startPadelGameType" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in padelGameTypeOptions" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>

                      <n-grid-item v-if="startGameSport === 1">
                        <n-radio-group v-model:value="startPadelDeuceType" size="large" style="width: 100%; display: flex;">
                          <n-radio-button v-for="option in padelDeuceTypeOptions" :key="option.value" :value="option.value" style="flex: 1; text-align: center;">
                            {{ option.label }}
                          </n-radio-button>
                        </n-radio-group>
                      </n-grid-item>
                    </n-grid>
                    
                    <n-button type="primary" size="large" block @click="startMatch" style="margin-top: 16px;">
                      START MATCH
                    </n-button>
                    
                    <n-divider />
                    <n-text strong style="font-size: 1.1em;">Boot Shortcuts</n-text>
                    <n-text depth="3" style="display: block; margin-bottom: 8px;">Hold a button while powering on to quick-start a match.</n-text>
                    
                    <n-space vertical size="medium">
                      <!-- Up Shortcut -->
                      <n-card size="small" style="background: rgba(255,255,255,0.05)">
                        <n-space justify="space-between" align="center">
                          <n-text strong>Up Button Shortcut</n-text>
                          <n-switch v-model:value="shortcuts.up.en" @update:value="saveShortcut(0)" />
                        </n-space>
                        <n-text depth="3" v-if="shortcuts.up.en" style="display: block; margin-top: 4px;">
                          {{ getShortcutDescription(shortcuts.up) }}
                        </n-text>
                        <n-button style="margin-top: 12px;" size="small" type="primary" secondary @click="saveCurrentToShortcut(0)">
                          Save Current Config to Up
                        </n-button>
                      </n-card>

                      <!-- Center Shortcut -->
                      <n-card size="small" style="background: rgba(255,255,255,0.05)">
                        <n-space justify="space-between" align="center">
                          <n-text strong>Center Button Shortcut</n-text>
                          <n-switch v-model:value="shortcuts.center.en" @update:value="saveShortcut(2)" />
                        </n-space>
                        <n-text depth="3" v-if="shortcuts.center.en" style="display: block; margin-top: 4px;">
                          {{ getShortcutDescription(shortcuts.center) }}
                        </n-text>
                        <n-button style="margin-top: 12px;" size="small" type="primary" secondary @click="saveCurrentToShortcut(2)">
                          Save Current Config to Center
                        </n-button>
                      </n-card>

                      <!-- Down Shortcut -->
                      <n-card size="small" style="background: rgba(255,255,255,0.05)">
                        <n-space justify="space-between" align="center">
                          <n-text strong>Down Button Shortcut</n-text>
                          <n-switch v-model:value="shortcuts.down.en" @update:value="saveShortcut(1)" />
                        </n-space>
                        <n-text depth="3" v-if="shortcuts.down.en" style="display: block; margin-top: 4px;">
                          {{ getShortcutDescription(shortcuts.down) }}
                        </n-text>
                        <n-button style="margin-top: 12px;" size="small" type="primary" secondary @click="saveCurrentToShortcut(1)">
                          Save Current Config to Down
                        </n-button>
                      </n-card>
                    </n-space>

                  </n-space>
                </n-tab-pane>

                <n-tab-pane name="live" tab="Live Control">
                  <n-space vertical>
                    <n-button type="error" size="medium" round ghost @click="disconnect">
                      Disconnect
                    </n-button>
                    
                    <n-grid :x-gap="24" :y-gap="24" cols="1 m:2" responsive="screen">
                      <n-grid-item>
                        <n-space vertical justify="space-between" style="margin-top: 16px; padding: 16px; background: rgba(0, 0, 0, 0.2); border-radius: 8px; height: 100%;">
                          <n-space justify="center" style="margin-bottom: 8px; width: 100%;">
                            <n-radio-group v-model:value="selectedDevice" size="medium">
                              <n-radio-button :value="Device.NONE">None</n-radio-button>
                              <n-radio-button :value="Device.DEVICE_1">Dev 1</n-radio-button>
                              <n-radio-button :value="Device.DEVICE_2">Dev 2</n-radio-button>
                            </n-radio-group>
                          </n-space>
                          <n-space vertical align="center" style="height: 100%; width: 100%;">
                            <!-- Main buttons -->
                            <n-grid :cols="2" x-gap="12" style="width: 100%;">
                              <n-grid-item>
                                <n-button-group vertical style="width: 100%;">
                                  <n-button 
                                    type="info" 
                                    style="user-select: none; -webkit-user-select: none; width: 100%;"
                                    @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BUTTON_DOWN_HOLD)"
                                    @pointerup="onPointerUp(selectedDevice, ButtonEvent.BUTTON_DOWN_PRESS, ButtonEvent.BUTTON_DOWN_DOUBLE_PRESS)"
                                    @pointerleave="onPointerLeave"
                                    @contextmenu.prevent
                                  >
                                    ▼ Down
                                  </n-button>
                                  <n-button 
                                    type="primary" 
                                    style="user-select: none; -webkit-user-select: none; width: 100%;"
                                    @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BUTTON_CENTER_HOLD)"
                                    @pointerup="onPointerUp(selectedDevice, ButtonEvent.BUTTON_CENTER_PRESS, ButtonEvent.BUTTON_CENTER_DOUBLE_PRESS)"
                                    @pointerleave="onPointerLeave"
                                    @contextmenu.prevent
                                  >
                                    ● Center
                                  </n-button>
                                  <n-button 
                                    type="info" 
                                    style="user-select: none; -webkit-user-select: none; width: 100%;"
                                    @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BUTTON_UP_HOLD)"
                                    @pointerup="onPointerUp(selectedDevice, ButtonEvent.BUTTON_UP_PRESS, ButtonEvent.BUTTON_UP_DOUBLE_PRESS)"
                                    @pointerleave="onPointerLeave"
                                    @contextmenu.prevent
                                  >
                                    ▲ Up
                                  </n-button>
                                </n-button-group>
                              </n-grid-item>
                              <n-grid-item>
                                <n-space vertical style="height: 100%;" justify="center">
                                  <n-button 
                                    type="error" 
                                    style="user-select: none; -webkit-user-select: none; width: 100%; padding: 24px 0;"
                                    @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BUTTON_POWER_HOLD)"
                                    @pointerup="onPointerUp(selectedDevice, ButtonEvent.BUTTON_POWER_PRESS, ButtonEvent.BUTTON_POWER_DOUBLE_PRESS)"
                                    @pointerleave="onPointerLeave"
                                    @contextmenu.prevent
                                  >
                                    Power
                                  </n-button>
                                </n-space>
                              </n-grid-item>
                            </n-grid>

                            <n-divider style="margin: 8px 0;" />

                            <!-- Peripherals -->
                            <n-grid :cols="3" x-gap="8" y-gap="8" style="width: 100%;">
                              <n-grid-item>
                                <n-button 
                                  type="success" 
                                  ghost
                                  size="small"
                                  style="user-select: none; -webkit-user-select: none; width: 100%;"
                                  @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BLE_BTN_A_HOLD)"
                                  @pointerup="onPointerUp(selectedDevice, ButtonEvent.BLE_BTN_A_PRESS)"
                                  @pointerleave="onPointerLeave"
                                  @contextmenu.prevent
                                >
                                  Shutter A
                                </n-button>
                              </n-grid-item>
                              <n-grid-item>
                                <n-button 
                                  type="warning" 
                                  ghost
                                  size="small"
                                  style="user-select: none; -webkit-user-select: none; width: 100%;"
                                  @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BLE_BTN_B_HOLD)"
                                  @pointerup="onPointerUp(selectedDevice, ButtonEvent.BLE_BTN_B_PRESS)"
                                  @pointerleave="onPointerLeave"
                                  @contextmenu.prevent
                                >
                                  Shutter B
                                </n-button>
                              </n-grid-item>
                              <n-grid-item>
                                <n-button 
                                  type="default" 
                                  size="small"
                                  style="user-select: none; -webkit-user-select: none; width: 100%;"
                                  @pointerdown="onPointerDown(selectedDevice, ButtonEvent.BLE_BTN_HOLD)"
                                  @pointerup="onPointerUp(selectedDevice, ButtonEvent.BLE_BTN_PRESS)"
                                  @pointerleave="onPointerLeave"
                                  @contextmenu.prevent
                                >
                                  1-Btn
                                </n-button>
                              </n-grid-item>
                              <n-grid-item span="3">
                                <n-button 
                                  type="info" 
                                  ghost
                                  size="small"
                                  style="user-select: none; -webkit-user-select: none; width: 100%;"
                                  @pointerdown="onPointerDown(selectedDevice, ButtonEvent.ITAG_PRESS)"
                                  @pointerup="onPointerUp(selectedDevice, ButtonEvent.ITAG_PRESS, ButtonEvent.ITAG_DOUBLE_PRESS)"
                                  @pointerleave="onPointerLeave"
                                  @contextmenu.prevent
                                >
                                  iTag Button
                                </n-button>
                              </n-grid-item>
                            </n-grid>

                            <n-text depth="3" style="font-size: 0.85em; margin-top: 8px;">Tap: Navigate/Score &nbsp;|&nbsp; Hold: Select/Undo</n-text>
                          </n-space>
                        </n-space>
                      </n-grid-item>

                      <n-grid-item>
                        <n-space vertical justify="space-between" style="margin-top: 16px; padding: 16px; background: rgba(0, 0, 0, 0.2); border-radius: 8px; height: 100%; box-sizing: border-box;">
                          <n-text strong style="display: block; font-size: 1.1em;">Device Settings</n-text>
                          
                          <n-space justify="space-between" align="center" style="margin-top: 8px;">
                            <n-text strong>Synchronize Internal Time</n-text>
                            <n-button type="primary" size="small" @click="syncTime" :loading="isSyncing">Sync Time</n-button>
                          </n-space>

                          <n-space justify="space-between" align="center" style="margin-top: 16px;">
                            <n-text strong>Brightness</n-text>
                            <div style="width: 150px">
                              <n-slider v-model:value="brightness" :min="0" :max="100" :marks="{0:'0', 100:'100'}" @update:value="setBrightness" />
                            </div>
                          </n-space>

                          <n-space justify="space-between" align="center" style="margin-top: 24px;">
                            <n-text strong>Volume</n-text>
                            <div style="width: 150px">
                              <n-slider v-model:value="volume" :min="0" :max="100" :marks="{0:'0', 100:'100'}" @update:value="setVolume" />
                            </div>
                          </n-space>
                          
                          <n-divider style="margin: 12px 0 8px 0;" />
                          
                          <n-space justify="space-between" align="center">
                            <n-text strong>Active Match History</n-text>
                            <n-button @click="getHistory" type="info" size="small">Fetch Live Match</n-button>
                          </n-space>
                        </n-space>
                      </n-grid-item>

                      <n-grid-item span="1 m:2">
                        <n-space vertical style="margin-top: 16px; padding: 16px; background: rgba(0, 0, 0, 0.2); border-radius: 8px;">
                          <n-text strong style="display: block; font-size: 1.1em; margin-bottom: 8px;">Screen Selection</n-text>
                          <n-space vertical size="large">
                            <div v-for="category in screenCategories" :key="category.name">
                              <n-text depth="3" style="font-size: 0.85em; text-transform: uppercase; letter-spacing: 1px; display: block; margin-bottom: 8px;">{{ category.name }}</n-text>
                              <n-space>
                                <n-button 
                                  v-for="screen in category.screens" 
                                  :key="screen.value" 
                                  size="small" 
                                  secondary
                                  @click="sendCommand({ cmd: 'goto', val: screen.value })"
                                >
                                  {{ screen.label }}
                                </n-button>
                              </n-space>
                            </div>
                          </n-space>
                        </n-space>
                      </n-grid-item>
                    </n-grid>
                  </n-space>
                </n-tab-pane>

                <n-tab-pane name="saved" tab="Saved Matches">
                  <n-space vertical>
                    <n-space justify="space-between" align="center" style="margin-top: 16px; padding: 16px; background: rgba(0, 0, 0, 0.2); border-radius: 8px;">
                      <n-text strong style="display: block; margin-bottom: 8px; font-size: 1.1em;">Saved Match Storage</n-text>
                      <n-space vertical align="center" style="width: 100%;">
                        <n-space align="center" justify="space-between" style="width: 100%;">
                          <n-text strong>Matches Stored: {{ savedMatchCount }}</n-text>
                          <n-button type="info" size="small" @click="getMatchCount">Refresh Count</n-button>
                        </n-space>
                        
                        <n-divider style="margin: 8px 0;" />
                        
                        <n-space style="width: 100%;" :wrap="false" align="center">
                          <n-input-number v-model:value="savedMatchIndex" :min="0" :max="Math.max(0, savedMatchCount - 1)" style="flex: 1;" placeholder="Index" />
                          <n-button type="primary" @click="getSavedMatch(savedMatchIndex)" :disabled="savedMatchCount === 0">Fetch Match</n-button>
                        </n-space>
                        
                        <n-divider style="margin: 8px 0;" />
                        
                        <n-popconfirm @positive-click="clearMatches">
                          <template #trigger>
                            <n-button type="error" ghost block>Clear All Matches</n-button>
                          </template>
                          Are you sure you want to delete all saved matches from the device? This cannot be undone.
                        </n-popconfirm>
                      </n-space>
                    </n-space>
                  </n-space>
                </n-tab-pane>

                <n-tab-pane name="settings" tab="Settings">
                  <n-space vertical>
                    <SettingsTab 
                      :isBigBoard="isBigBoard" 
                      :enableBuzzer="enableBuzzer" 
                      :displayMode="displayMode"
                      :slots="slots"
                      :swapTeams="swapTeams"
                      :bleName="bleName"
                      :battery="battery"
                      :serveBypass="serveBypass"
                      @update:settings="updateSettings" 
                      @update:bleName="updateBleName"
                      @update:serveBypass="updateServeBypass"
                    />
                  </n-space>
                </n-tab-pane>

                <n-tab-pane name="calibration" tab="Calibration">
                  <CalibrationTab
                    :groupCal="groupCal"
                    :segmentA="segmentA"
                    :segmentB="segmentB"
                    :miscA="miscA"
                    :miscB="miscB"
                    :isBigBoard="isBigBoard"
                    @update:group="handleGroupCalUpdate"
                    @update:digit="handleDigitCalUpdate"
                    @update:misc="handleMiscCalUpdate"
                    @import:calibration="handleCalibrationImport"
                  />
                </n-tab-pane>
              </n-tabs>

              <div v-if="filteredHistory.length > 0" style="margin-top: 24px;">
                <!-- Set Results Summary -->
                <div style="padding: 16px; background: rgba(0, 0, 0, 0.2); border-radius: 8px; margin-bottom: 16px;">
                  <n-text strong style="display: block; margin-bottom: 12px; font-size: 1.1em;">Match Score</n-text>
                  <n-space align="center" size="large">
                    <div v-for="n in (isPadel ? (currentPadelScore?.home_sets + currentPadelScore?.away_sets + 1) : (currentScore?.home_sets + currentScore?.away_sets + 1))" :key="n" style="background: rgba(255,255,255,0.05); padding: 12px; border-radius: 12px; text-align: center; min-width: 80px; box-shadow: 0 4px 6px rgba(0,0,0,0.1);">
                      <n-text depth="3" style="font-size: 0.8em; text-transform: uppercase; letter-spacing: 1px;">Set {{ n }}</n-text>
                      <div style="font-size: 1.5em; font-weight: bold; margin-top: 8px; display: flex; justify-content: center; align-items: center; gap: 8px;">
                        <span style="color: #63e2b7;">{{ isPadel ? currentPadelScore?.set_games_home[n-1] : currentScore?.set_points_home[n-1] }}</span>
                        <span style="color: rgba(255,255,255,0.2); font-size: 0.8em;">-</span>
                        <span style="color: #f2c97d;">{{ isPadel ? currentPadelScore?.set_games_away[n-1] : currentScore?.set_points_away[n-1] }}</span>
                      </div>
                    </div>
                  </n-space>
                </div>

                <!-- Event Log -->
                <n-text strong style="display: block; margin-bottom: 8px;">Event Log</n-text>
                <div style="padding: 10px; background: rgba(255,255,255,0.05); border-radius: 8px; max-height: 250px; overflow-y: auto;">
                  <n-table size="small" :single-line="false">
                    <thead>
                    <tr>
                      <th>Time</th>
                      <th>Team</th>
                      <th>Event</th>
                      <th>Sport</th>
                    </tr>
                  </thead>
                  <tbody>
                    <tr v-for="(ev, i) in filteredHistory.slice().reverse()" :key="i">
                      <td>{{ new Date(ev.ts * 1000).toISOString().substr(11, 8) }}</td>
                      <td>
                        <n-text :type="ev.team === 0 ? 'info' : 'warning'" strong>
                          {{ ev.team === 0 ? 'HOME' : 'AWAY' }}
                        </n-text>
                      </td>
                      <td>
                        <n-text :type="ev.evt === 0 ? 'success' : 'error'">
                          {{ ev.evt === 0 ? 'Scored' : 'Undo' }}
                        </n-text>
                      </td>
                      <td>
                        <n-text :type="ev.sport === 0 ? 'info' : 'success'">
                          {{ getSportName(ev.sport) }} ({{ getModeName(ev.game_mode) }})
                        </n-text>
                      </td>
                    </tr>
                  </tbody>
                </n-table>
              </div>
              
              </div>
              
              <div v-if="lastResponse" style="margin-top: 16px; padding: 10px; background: rgba(255,255,255,0.05); border-radius: 8px;">
                <n-text depth="3" style="font-size: 12px; font-family: monospace; word-break: break-all;">
                  Last Device Response: {{ lastResponse.substring(0, 100) }}...
                </n-text>
              </div>
            </div>
              
            <n-text 
              v-if="syncStatus" 
              :type="syncSuccess ? 'success' : (isSyncing ? 'info' : 'default')"
              style="margin-top: 10px; display: block;"
            >
              Status: {{ syncStatus }}
            </n-text>
          </n-space>
        </n-card>
      </n-layout-content>
      
      <n-layout-footer bordered class="footer">
        <n-text depth="3">© 2026 Netscore Systems</n-text>
      </n-layout-footer>
    </n-layout>
  </n-config-provider>
</template>

<style scoped>
.app-layout {
  background: radial-gradient(circle at top right, #1a202c, #0d1117);
}

.header {
  padding: 24px;
  background: rgba(13, 17, 23, 0.7);
  backdrop-filter: blur(12px);
  display: flex;
  justify-content: center;
}

.title {
  font-weight: 800;
  letter-spacing: -0.5px;
}

.content {
  padding: 48px;
  min-height: calc(100vh - 160px);
  display: flex;
  align-items: center;
  justify-content: center;
  background: transparent;
}

.hero-card {
  max-width: 900px;
  width: 100%;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.03);
  backdrop-filter: blur(20px);
  border: 1px solid rgba(255, 255, 255, 0.08);
  box-shadow: 0 25px 50px -12px rgba(0, 0, 0, 0.5);
  transition: transform 0.3s ease, box-shadow 0.3s ease;
}

.hero-card:hover {
  transform: translateY(-5px);
  box-shadow: 0 30px 60px -12px rgba(0, 0, 0, 0.6);
}

.description {
  font-size: 1.1rem;
  line-height: 1.6;
}

.action-buttons {
  margin-top: 16px;
}

.footer {
  padding: 24px;
  text-align: center;
  background: rgba(13, 17, 23, 0.9);
}
</style>
